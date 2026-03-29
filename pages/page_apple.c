#include "page_apple.h"

#include "main.h"

typedef struct
{
    BasePage base;
    bool ui_hidden;
    lv_obj_t * btn_control;
    lv_obj_t * btn_control_label;
    ff_player_t * player;
    lv_obj_t * slider_volume;
    lv_obj_t * slider_progress;
    lv_obj_t * btn_back;
    lv_timer_t * timer;
} VideoPage;

static lv_obj_t * page_video_obj(VideoPage * page, char * filename);
static void back_click(lv_event_t * e);
static void slider_progress_released(lv_event_t * e);
static void slider_volume_changed(lv_event_t * e);
static void touch_clicked(lv_event_t * e);
static void control_click(lv_event_t * e);
static void timer_tick(lv_timer_t * e);
static void player_finished(void * p);
static void page_video_destroy(void * p);

BasePage * page_video_create(char * filename)
{
    VideoPage * page = malloc(sizeof(VideoPage));
    if(!page) return NULL;
    memset(page, 0, sizeof(VideoPage));

    page->base.obj        = page_video_obj(page, filename);
    page->base.on_destroy = page_video_destroy;
    return page;
}

static lv_obj_t * page_video_obj(VideoPage * page, char * filename)
{
    lv_obj_t * screen = lv_obj_create(lv_scr_act());
    lv_obj_remove_style_all(screen);
    lv_obj_set_size(screen, lv_pct(100), lv_pct(100));

    sys_set_dont_deep_sleep(true);
    sys_set_dont_timeout(true);
    audio_enable();

    lv_img_header_t header;
    ffmpeg_get_img_header(filename, &header);
    lv_obj_t * img = lv_img_create(screen);
    double scale_default  = fmin((double)LV_SCR_WIDTH / header.w, (double)LV_SCR_HEIGHT / header.h);
    lv_coord_t img_scaled_w   = (lv_coord_t)(header.w * scale_default);
    lv_coord_t img_scaled_h   = (lv_coord_t)(header.h * scale_default);
    lv_obj_set_size(img, img_scaled_w, img_scaled_h);
    lv_obj_center(img);

    page->player = player_create();
    if(player_open(page->player, filename) == 0 && player_init_audio(page->player) == 0 &&
       player_init_video(page->player, img) == 0) {
        player_resume(page->player);
        player_set_finish_callback(page->player, player_finished, page);
    } else
        page->player = NULL;

    lv_obj_t * touch_area = lv_obj_create(screen);
    lv_obj_set_size(touch_area, LV_SCR_WIDTH, LV_SCR_HEIGHT);
    lv_obj_set_pos(touch_area, 0, 0);
    lv_obj_set_style_bg_opa(touch_area, LV_OPA_0, 0);
    lv_obj_set_style_border_opa(touch_area, LV_OPA_0, 0);
    lv_obj_set_style_outline_opa(touch_area, LV_OPA_0, 0);
    lv_obj_set_style_shadow_opa(touch_area, LV_OPA_0, 0);
    lv_obj_add_event_cb(touch_area, touch_clicked, LV_EVENT_CLICKED, page);

    page->slider_progress = lv_slider_create(screen);
    lv_obj_set_size(page->slider_progress, lv_pct(90), lv_pct(10));
    lv_obj_align(page->slider_progress, LV_ALIGN_TOP_MID, 0, lv_pct(90));
    lv_slider_set_range(page->slider_progress, 0, 100);
    lv_obj_add_event_cb(page->slider_progress, slider_progress_released, LV_EVENT_RELEASED, page);

    page->slider_volume = lv_slider_create(screen);
    lv_obj_set_size(page->slider_volume, lv_pct(10), lv_pct(60));
    lv_obj_align(page->slider_volume, LV_ALIGN_LEFT_MID, lv_pct(90), 0);
    lv_slider_set_range(page->slider_volume, 0, 100);
    lv_slider_set_value(page->slider_volume, audio_volume_get(), LV_ANIM_OFF);
    lv_obj_add_event_cb(page->slider_volume, slider_volume_changed, LV_EVENT_VALUE_CHANGED, NULL);

    page->btn_control = lv_btn_create(screen);
    lv_obj_set_size(page->btn_control, lv_pct(25), lv_pct(25));
    lv_obj_align(page->btn_control, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_opa(page->btn_control, LV_OPA_50, 0);
    lv_obj_set_style_border_opa(page->btn_control, LV_OPA_50, 0);
    lv_obj_set_style_outline_opa(page->btn_control, LV_OPA_50, 0);
    lv_obj_set_style_shadow_opa(page->btn_control, LV_OPA_50, 0);
    page->btn_control_label = lv_label_create(page->btn_control);
    lv_label_set_text(page->btn_control_label, LV_SYMBOL_PAUSE "");
    lv_obj_center(page->btn_control_label);
    lv_obj_add_event_cb(page->btn_control, control_click, LV_EVENT_CLICKED, page);

    page->btn_back = lv_btn_create(screen);
    lv_obj_set_size(page->btn_back, lv_pct(25), lv_pct(12));
    lv_obj_align(page->btn_back, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_t * btn_back_label = lv_label_create(page->btn_back);
    lv_label_set_text(btn_back_label, CUSTOM_SYMBOL_BACK "");
    lv_obj_center(btn_back_label);
    lv_obj_add_event_cb(page->btn_back, back_click, LV_EVENT_CLICKED, NULL);

    page->timer = lv_timer_create(timer_tick, 250, page);

    return screen;
}

static void control_click(lv_event_t * e)
{
    VideoPage * page = (VideoPage *)e->user_data;
    if(!page || !page->player) return;

    player_state_t state = player_get_state(page->player);
    if(state == PLAYER_PAUSED) {
        audio_enable();
        player_resume(page->player);
        lv_label_set_text(page->btn_control_label, LV_SYMBOL_PAUSE "");
    }
    if(state == PLAYER_PLAYING) {
        player_pause(page->player);
        lv_label_set_text(page->btn_control_label, LV_SYMBOL_PLAY "");
        audio_disable();
    }
}

static void player_finished(void * p)
{
    ff_player_t * player = (ff_player_t *)p;
    VideoPage * page = (VideoPage *)player->user_data;
    if(!page) return;
    lv_label_set_text(page->btn_control_label, LV_SYMBOL_PLAY "");
    audio_disable();
}

static void slider_volume_changed(lv_event_t * e)
{
    lv_obj_t * slider = lv_event_get_target(e);
    int value         = lv_slider_get_value(slider);
    
    audio_volume_set(value);
}

static void slider_progress_released(lv_event_t * e)
{
    VideoPage * page = (VideoPage *)e->user_data;
    if(!page || !page->player) return;
    
    player_state_t state = player_get_state(page->player);
    if(state == PLAYER_PLAYING || state == PLAYER_PAUSED) {
        lv_obj_t * slider = lv_event_get_target(e);
        int value         = lv_slider_get_value(slider);
        player_seek_pct(page->player, value);
    }
}

static void touch_clicked(lv_event_t * e) 
{
    VideoPage * page = (VideoPage *)e->user_data;
    if(!page) return;

    page->ui_hidden = !page->ui_hidden;
    if(page->ui_hidden) {
        lv_obj_add_flag(page->slider_volume, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(page->slider_progress, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(page->btn_control, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(page->btn_back, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_clear_flag(page->slider_volume, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(page->slider_progress, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(page->btn_control, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(page->btn_back, LV_OBJ_FLAG_HIDDEN);
    }
}

static void timer_tick(lv_timer_t * e)
{
    VideoPage * page = (VideoPage *)e->user_data;
    if(!page || !page->player) return;
    lv_slider_set_value(page->slider_progress, player_get_position_pct(page->player), LV_ANIM_OFF);
}

static void back_click(lv_event_t * e)
{
    page_back();
}

static void page_video_destroy(void * p)
{
    VideoPage * page = (VideoPage *)p;
    if(!page) return;

    if(page->timer) lv_timer_del(page->timer);
    if(page->player) player_destroy(page->player);
    page->player = NULL;
    audio_disable();
    sys_set_dont_deep_sleep(false);
    sys_set_dont_timeout(false);
}