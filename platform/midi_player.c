/**
 * 一个基于libtimidity的MIDI播放器，同样理论上支持所有linux设备
 * soundfont分支可以使用sf2音源库
 * 这个库也有点冷门，但是体积小且完全符合我的需求
 * 合成效果比wildmidi好很多，也支持连音
 * 感谢DeepSeek提供的珍贵资料
 */

#include "midi_player.h"

#define BUFFER_SIZE 8192
#define CHANNELS 2
#define SAMPLE_RATE 44100
#define CHUNK_SIZE 1024

static void * midi_thread_func(void * arg);

midi_player_t * midi_create(const char * config_file)
{
    midi_player_t * player = malloc(sizeof(midi_player_t));
    if(!player) return NULL;

    memset(player, 0, sizeof(midi_player_t));

    // 初始化互斥锁
    pthread_mutex_init(&player->mutex, NULL);

    // 初始化状态
    player->state = MIDI_STOPPED;
    player->seek_request        = false;
    player->config_file = strdup(config_file);

    return player;
}

int midi_open(midi_player_t * player, const char * filename)
{
    if(!player) return -1;

    pthread_mutex_lock(&player->mutex);

    // 如果已经在播放，直接返回
    if(player->state == MIDI_PLAYING) {
        pthread_mutex_unlock(&player->mutex);
        return -2;
    }

    player->filename = strdup(filename);

    int ret = 0;

    // 初始化
    ret = mid_init(player->config_file);
    if(ret < 0) {
        fprintf(stderr, "[midi_player]初始化timidity错误\n");
        ret = -1;
        goto cleanup;
    }

    // 打开MIDI文件
    player->midi_stream = mid_istream_open_file(filename);
    if(!player->midi_stream) {
        fprintf(stderr, "[midi_player]MIDI文件打开失败\n");
        ret = -2;
        goto cleanup;
    }

    player->song_options.rate        = SAMPLE_RATE;
    player->song_options.format      = MID_AUDIO_S16LSB;
    player->song_options.channels    = CHANNELS;
    player->song_options.buffer_size = BUFFER_SIZE;

    player->song = mid_song_load(player->midi_stream, &player->song_options);
    if(!player->song) {
        fprintf(stderr, "[midi_player]MIDI文件加载失败\n");
        ret = -2;
        goto cleanup;
    }
    mid_istream_close(player->midi_stream);
    player->midi_stream = NULL;
    mid_song_start(player->song);

    pthread_mutex_unlock(&player->mutex);
    return 0;

cleanup:
    pthread_mutex_unlock(&player->mutex);
    midi_stop(player);
    return ret;
}

int midi_init(midi_player_t * player)
{
    if(!player) return -1;
    pthread_mutex_lock(&player->mutex);

    int ret = 0;

    player->sample_rate = SAMPLE_RATE;
    player->channels    = CHANNELS;

    // 打开ALSA设备
    int err;
    if((err = snd_pcm_open(&player->pcm_handle, "default", SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
        fprintf(stderr, "[midi_player]无法打开PCM设备: %s\n", snd_strerror(err));
        ret = -1;
        goto cleanup;
    }

    // 配置PCM参数
    snd_pcm_hw_params_t * hw_params;
    snd_pcm_hw_params_alloca(&hw_params);

    snd_pcm_hw_params_any(player->pcm_handle, hw_params);
    snd_pcm_hw_params_set_access(player->pcm_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(player->pcm_handle, hw_params, SND_PCM_FORMAT_S16_LE);
    snd_pcm_hw_params_set_channels(player->pcm_handle, hw_params, player->channels);
    snd_pcm_hw_params_set_rate_near(player->pcm_handle, hw_params, &player->sample_rate, 0);

    snd_pcm_uframes_t period_size = CHUNK_SIZE;
    snd_pcm_hw_params_set_period_size_near(player->pcm_handle, hw_params, &period_size, 0);

    if((err = snd_pcm_hw_params(player->pcm_handle, hw_params)) < 0) {
        fprintf(stderr, "[midi_player]无法设置硬件参数: %s\n", snd_strerror(err));
        ret = -1;
        goto cleanup;
    }

    // 创建播放线程
    player->state = MIDI_PAUSED;

    if(pthread_create(&player->player_thread, NULL, midi_thread_func, player) != 0) {
        fprintf(stderr, "[midi_player]无法创建播放线程\n");
        ret = -1;
        goto cleanup;
    }

    pthread_mutex_unlock(&player->mutex);
    return 0;

cleanup:
    pthread_mutex_unlock(&player->mutex);
    midi_stop(player);
    return ret;
}

static void * midi_thread_func(void * arg)
{
    midi_player_t * player = (midi_player_t *)arg;

    sint8 * audio_buffer = malloc(BUFFER_SIZE * player->channels * sizeof(int16_t)); // S16LE
    if(!audio_buffer) {
        fprintf(stderr, "[midi_player]无法分配音频缓冲区\n");
        goto cleanup;
    }

    while(1) {
        pthread_mutex_lock(&player->mutex);
        if(player->state == MIDI_STOPPED) {
            pthread_mutex_unlock(&player->mutex);
            break;
        }

        // 检查跳转请求
        if(player->seek_request) {
            if(player->seek_pos != 0) mid_song_seek(player->song, player->seek_pos);
            else mid_song_start(player->song);
            // timidity在结束后不能直接seek，需要重新初始化
            player->seek_request = false;
        }
        // 检查暂停状态
        if(player->state == MIDI_PAUSED) {
            pthread_mutex_unlock(&player->mutex);
            usleep(100000); // 100ms
            continue;
        }

        long bytes_read = mid_song_read_wave(player->song, audio_buffer, BUFFER_SIZE * player->channels * 2);

        // 文件结束或错误
        if(bytes_read <= 0) {
            player->state        = MIDI_PAUSED;
            player->seek_request = true;
            player->seek_pos     = 0;
            pthread_mutex_unlock(&player->mutex);
            if(player->finish_callback_ptr) {
                (*player->finish_callback_ptr)(player);
            }
            continue;
        }
        pthread_mutex_unlock(&player->mutex);

        int frames = bytes_read / 4;

        // 写入PCM设备
        int err = snd_pcm_writei(player->pcm_handle, audio_buffer, frames);

        if(err == -EPIPE) {
            // 缓冲区欠载，尝试恢复
            fprintf(stderr, "[midi_player]缓冲区欠载，正在恢复\n");
            snd_pcm_prepare(player->pcm_handle);

            // 重试写入
            err = snd_pcm_writei(player->pcm_handle, audio_buffer, frames);
            if(err < 0) {
                fprintf(stderr, "[midi_player]恢复失败：%s\n", snd_strerror(err));
                break;
            }
        } else if(err < 0) {
            fprintf(stderr, "[midi_player]写入PCM设备失败：%s\n", snd_strerror(err));
            break;
        }
    }

cleanup:
    if(audio_buffer) free(audio_buffer);

    return NULL;
}

int midi_pause(midi_player_t * player)
{
    if(!player) return -1;

    if(player->state == MIDI_PLAYING) {
        pthread_mutex_lock(&player->mutex);
        player->state = MIDI_PAUSED;
        pthread_mutex_unlock(&player->mutex);
        snd_pcm_pause(player->pcm_handle, 1);
        return 0;
    }
    return -1;
}

int midi_resume(midi_player_t * player)
{
    if(!player) return -1;

    if(player->state == MIDI_PAUSED) {
        pthread_mutex_lock(&player->mutex);
        player->state = MIDI_PLAYING;
        pthread_mutex_unlock(&player->mutex);
        snd_pcm_pause(player->pcm_handle, 0);
        return 0;
    }
    return -1;
}

int midi_stop(midi_player_t * player)
{
    if(!player) return -1;

    pthread_mutex_lock(&player->mutex);
    player->state = MIDI_STOPPED;
    pthread_mutex_unlock(&player->mutex);

    // 等待线程结束
    if(player->player_thread) {
        pthread_join(player->player_thread, NULL);
        player->player_thread = 0;
    }

    if(player->pcm_handle) {
        snd_pcm_drain(player->pcm_handle);
        snd_pcm_close(player->pcm_handle);
        player->pcm_handle = NULL;
    }

    if(player->midi_stream) {
        mid_istream_close(player->midi_stream);
        player->midi_stream = NULL;
    }

    if(player->song) {
        mid_song_free(player->song);
        player->song = NULL;
    }
    mid_exit();

    return 0;
}


//根据百分比跳转
int midi_seek_pct(midi_player_t * player, double percent)
{
    if(!player || !player->song) return -1;

    midi_seek_ms(player, (uint32_t)(midi_get_duration_ms(player) * percent / 100));

    return 0;
}

int midi_seek_ms(midi_player_t * player, uint32_t ms)
{
    if(!player || !player->song) return -1;

    LV_LOG_USER("[midi_player]now=%d, duration=%d\n", ms, midi_get_duration_ms(player));

    pthread_mutex_lock(&player->mutex);
    player->seek_pos     = ms;
    player->seek_request = true;
    pthread_mutex_unlock(&player->mutex);
    return 0;
}

double midi_get_position_pct(midi_player_t * player)
{
    if(!player) return 0.0;
    return (double)midi_get_progress_ms(player) / midi_get_duration_ms(player) * 100.0;
}

uint32_t midi_get_progress_ms(midi_player_t * player)
{
    if(!player || !player->song) return 0;

    pthread_mutex_lock(&player->mutex);
    uint32_t ret = mid_song_get_time(player->song);
    pthread_mutex_unlock(&player->mutex);
    return ret;
}

uint32_t midi_get_duration_ms(midi_player_t * player)
{
    if(!player || !player->song) return 0;
    pthread_mutex_lock(&player->mutex);
    uint32_t ret = mid_song_get_total_time(player->song);
    pthread_mutex_unlock(&player->mutex);
    return ret;
}

midi_state_t midi_get_state(midi_player_t * player)
{
    if(!player) return MIDI_STOPPED;
    pthread_mutex_lock(&player->mutex);
    midi_state_t ret = player->state;
    pthread_mutex_unlock(&player->mutex);
    return ret;
}

void midi_destroy(midi_player_t * player)
{
    if(!player) return;

    midi_stop(player);

    if(player->filename) {
        free(player->filename);
        player->filename = NULL;
    }

    if(player->config_file) {
        free(player->config_file);
        player->config_file = NULL;
    }

    player->finish_callback_ptr = NULL;
    player->user_data            = NULL;

    pthread_mutex_destroy(&player->mutex);
    free(player);
}

void midi_set_finish_callback(midi_player_t * player, void (*func_ptr)(void *), void * user_data)
{
    if(!player) return;
    player->finish_callback_ptr = func_ptr;
    player->user_data           = user_data;
}
