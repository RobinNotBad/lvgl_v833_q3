#include "page_audio.h"

typedef struct
{
    BasePage base;
    lv_obj_t * btn_control_label;
    lv_obj_t * btn_cycle_label;
    lv_obj_t * slider_progress;
    lv_obj_t * slider_volume;
    ff_player_t * player;
    lv_timer_t * timer;
    bool cycle;
    lv_obj_t * label_title;
    lv_obj_t * label_battery;
} AudioPage;

static lv_obj_t * page_audio_obj(AudioPage * page, char * filename);
static void back_click(lv_event_t * e);
static void cycle_click(lv_event_t * e);
static void control_click(lv_event_t * e);
static void timer_tick(lv_timer_t * e);
static void slider_progress_changed(lv_event_t * e);
static void slider_progress_released(lv_event_t * e);
static void slider_volume_changed(lv_event_t * e);
static void slider_volume_released(lv_event_t * e);
static void player_finished(void * p);
static void page_audio_destroy(void * p);

BasePage * page_audio_create(char * filename)
{
    AudioPage * page = malloc(sizeof(AudioPage));
    if(!page) return NULL;
    memset(page, 0, sizeof(AudioPage));

    page->base.obj        = page_audio_obj(page, filename);
    page->base.on_destroy = page_audio_destroy;
    return (BasePage *)page;
}

static lv_obj_t * page_audio_obj(AudioPage * page, char * filename)
{
    lv_obj_t * screen = lv_obj_create(lv_scr_act());
    lv_obj_remove_style_all(screen);
    lv_obj_set_size(screen, lv_pct(100), lv_pct(100));

    sys_set_dont_deep_sleep(true);
    page->cycle = false;
    audio_enable();

    ff_player_t * player = player_create();
    if(player_open(player, filename) == 0 && player_init_audio(player) == 0) {
        player_resume(player);
        player_set_finish_callback(player, player_finished, page);
        page->player = player;
    } else {
        player = NULL;
    }

    lv_obj_t * clock = lv_text_clock_create(screen);
    lv_obj_align(clock, LV_ALIGN_TOP_LEFT, lv_pct(5), lv_pct(2));

    lv_obj_t * label_battery = lv_label_create(screen);
    lv_obj_align(label_battery, LV_ALIGN_TOP_RIGHT, lv_pct(-5), lv_pct(2));
    uint8_t cap = battery_get_capacity();
    lv_label_set_text_fmt(label_battery, "%d%% ", cap);
    page->label_battery = label_battery;

    lv_obj_t * label_title = lv_label_create(screen);
    lv_obj_set_width(label_title, lv_pct(85));
    lv_label_set_long_mode(label_title, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_style_text_align(label_title, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(label_title, LV_ALIGN_TOP_MID, 0, lv_pct(15));

    // 用ffmpeg获取一下文件元数据
    const char * title = NULL;
    if (player && player->format_ctx) {
        AVDictionaryEntry *tag = av_dict_get(player->format_ctx->metadata, "title", NULL, 0);
        if (tag && tag->value && strlen(tag->value) > 0) {
            title = tag->value;
        }
    }
    if (title) {
        // 有元数据，显示曲名
        lv_label_set_text(label_title, title);
    } else {
        // 啥也没有，显示文件名
        char * base = strrchr(filename, '/');
        if (base)    base++;                // 有斜杠，去掉斜杠
        else         base = filename;       // 没斜杠，直接用原名
        lv_label_set_text(label_title, base);
    }
    page->label_title = label_title;

    lv_obj_t * slider_progress = lv_slider_create(screen);
    lv_obj_set_size(slider_progress, lv_pct(80), lv_pct(8));
    lv_obj_align(slider_progress, LV_ALIGN_TOP_MID, 0, lv_pct(32));
    lv_slider_set_range(slider_progress, 0, 100);
    lv_obj_add_event_cb(slider_progress, slider_progress_changed, LV_EVENT_VALUE_CHANGED, page);
    lv_obj_add_event_cb(slider_progress, slider_progress_released, LV_EVENT_RELEASED, page);
    page->slider_progress = slider_progress;

    lv_obj_t * slider_volume = lv_slider_create(screen);
    lv_obj_set_size(slider_volume, lv_pct(80), lv_pct(8));
    lv_obj_align(slider_volume, LV_ALIGN_TOP_MID, 0, lv_pct(48));
    lv_slider_set_range(slider_volume, 0, 100);
    lv_slider_set_value(slider_volume, audio_volume_get(), LV_ANIM_OFF);
    lv_obj_add_event_cb(slider_volume, slider_volume_changed, LV_EVENT_VALUE_CHANGED, page);
    lv_obj_add_event_cb(slider_volume, slider_volume_released, LV_EVENT_RELEASED, page);
    page->slider_volume = slider_volume;

    lv_obj_t * btn_control = lv_btn_create(screen);
    lv_obj_set_size(btn_control, lv_pct(40), lv_pct(20));
    lv_obj_align(btn_control, LV_ALIGN_TOP_MID, 0, lv_pct(64));
    lv_obj_t * btn_control_label = lv_label_create(btn_control);
    lv_label_set_text(btn_control_label, LV_SYMBOL_PAUSE "");
    lv_obj_center(btn_control_label);
    lv_obj_add_event_cb(btn_control, control_click, LV_EVENT_CLICKED, page);
    page->btn_control_label = btn_control_label;

    lv_obj_t * btn_back = lv_btn_create(screen);
    lv_obj_set_size(btn_back, lv_pct(25), lv_pct(12));
    lv_obj_align(btn_back, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    lv_obj_t * btn_back_label = lv_label_create(btn_back);
    lv_label_set_text(btn_back_label, CUSTOM_SYMBOL_BACK "");
    lv_obj_center(btn_back_label);
    lv_obj_add_event_cb(btn_back, back_click, LV_EVENT_CLICKED, page);

    lv_obj_t * btn_cycle = lv_btn_create(screen);
    lv_obj_set_size(btn_cycle, lv_pct(25), lv_pct(12));
    lv_obj_align(btn_cycle, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
    lv_obj_t * btn_cycle_label = lv_label_create(btn_cycle);
    lv_label_set_text(btn_cycle_label, CUSTOM_SYMBOL_BAN "");
    lv_obj_center(btn_cycle_label);
    lv_obj_add_event_cb(btn_cycle, cycle_click, LV_EVENT_CLICKED, page);
    page->btn_cycle_label = btn_cycle_label;

    // 电量的定时器用timer_tick的（
    page->timer = lv_timer_create(timer_tick, 250, page);

    return screen;
}

static void cycle_click(lv_event_t * e)
{
    AudioPage * page = (AudioPage *)e->user_data;
    if(!page) return;
    page->cycle = !page->cycle;
    if(page->cycle)
        lv_label_set_text(page->btn_cycle_label, CUSTOM_SYMBOL_CYCLE "");
    else
        lv_label_set_text(page->btn_cycle_label, CUSTOM_SYMBOL_BAN "");
}

static void control_click(lv_event_t * e)
{
    AudioPage * page = (AudioPage *)e->user_data;
    if(!page) return;
    ff_player_t * player = page->player;
    lv_obj_t * btn_control_label = page->btn_control_label;
    if(!player || !btn_control_label) return;

    player_state_t state = player_get_state(player);
    if(state == PLAYER_PAUSED) {
        audio_enable();
        player_resume(player);
        lv_label_set_text(btn_control_label, LV_SYMBOL_PAUSE "");
    }
    else if(state == PLAYER_PLAYING) {
        player_pause(player);
        lv_label_set_text(btn_control_label, LV_SYMBOL_PLAY "");
        audio_disable();
    }
}

static void slider_progress_changed(lv_event_t * e) {}

static void slider_progress_released(lv_event_t * e)
{
    AudioPage * page = (AudioPage *)e->user_data;
    ff_player_t * player = page->player;
    if(!player) return;
    player_state_t state = player_get_state(player);
    if(state == PLAYER_PLAYING || state == PLAYER_PAUSED){
        lv_obj_t * slider = lv_event_get_target(e);
        int value = lv_slider_get_value(slider);
        player_seek_pct(player, value);
    }
}

static void slider_volume_changed(lv_event_t * e)
{
    lv_obj_t * slider = lv_event_get_target(e);
    int value = lv_slider_get_value(slider);
    audio_volume_set(value);
}

static void slider_volume_released(lv_event_t * e)
{
    // 可留空
}

static void timer_tick(lv_timer_t * e)
{
    AudioPage * page = (AudioPage *)e->user_data;
    if(!page) return;

    // 更新进度条
    if(page->player) {
        lv_slider_set_value(page->slider_progress, player_get_position_pct(page->player), LV_ANIM_OFF);
    }

    // 更新电量
    if(page->label_battery) {
        uint8_t cap = battery_get_capacity();
        lv_label_set_text_fmt(page->label_battery, "%d%%", cap);
    }
}

static void player_finished(void * p)
{
    ff_player_t * player = (ff_player_t *)p;
    AudioPage * page = (AudioPage *)player->user_data;
    if(page->cycle) {
        player_seek_ms(player, 0);
        player_resume(player);
    }
    else {
        lv_label_set_text(page->btn_control_label, LV_SYMBOL_PLAY "");
        audio_disable();
    }
}

static void back_click(lv_event_t * e)
{
    page_back();
}

static void page_audio_destroy(void * p)
{
    AudioPage * page = (AudioPage *)p;
    if(page->timer) lv_timer_del(page->timer);
    if(page->player) player_destroy(page->player);
    page->player = NULL;
    audio_disable();
    sys_set_dont_deep_sleep(false);
}