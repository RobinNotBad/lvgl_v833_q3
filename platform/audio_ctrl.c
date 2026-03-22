#include "audio_ctrl.h"

static snd_mixer_t * mixer;
static snd_mixer_elem_t * elem;
static long volume_min, volume_max;
static int volume; // 0-100

void audio_enable(void)
{
    system("echo 1 > /dev/spk_crtl");
}

void audio_disable(void)
{
    system("echo 0 > /dev/spk_crtl");
}

int audio_init(void)
{
    int ret;
    const char * card       = "default";
    const char * selem_name = "LINEOUT volume";

    if((ret = snd_mixer_open(&mixer, 0)) < 0) {
        fprintf(stderr, "[alsa]无法打开mixer: %s\n", snd_strerror(ret));
        goto cleanup;
    }

    if((ret = snd_mixer_attach(mixer, card)) < 0) {
        fprintf(stderr, "[alsa]无法附加mixer到卡: %s\n", snd_strerror(ret));
        goto cleanup;
    }

    if((ret = snd_mixer_selem_register(mixer, NULL, NULL)) < 0) {
        fprintf(stderr, "[alsa]无法注册mixer: %s\n", snd_strerror(ret));
        goto cleanup;
    }

    if((ret = snd_mixer_load(mixer)) < 0) {
        fprintf(stderr, "[alsa]无法加载mixer: %s\n", snd_strerror(ret));
        goto cleanup;
    }

    // 查找音量控制元素
    {
        snd_mixer_selem_id_t * sid;
        snd_mixer_selem_id_alloca(&sid);
        snd_mixer_selem_id_set_index(sid, 0);
        snd_mixer_selem_id_set_name(sid, selem_name);

        elem = snd_mixer_find_selem(mixer, sid);
    }  // 防止栈分配问题，虽然不知道有没有用，编译器也还是会警告
    
    if(!elem) {
        fprintf(stderr, "[alsa]无法找到音量控制元素 '%s'\n", selem_name);
        goto cleanup;
    }

    // 获取音量范围
    snd_mixer_selem_get_playback_volume_range(elem, &volume_min, &volume_max);

    // 获取实际音量并计算百分比
    long actual_volume;
    snd_mixer_selem_get_playback_volume(elem, 0, &actual_volume);
    volume = 100 * (actual_volume - volume_min) / (volume_max - volume_min);

    printf("[alsa]音量: %d (%ld, %ld-%ld)\n", volume, actual_volume, volume_min, volume_max);
    return 0;

cleanup:
    snd_mixer_close(mixer);
    mixer = NULL;
    return -1;
}

int audio_volume_set(int percent)
{
    if(!elem) audio_init();

    if(percent < 0) percent = 0;
    if(percent > 100) percent = 100;

    volume = percent;

    // 将百分比转换为实际音量值
    long actual_volume = volume_min + (percent * (volume_max - volume_min)) / 100;

    int ret = snd_mixer_selem_set_playback_volume_all(elem, actual_volume);
    if(ret < 0) {
        fprintf(stderr, "[alsa]设置音量失败: %s\n", snd_strerror(ret));
        return -1;
    }

    return 0;
}

int audio_volume_get(void)
{
    if(!elem) audio_init();
    return volume;
}

void audio_close(void)
{
    if(mixer) {
        snd_mixer_close(mixer);
        mixer = NULL;
        elem  = NULL;
    }
}
