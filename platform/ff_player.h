#ifndef FF_PLAYER_H
#define FF_PLAYER_H

#include "../lvgl/lvgl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <stdatomic.h>

// FFmpeg 头文件
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
#include <libavutil/samplefmt.h>
#include <libavutil/timestamp.h>
#include <libswscale/swscale.h>

// ALSA 头文件
#include <alsa/asoundlib.h>

typedef enum { PLAYER_STOPPED, PLAYER_PLAYING, PLAYER_PAUSED } player_state_t;

typedef struct
{
    // FFmpeg 相关
    AVFormatContext * format_ctx;
    AVCodecContext * audio_codec_ctx;
    AVCodecContext * video_codec_ctx;
    SwrContext * swr_ctx;
    struct SwsContext * sws_ctx;
    int audio_stream_index;
    int video_stream_index;

    // ALSA 相关
    snd_pcm_t * pcm_handle;

    // 显示相关
    uint8_t * video_src_data[4];    // 图像数据的指针
    uint8_t * video_dst_data[4];    // 缩放后的图像数据的指针
    int video_src_linesize[4];
    int video_dst_linesize[4];
    enum AVPixelFormat video_dst_pix_fmt;
    lv_obj_t * video_area;
    lv_img_dsc_t img_dsc;

    // 播放控制
    volatile int state;
    volatile bool seek_request;
    volatile int64_t seek_pos;
    pthread_t player_thread;
    pthread_mutex_t mutex;

    // 进度信息
    volatile int64_t current_pts;
    AVRational time_base;
    int64_t duration;

    char * filename;

    void * user_data;
    void (*finish_callback_ptr)(void *);
} ff_player_t;

// 函数声明
ff_player_t * player_create(void);
int player_open(ff_player_t * player, const char * filename);
int player_init_audio(ff_player_t * player);
int player_init_video(ff_player_t * player, lv_obj_t * lv_obj);
int player_pause(ff_player_t * player);
int player_resume(ff_player_t * player);
int player_stop(ff_player_t * player);
int player_seek_pct(ff_player_t * player, double percent);
int player_seek_ms(ff_player_t * player, int64_t target_ms);
int64_t player_get_position_ms(ff_player_t * player);
int64_t player_get_duration_ms(ff_player_t * player);
double player_get_position_pct(ff_player_t * player);
player_state_t player_get_state(ff_player_t * player);
void player_destroy(ff_player_t * player);

// 状态变化回调
void player_set_finish_callback(ff_player_t * player, void (*func_ptr)(void *), void * user_data);

#endif