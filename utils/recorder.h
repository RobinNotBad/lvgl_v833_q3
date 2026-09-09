#ifndef RECORDER_H
#define RECORDER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdint.h>

// FFmpeg 头文件
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
#include <libavutil/frame.h>

// ALSA 头文件
#include <alsa/asoundlib.h>

typedef enum { RECORDER_STOPPED, RECORDER_RECORDING } recorder_state_t;

typedef struct
{
    // 控制
    atomic_int state;
    pthread_t thread;

    // 输出文件名
    char * filename;

    // ALSA 采集设备
    snd_pcm_t * pcm_handle;

    // FFmpeg 编码相关
    AVFormatContext * fmt_ctx;
    AVCodecContext * codec_ctx;
    AVStream * stream;
    SwrContext * swr_ctx;
    AVFrame * frame;
    AVPacket * pkt;
    uint8_t * pcm_buf;
    int frame_size;
    int64_t pts;
} recorder_t;

recorder_t * recorder_init(void);
int recorder_start(recorder_t * recorder, const char * file_name);
int recorder_stop(recorder_t * recorder);
void recorder_destroy(recorder_t * recorder);

#endif
