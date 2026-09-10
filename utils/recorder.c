/**
 * 一个简单的录音组件
 * 使用 ALSA 默认采集设备读取音频（S16_LE）
 * 通过 ffmpeg（libavcodec/libavformat/libswresample）编码为音频文件
 * 录制的参数：44100Hz / 单声道 / 128kbps
 *
 * 初始化（ALSA 与 FFmpeg）在 recorder_start 中同步完成，
 * 失败时返回负值，便于上层进行错误处理。
 */

#include "recorder.h"

#include <unistd.h>

#include <libavutil/opt.h>
#include <libavutil/samplefmt.h>

#define REC_SAMPLE_RATE 44100
#define REC_CHANNELS    1
#define REC_BIT_RATE    128000
#define REC_FRAME_SIZE  1152
#define REC_PERIOD_SIZE 1024

static void * recorder_thread_func(void * arg);
static void recorder_cleanup(recorder_t * recorder);

recorder_t * recorder_init(void)
{
    recorder_t * recorder = malloc(sizeof(recorder_t));
    if(!recorder) return NULL;

    memset(recorder, 0, sizeof(recorder_t));
    atomic_store(&recorder->state, RECORDER_STOPPED);

    return recorder;
}

int recorder_start(recorder_t * recorder, const char * file_name)
{
    if(!recorder || !file_name) return -1;

    // 已在录制则忽略
    if(atomic_load(&recorder->state) == RECORDER_RECORDING) return -2;

    int ret = 0;

    free(recorder->filename);
    recorder->filename = strdup(file_name);
    recorder->pts      = 0;

    // 1. 打开 ALSA 采集设备
    if((ret = snd_pcm_open(&recorder->pcm_handle, "default", SND_PCM_STREAM_CAPTURE, 0)) < 0) {
        fprintf(stderr, "[recorder]无法打开采集设备: %s\n", snd_strerror(ret));
        ret = -10;
        goto cleanup;
    }

    snd_pcm_hw_params_t * hw_params;
    snd_pcm_hw_params_alloca(&hw_params);

    unsigned int sample_rate = REC_SAMPLE_RATE;
    snd_pcm_uframes_t period_size = REC_PERIOD_SIZE;
    snd_pcm_uframes_t buffer_size = period_size * 4;

    snd_pcm_hw_params_any(recorder->pcm_handle, hw_params);
    snd_pcm_hw_params_set_access(recorder->pcm_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(recorder->pcm_handle, hw_params, SND_PCM_FORMAT_S16_LE);
    snd_pcm_hw_params_set_channels(recorder->pcm_handle, hw_params, REC_CHANNELS);
    snd_pcm_hw_params_set_rate_near(recorder->pcm_handle, hw_params, &sample_rate, 0);
    snd_pcm_hw_params_set_period_size_near(recorder->pcm_handle, hw_params, &period_size, 0);
    snd_pcm_hw_params_set_buffer_size_near(recorder->pcm_handle, hw_params, &buffer_size);

    if((ret = snd_pcm_hw_params(recorder->pcm_handle, hw_params)) < 0) {
        fprintf(stderr, "[recorder]无法设置采集参数: %s\n", snd_strerror(ret));
        ret = -11;
        goto cleanup;
    }

    // 2. 根据文件扩展名分配输出上下文
    if((ret = avformat_alloc_output_context2(&recorder->fmt_ctx, NULL, NULL, recorder->filename)) < 0) {
        fprintf(stderr, "[recorder]无法创建输出上下文\n");
        ret = -12;
        goto cleanup;
    }

    // 3. 查找编码器
    const AVCodec * codec = avcodec_find_encoder(AV_CODEC_ID_AAC);
    if(!codec) {
        fprintf(stderr, "[recorder]未找到编码器\n");
        ret = -13;
        goto cleanup;
    }

    // 4. 分配并配置编码上下文
    recorder->codec_ctx = avcodec_alloc_context3(codec);
    if(!recorder->codec_ctx) {
        fprintf(stderr, "[recorder]无法分配编码上下文\n");
        ret = -14;
        goto cleanup;
    }

    recorder->codec_ctx->bit_rate       = REC_BIT_RATE;
    recorder->codec_ctx->sample_fmt     = AV_SAMPLE_FMT_FLTP;
    recorder->codec_ctx->sample_rate    = sample_rate;
    recorder->codec_ctx->channels       = REC_CHANNELS;
    recorder->codec_ctx->channel_layout = av_get_default_channel_layout(REC_CHANNELS);

    if((ret = avcodec_open2(recorder->codec_ctx, codec, NULL)) < 0) {
        fprintf(stderr, "[recorder]无法打开编码器\n");
        ret = -15;
        goto cleanup;
    }

    recorder->frame_size = REC_FRAME_SIZE;
    if(recorder->codec_ctx->frame_size > 0) recorder->frame_size = recorder->codec_ctx->frame_size;

    // 5. 新建音频流
    recorder->stream = avformat_new_stream(recorder->fmt_ctx, NULL);
    if(!recorder->stream) {
        fprintf(stderr, "[recorder]无法创建音频流\n");
        ret = -16;
        goto cleanup;
    }
    recorder->stream->time_base = (AVRational){ 1, recorder->codec_ctx->sample_rate };
    if((ret = avcodec_parameters_from_context(recorder->stream->codecpar, recorder->codec_ctx)) < 0) {
        fprintf(stderr, "[recorder]无法复制编码参数\n");
        ret = -17;
        goto cleanup;
    }

    // 6. 打开输出文件并写入文件头
    if(!(recorder->fmt_ctx->oformat->flags & AVFMT_NOFILE)) {
        if((ret = avio_open(&recorder->fmt_ctx->pb, recorder->filename, AVIO_FLAG_WRITE)) < 0) {
            fprintf(stderr, "[recorder]无法打开输出文件: %s\n", recorder->filename);
            ret = -18;
            goto cleanup;
        }
    }

    if((ret = avformat_write_header(recorder->fmt_ctx, NULL)) < 0) {
        fprintf(stderr, "[recorder]无法写入文件头\n");
        ret = -19;
        goto cleanup;
    }

    // 7. 分配编码帧
    recorder->frame = av_frame_alloc();
    if(!recorder->frame) {
        fprintf(stderr, "[recorder]无法分配帧\n");
        ret = -20;
        goto cleanup;
    }
    recorder->frame->nb_samples     = recorder->frame_size;
    recorder->frame->format         = recorder->codec_ctx->sample_fmt;
    recorder->frame->channel_layout = recorder->codec_ctx->channel_layout;
    recorder->frame->sample_rate    = recorder->codec_ctx->sample_rate;
    if((ret = av_frame_get_buffer(recorder->frame, 0)) < 0) {
        fprintf(stderr, "[recorder]无法分配帧缓冲区\n");
        ret = -21;
        goto cleanup;
    }

    // 8. 分配重采样器（interleaved S16 -> planar）
    recorder->swr_ctx = swr_alloc();
    if(!recorder->swr_ctx) {
        fprintf(stderr, "[recorder]无法分配重采样器\n");
        ret = -22;
        goto cleanup;
    }
    av_opt_set_int(recorder->swr_ctx, "in_channel_layout", recorder->codec_ctx->channel_layout, 0);
    av_opt_set_int(recorder->swr_ctx, "out_channel_layout", recorder->codec_ctx->channel_layout, 0);
    av_opt_set_int(recorder->swr_ctx, "in_sample_rate", recorder->codec_ctx->sample_rate, 0);
    av_opt_set_int(recorder->swr_ctx, "out_sample_rate", recorder->codec_ctx->sample_rate, 0);
    av_opt_set_sample_fmt(recorder->swr_ctx, "in_sample_fmt", AV_SAMPLE_FMT_S16, 0);
    av_opt_set_sample_fmt(recorder->swr_ctx, "out_sample_fmt", recorder->codec_ctx->sample_fmt, 0);
    if((ret = swr_init(recorder->swr_ctx)) < 0) {
        fprintf(stderr, "[recorder]无法初始化重采样器\n");
        ret = -23;
        goto cleanup;
    }

    // 9. 分配 PCM 缓冲区与数据包
    int pcm_buf_size = av_samples_get_buffer_size(NULL, REC_CHANNELS, recorder->frame_size, AV_SAMPLE_FMT_S16, 0);
    recorder->pcm_buf = av_malloc(pcm_buf_size);
    if(!recorder->pcm_buf) {
        fprintf(stderr, "[recorder]无法分配 PCM 缓冲区\n");
        ret = -24;
        goto cleanup;
    }

    recorder->pkt = av_packet_alloc();
    if(!recorder->pkt) {
        fprintf(stderr, "[recorder]无法分配数据包\n");
        ret = -25;
        goto cleanup;
    }

    // 10. 启动录制线程
    atomic_store(&recorder->state, RECORDER_RECORDING);
    if(pthread_create(&recorder->thread, NULL, recorder_thread_func, recorder) != 0) {
        fprintf(stderr, "[recorder]无法创建录制线程\n");
        atomic_store(&recorder->state, RECORDER_STOPPED);
        ret = -26;
        goto cleanup;
    }

    printf("[recorder]录制开始\n");
    return 0;

cleanup:
    recorder_cleanup(recorder);
    return ret;
}

int recorder_stop(recorder_t * recorder)
{
    if(!recorder) return -1;
    if(recorder->state != RECORDER_RECORDING) return -2;

    atomic_store(&recorder->state, RECORDER_STOPPED);

    if(recorder->thread) {
        pthread_join(recorder->thread, NULL);
        recorder->thread = 0;
    }

    recorder_cleanup(recorder);

    printf("[recorder]录制完成\n");

    return 0;
}

void recorder_destroy(recorder_t * recorder)
{
    if(!recorder) return;

    recorder_stop(recorder);
    free(recorder);
}

static void recorder_cleanup(recorder_t * recorder)
{
    if(!recorder) return;

    if(recorder->pkt) av_packet_free(&recorder->pkt);
    if(recorder->pcm_buf) av_free(recorder->pcm_buf);
    if(recorder->swr_ctx) swr_free(&recorder->swr_ctx);
    if(recorder->frame) av_frame_free(&recorder->frame);
    if(recorder->codec_ctx) avcodec_free_context(&recorder->codec_ctx);

    if(recorder->fmt_ctx) {
        if(recorder->fmt_ctx->pb) avio_closep(&recorder->fmt_ctx->pb);
        avformat_free_context(recorder->fmt_ctx);
    }

    if(recorder->pcm_handle) {
        snd_pcm_drop(recorder->pcm_handle);
        snd_pcm_close(recorder->pcm_handle);
    }

    recorder->fmt_ctx    = NULL;
    recorder->codec_ctx  = NULL;
    recorder->stream     = NULL;
    recorder->swr_ctx    = NULL;
    recorder->frame      = NULL;
    recorder->pkt        = NULL;
    recorder->pcm_buf    = NULL;
    recorder->pcm_handle = NULL;

    free(recorder->filename);
    recorder->filename = NULL;
}

static void * recorder_thread_func(void * arg)
{
    recorder_t * recorder = (recorder_t *)arg;

    while(atomic_load(&recorder->state) == RECORDER_RECORDING) {
        snd_pcm_sframes_t frames = snd_pcm_readi(recorder->pcm_handle, recorder->pcm_buf, recorder->frame_size);

        if(frames == -EPIPE) {
            // 采集端缓冲区溢出
            snd_pcm_prepare(recorder->pcm_handle);
            continue;
        }
        if(frames < 0) {
            fprintf(stderr, "[recorder]读取音频失败: %s\n", snd_strerror(frames));
            break;
        }
        if(frames == 0) continue;

        av_frame_make_writable(recorder->frame);

        const uint8_t * in_data[1] = { recorder->pcm_buf };
        int out_samples = swr_convert(recorder->swr_ctx, recorder->frame->data, recorder->frame_size, in_data, frames);
        if(out_samples <= 0) continue;

        recorder->frame->nb_samples = out_samples;
        recorder->frame->pts        = recorder->pts;
        recorder->pts += out_samples;

        int ret = avcodec_send_frame(recorder->codec_ctx, recorder->frame);
        if(ret < 0) {
            fprintf(stderr, "[recorder]编码错误(发送帧)\n");
            break;
        }

        while(ret >= 0) {
            ret = avcodec_receive_packet(recorder->codec_ctx, recorder->pkt);
            if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) break;
            if(ret < 0) {
                fprintf(stderr, "[recorder]编码错误(接收包)\n");
                break;
            }

            av_packet_rescale_ts(recorder->pkt, (AVRational){ 1, recorder->codec_ctx->sample_rate }, recorder->stream->time_base);
            recorder->pkt->stream_index = recorder->stream->index;
            av_interleaved_write_frame(recorder->fmt_ctx, recorder->pkt);
            av_packet_unref(recorder->pkt);
        }
    }

    // 刷新编码器并写入文件尾
    avcodec_send_frame(recorder->codec_ctx, NULL);
    int ret;
    while(1) {
        ret = avcodec_receive_packet(recorder->codec_ctx, recorder->pkt);
        if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) break;
        if(ret < 0) break;

        av_packet_rescale_ts(recorder->pkt, (AVRational){1, recorder->codec_ctx->sample_rate},
                             recorder->stream->time_base);
        recorder->pkt->stream_index = recorder->stream->index;
        av_interleaved_write_frame(recorder->fmt_ctx, recorder->pkt);
        av_packet_unref(recorder->pkt);
    }
    av_write_trailer(recorder->fmt_ctx);

    return NULL;
}
