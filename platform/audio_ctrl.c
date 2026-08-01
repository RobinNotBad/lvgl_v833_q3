#include "audio_ctrl.h"

static snd_mixer_t * mixer;
static snd_mixer_elem_t * elem_volume;
static long volume_min, volume_max;
static int volume; // 0-100

static int set_enum(const char *name, unsigned int index);
static int set_switch(const char * name, int on);
static int set_volume(const char *name, long value);

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

    set_switch("External Speaker", 1);
    set_switch("Left Input Mixer MIC1 Boost", 1);
    set_enum("Left LINEOUT Mux", 1);
    
    // 查找音量控制元素
    {
        snd_mixer_selem_id_t * sid;
        snd_mixer_selem_id_alloca(&sid);
        snd_mixer_selem_id_set_index(sid, 0);
        snd_mixer_selem_id_set_name(sid, selem_name);

        elem_volume = snd_mixer_find_selem(mixer, sid);
    }  // 防止栈分配问题，虽然不知道有没有用，编译器也还是会警告
    
    if(!elem_volume) {
        fprintf(stderr, "[alsa]无法找到音量控制元素 '%s'\n", selem_name);
        goto cleanup;
    }

    // 获取音量范围
    snd_mixer_selem_get_playback_volume_range(elem_volume, &volume_min, &volume_max);

    // 获取实际音量并计算百分比
    long actual_volume;
    snd_mixer_selem_get_playback_volume(elem_volume, 0, &actual_volume);
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
    if(!elem_volume) audio_init();

    if(percent < 0) percent = 0;
    if(percent > 100) percent = 100;

    volume = percent;

    // 将百分比转换为实际音量值
    long actual_volume = volume_min + (percent * (volume_max - volume_min)) / 100;

    int ret = snd_mixer_selem_set_playback_volume_all(elem_volume, actual_volume);
    if(ret < 0) {
        fprintf(stderr, "[alsa]设置音量失败: %s\n", snd_strerror(ret));
        return -1;
    }

    return 0;
}

int audio_volume_get(void)
{
    if(!elem_volume) audio_init();
    return volume;
}

void audio_close(void)
{
    if(mixer) {
        snd_mixer_close(mixer);
        mixer = NULL;
        elem_volume  = NULL;
    }
}


// 辅助函数：设置播放开关（on=1, off=0）
static int set_switch(const char *name, int on) {
    snd_mixer_selem_id_t *sid;
    snd_mixer_elem_t *elem;
    int err;

    snd_mixer_selem_id_alloca(&sid);
    snd_mixer_selem_id_set_name(sid, name);
    snd_mixer_selem_id_set_index(sid, 0);  // 索引为0

    elem = snd_mixer_find_selem(mixer, sid);
    if (!elem) {
        fprintf(stderr, "控件 '%s' 未找到\n", name);
        return -ENOENT;
    }

    err = snd_mixer_selem_set_playback_switch_all(elem, on);
    if (err < 0) {
        fprintf(stderr, "设置开关 '%s' 失败: %s\n", name, snd_strerror(err));
        return err;
    }
    return 0;
}

// 辅助函数：设置播放音量（范围由控件决定）
static int set_volume(const char *name, long value) {
    snd_mixer_selem_id_t *sid;
    snd_mixer_elem_t *elem;
    int err;

    snd_mixer_selem_id_alloca(&sid);
    snd_mixer_selem_id_set_name(sid, name);
    snd_mixer_selem_id_set_index(sid, 0);

    elem = snd_mixer_find_selem(mixer, sid);
    if (!elem) {
        fprintf(stderr, "控件 '%s' 未找到\n", name);
        return -ENOENT;
    }

    // 检查控件是否支持播放音量
    if (!snd_mixer_selem_has_playback_volume(elem)) {
        fprintf(stderr, "控件 '%s' 不支持播放音量\n", name);
        return -ENOTSUP;
    }

    long min, max;
    snd_mixer_selem_get_playback_volume_range(elem, &min, &max);
    if (value < min || value > max) {
        fprintf(stderr, "音量值 %ld 超出范围 [%ld, %ld]\n", value, min, max);
        return -EINVAL;
    }

    err = snd_mixer_selem_set_playback_volume_all(elem, value);
    if (err < 0) {
        fprintf(stderr, "设置音量 '%s' 失败: %s\n", name, snd_strerror(err));
        return err;
    }
    return 0;
}

// 辅助函数：设置枚举项（按索引）
static int set_enum(const char *name, unsigned int index) {
    snd_mixer_selem_id_t *sid;
    snd_mixer_elem_t *elem;
    int err;

    snd_mixer_selem_id_alloca(&sid);
    snd_mixer_selem_id_set_name(sid, name);
    snd_mixer_selem_id_set_index(sid, 0);

    elem = snd_mixer_find_selem(mixer, sid);
    if (!elem) {
        fprintf(stderr, "控件 '%s' 未找到\n", name);
        return -ENOENT;
    }

    // 获取枚举项数量（仅用于调试）
    unsigned int items = snd_mixer_selem_get_enum_items(elem);
    if (index >= items) {
        fprintf(stderr, "枚举索引 %u 超出范围 (共 %u 项)\n", index, items);
        return -EINVAL;
    }

    // 设置枚举项（播放方向）
    err = snd_mixer_selem_set_enum_item(elem, SND_MIXER_SCHN_MONO, index);
    if (err < 0) {
        fprintf(stderr, "设置枚举 '%s' 失败: %s\n", name, snd_strerror(err));
        return err;
    }
    return 0;
}
