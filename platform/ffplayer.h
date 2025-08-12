#ifndef FF_PLAYER_H
#define FF_PLAYER_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include <stdio.h>
#include <pthread.h>
#include <alsa/asoundlib.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <lvgl/lvgl.h>

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/
// 全局状态结构
typedef struct
{
    // FFmpeg相关
    AVFormatContext * fmt_ctx;
    AVCodecContext *video_dec_ctx, *audio_dec_ctx;
    int video_stream_idx, audio_stream_idx;
    AVFrame * frame;
    AVPacket pkt;

    // ALSA音频
    snd_pcm_t * audio_handle;
    int audio_channels;
    int sample_rate;

    // LVGL视频显示
    lv_obj_t * video_img;
    uint8_t * img_buf;
    struct SwsContext * sws_ctx;

    // 线程控制
    pthread_t video_thread, audio_thread;
    int quit_flag;
} ffplayer;

/**********************
 * GLOBAL PROTOTYPES
 **********************/


/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
