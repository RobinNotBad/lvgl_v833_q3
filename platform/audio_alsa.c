#include "audio_alsa.h"

snd_pcm_t * pcm_handle;

int audio_init(void)
{
    if(pcm_handle) return 0;
    printf("[audio]init");

    system("echo 1 > /dev/spk_crtl");
    snd_pcm_open(&pcm_handle, "hw:0,0", SND_PCM_STREAM_PLAYBACK, 0);
    snd_pcm_hw_params_t * params;
    snd_pcm_hw_params_alloca(&params);

    // 获取默认参数
    snd_pcm_hw_params_any(pcm_handle, params);

    // 设置参数
    snd_pcm_hw_params_set_access(pcm_handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(pcm_handle, params, SND_PCM_FORMAT_S16_LE);
    snd_pcm_hw_params_set_channels(pcm_handle, params, 1);

    unsigned int rate = 44100;
    snd_pcm_hw_params_set_rate_near(pcm_handle, params, &rate, 0);

    // 设置缓冲区时间
    unsigned int buffer_time = 500000;
    snd_pcm_hw_params_set_buffer_time_near(pcm_handle, params, &buffer_time, 0);

    // 设置周期时间
    unsigned int period_time = 100000;
    snd_pcm_hw_params_set_period_time_near(pcm_handle, params, &period_time, 0);

    // 应用参数
    if(snd_pcm_hw_params(pcm_handle, params) < 0) {
        snd_pcm_close(pcm_handle);
        pcm_handle = NULL;
        return -1;
    }

    return 0;
}

int audio_release(void)
{
    if(pcm_handle) {
    printf("[audio]close");
        snd_pcm_drain(pcm_handle);
        snd_pcm_close(pcm_handle);
        pcm_handle = NULL;
        system("echo 0 > /dev/spk_crtl");
        return 0;
    }
    return -1;
}

int audio_write(void * data, size_t size)
{
    if(!pcm_handle) return -1;

    const uint8_t * p       = data;
    size_t remaining_frames = size / sizeof(int16_t);

    while(remaining_frames > 0) {
        snd_pcm_sframes_t frames = snd_pcm_writei(pcm_handle, p, remaining_frames);

        if(frames < 0) {
            // 尝试恢复错误
            frames = snd_pcm_recover(pcm_handle, frames, 0);

            if(frames < 0) {
                // 严重错误，重新初始化设备
                audio_release();
                if(audio_init() < 0) {
                    return -1;
                }
                continue;
            }
        }

        p += frames * sizeof(int16_t);
        remaining_frames -= frames;

        /*
        // 添加小延迟避免忙等待
        if(remaining_frames > 0) {
            usleep(1000); // 1ms
        }
        */
    }

    return 0;
}

int audio_volume(int pct)
{
    snd_mixer_t * mixer;
    snd_mixer_open(&mixer, 0);
    snd_mixer_attach(mixer, "default");

    snd_mixer_elem_t * elem;

    elem = snd_mixer_first_elem(mixer);
    while(elem)
    {
        if(strcmp("LINEOUT volume", snd_mixer_selem_get_name(elem)) == 0) break;
        elem = snd_mixer_elem_next(elem);
    }

    if(!elem) {
        snd_mixer_close(elem);
        elem = NULL;
        return -1;
    }

    long min, max;
    snd_mixer_selem_get_playback_volume_range(elem, &min, &max);

    long volume = (max - min) * pct / 100 + min;

    snd_mixer_handle_events(mixer);
    snd_mixer_selem_set_playback_volume_all(elem, volume);
    snd_mixer_selem_set_playback_switch_all(elem, 1);
    printf("[audio]volume=%ld", volume);
    snd_mixer_close(mixer);
}