#include "ffplayer.h"

// 初始化FFmpeg
int init_ffmpeg(ffplayer * s, const char * filename)
{
    // 打开媒体文件
    if(avformat_open_input(&s->fmt_ctx, filename, NULL, NULL) != 0) {
        printf("[ff]file open failed\n");
        return -1;
    }

    // 获取流信息
    if(avformat_find_stream_info(s->fmt_ctx, NULL) < 0) {
        printf("[ff]stream not found\n");
        return -1;
    }

    // 查找视频流和音频流
    s->video_stream_idx = -1;
    s->audio_stream_idx = -1;
    for(int i = 0; i < s->fmt_ctx->nb_streams; i++) {
        AVStream * stream = s->fmt_ctx->streams[i];
        if(stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO && s->video_stream_idx == -1) {
            s->video_stream_idx = i;
        } else if(stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO && s->audio_stream_idx == -1) {
            s->audio_stream_idx = i;
        }
    }

    // 初始化视频解码器
    if(s->video_stream_idx != -1) {
        AVStream * video_stream     = s->fmt_ctx->streams[s->video_stream_idx];
        const AVCodec * video_codec = avcodec_find_decoder(video_stream->codecpar->codec_id);
        if(!video_codec) {
            printf("[ff]video decoder not found\n");
            return -1;
        }

        s->video_dec_ctx = avcodec_alloc_context3(video_codec);
        avcodec_parameters_to_context(s->video_dec_ctx, video_stream->codecpar);
        if(avcodec_open2(s->video_dec_ctx, video_codec, NULL) < 0) {
            printf("[ff]video decoder open failed\n");
            return -1;
        }
    }

    // 初始化音频解码器
    if(s->audio_stream_idx != -1) {
        AVStream * audio_stream     = s->fmt_ctx->streams[s->audio_stream_idx];
        const AVCodec * audio_codec = avcodec_find_decoder(audio_stream->codecpar->codec_id);
        if(!audio_codec) {
            printf("[ff]audio decoder not found\n");
            return -1;
        }

        s->audio_dec_ctx = avcodec_alloc_context3(audio_codec);
        avcodec_parameters_to_context(s->audio_dec_ctx, audio_stream->codecpar);
        if(avcodec_open2(s->audio_dec_ctx, audio_codec, NULL) < 0) {
            printf("[ff]audio decoder open failed\n");
            return -1;
        }
    }

    s->frame = av_frame_alloc();
    av_init_packet(&s->pkt);
    return 0;
}

// 初始化ALSA
int init_alsa(ffplayer * s)
{
    if(s->audio_stream_idx == -1) return 0;

    s->sample_rate    = s->audio_dec_ctx->sample_rate;
    s->audio_channels = s->audio_dec_ctx->channels;

    // 打开PCM设备
    if(snd_pcm_open(&s->audio_handle, "default", SND_PCM_STREAM_PLAYBACK, 0) < 0) {
        printf("[ff]pcm open failed\n");
        return -1;
    }

    // 配置PCM参数
    snd_pcm_hw_params_t * params;
    snd_pcm_hw_params_alloca(&params);
    snd_pcm_hw_params_any(s->audio_handle, params);
    snd_pcm_hw_params_set_access(s->audio_handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(s->audio_handle, params, SND_PCM_FORMAT_S16_LE);
    snd_pcm_hw_params_set_channels(s->audio_handle, params, s->audio_channels);
    snd_pcm_hw_params_set_rate_near(s->audio_handle, params, (unsigned int *)&s->sample_rate, 0);

    if(snd_pcm_hw_params(s->audio_handle, params) < 0) {
        printf("[ff]pcm set params failed\n");
        return -1;
    }

    return 0;
}

// 初始化LVGL视频显示
void init_lvgl_video(ffplayer * s, lv_obj_t * img_obj)
{
    s->video_img = img_obj;
    int width    = s->video_dec_ctx->width;
    int height   = s->video_dec_ctx->height;

    // 分配RGB缓冲区
    s->img_buf           = malloc(width * height * 4); // ARGB32格式
    lv_img_dsc_t img_dsc = {.header.always_zero = 0,
                            .header.w           = width,
                            .header.h           = height,
                            .data_size          = width * height * 4,
                            .header.cf          = LV_IMG_CF_TRUE_COLOR,
                            .data               = s->img_buf};

    // 设置图像源
    lv_img_set_src(s->video_img, &img_dsc);

    // 初始化图像转换器
    s->sws_ctx = sws_getContext(width, height, s->video_dec_ctx->pix_fmt, width, height, AV_PIX_FMT_BGRA, SWS_BILINEAR,
                                NULL, NULL, NULL);
}

// 视频解码线程
void * video_thread_func(void * arg)
{
    ffplayer * s = (ffplayer *)arg;

    while(!s->quit_flag && av_read_frame(s->fmt_ctx, &s->pkt) >= 0) {
        if(s->pkt.stream_index == s->video_stream_idx) {
            // 发送数据包到解码器
            if(avcodec_send_packet(s->video_dec_ctx, &s->pkt) < 0) {
                av_packet_unref(&s->pkt);
                continue;
            }

            // 接收解码后的帧
            while(avcodec_receive_frame(s->video_dec_ctx, s->frame) == 0) {
                // 转换图像格式为BGRA
                uint8_t * dst[]    = {s->img_buf};
                int dst_linesize[] = {s->video_dec_ctx->width * 4};
                sws_scale(s->sws_ctx, (const uint8_t * const *)s->frame->data, s->frame->linesize, 0,
                          s->video_dec_ctx->height, dst, dst_linesize);

                // 通知LVGL刷新显示
                lv_obj_invalidate(s->video_img);

                // 简单帧率控制
                usleep(1000000 / 30);
            }
        }
        av_packet_unref(&s->pkt);
    }
    return NULL;
}

// 音频解码线程
void * audio_thread_func(void * arg)
{
    ffplayer * s = (ffplayer *)arg;
    if(s->audio_stream_idx == -1) return NULL;

    while(!s->quit_flag && av_read_frame(s->fmt_ctx, &s->pkt) >= 0) {
        if(s->pkt.stream_index == s->audio_stream_idx) {
            // 发送数据包到解码器
            if(avcodec_send_packet(s->audio_dec_ctx, &s->pkt) < 0) {
                av_packet_unref(&s->pkt);
                continue;
            }

            // 接收解码后的帧
            while(avcodec_receive_frame(s->audio_dec_ctx, s->frame) == 0) {
                // 播放音频
                int16_t * audio_buf = (int16_t *)s->frame->data[0];
                int frames          = s->frame->nb_samples;

                snd_pcm_sframes_t written = snd_pcm_writei(s->audio_handle, audio_buf, frames);

                if(written < 0) {
                    // 处理音频恢复
                    snd_pcm_recover(s->audio_handle, written, 0);
                }
            }
        }
        av_packet_unref(&s->pkt);
    }
    return NULL;
}

// 启动播放器
void start_player(ffplayer * s)
{
    s->quit_flag = 0;
    pthread_create(&s->video_thread, NULL, video_thread_func, s);
    pthread_create(&s->audio_thread, NULL, audio_thread_func, s);
}

// 停止播放器
void stop_player(ffplayer * s)
{
    s->quit_flag = 1;
    pthread_join(s->video_thread, NULL);
    pthread_join(s->audio_thread, NULL);
}

// 清理资源
void cleanup_player(ffplayer * s)
{
    if(s->video_dec_ctx) avcodec_free_context(&s->video_dec_ctx);
    if(s->audio_dec_ctx) avcodec_free_context(&s->audio_dec_ctx);
    if(s->fmt_ctx) avformat_close_input(&s->fmt_ctx);
    if(s->frame) av_frame_free(&s->frame);
    if(s->audio_handle) snd_pcm_close(s->audio_handle);
    if(s->sws_ctx) sws_freeContext(s->sws_ctx);
    if(s->img_buf) free(s->img_buf);
}
