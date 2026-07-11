#ifndef MIDI_PLAYER_H
#define MIDI_PLAYER_H

#include "../lvgl/lvgl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#include "timidity.h"
#include <alsa/asoundlib.h>

typedef enum { MIDI_STOPPED, MIDI_PLAYING, MIDI_PAUSED } midi_state_t;

typedef struct
{
    // WildMidi相关
    MidIStream * midi_stream;
    MidSongOptions song_options;
    MidSong * song;

    // ALSA 相关
    snd_pcm_t * pcm_handle;
    snd_pcm_uframes_t frames;
    unsigned int sample_rate;
    int channels;

    // 播放控制
    volatile int state;
    volatile bool seek_request;
    volatile uint32_t seek_pos;
    pthread_t player_thread;
    pthread_mutex_t mutex;

    char * filename;
    char * config_file;

    void * user_data;
    void (*finish_callback_ptr)(void *);
} midi_player_t;

// 函数声明
midi_player_t * midi_create(const char * config_file);
int midi_open(midi_player_t * player, const char * filename);
int midi_init(midi_player_t * player);
int midi_pause(midi_player_t * player);
int midi_resume(midi_player_t * player);
int midi_stop(midi_player_t * player);
int midi_seek_pct(midi_player_t * player, double percent);
uint32_t midi_get_duration_ms(midi_player_t * player);
int midi_seek_ms(midi_player_t * player, uint32_t ms);
double midi_get_position_pct(midi_player_t * player);
uint32_t midi_get_progress_ms(midi_player_t * player);
midi_state_t midi_get_state(midi_player_t * player);
void midi_destroy(midi_player_t * player);

// 状态变化回调
void midi_set_finish_callback(midi_player_t * player, void (*func_ptr)(void *), void * user_data);

#endif