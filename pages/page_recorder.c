#include "page_recorder.h"

#include "main.h"
#include "recorder.h"
#include "dendro_conf.h"

#include <time.h>
#include <sys/stat.h>

typedef struct
{
    BasePage base;
    recorder_t * recorder;
    lv_obj_t * btn_control;
    lv_obj_t * btn_control_label;
    lv_obj_t * label_status;
    lv_obj_t * label_filename;
    lv_timer_t * timer;
    bool recording;
    uint64_t home_down_time;
    time_t start_time;
    char dir[PATH_MAX_LENGTH];
    char filename[PATH_MAX_LENGTH];
} RecorderPage;

static lv_obj_t * page_recorder_obj(RecorderPage * page);
static void back_click(lv_event_t * e);
static void control_click(lv_event_t * e);
static void timer_tick(lv_timer_t * e);
static void page_recorder_destroy(void * p);
static bool page_recorder_on_key(void * p, key_code_t key_code, key_action_t key_action);

BasePage * page_recorder_create(void)
{
    RecorderPage * page = malloc(sizeof(RecorderPage));
    if(!page) return NULL;
    memset(page, 0, sizeof(RecorderPage));

    page->base.obj        = page_recorder_obj(page);
    page->base.on_destroy = page_recorder_destroy;
    page->base.on_key     = page_recorder_on_key;
    return (BasePage *)page;
}

static lv_obj_t * page_recorder_obj(RecorderPage * page)
{
    lv_obj_t * screen = lv_obj_create(lv_scr_act());
    lv_obj_remove_style_all(screen);
    lv_obj_set_size(screen, lv_pct(100), lv_pct(100));

    sys_set_dont_deep_sleep(true);

    page->recorder = recorder_init();

    snprintf(page->dir, sizeof(page->dir), "/mnt/UDISK/recorder");

    lv_obj_t * label_title = lv_label_create(screen);
    lv_obj_align(label_title, LV_ALIGN_TOP_MID, 0, lv_pct(4));
    lv_label_set_text(label_title, "Recorder");

    lv_obj_t * label_status = lv_label_create(screen);
    lv_obj_align(label_status, LV_ALIGN_TOP_MID, 0, lv_pct(24));
    if (page->recorder) lv_label_set_text(label_status, "Ready");
    else lv_label_set_text(label_status, "Error");
    page->label_status = label_status;

    lv_obj_t * label_filename = lv_label_create(screen);
    lv_obj_set_width(label_filename, lv_pct(85));
    lv_obj_align(label_filename, LV_ALIGN_TOP_MID, 0, lv_pct(38));
    lv_label_set_long_mode(label_filename, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_style_text_align(label_filename, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(label_filename, "Click the button to start");
    page->label_filename = label_filename;

    lv_obj_t * btn_control = lv_btn_create(screen);
    lv_obj_set_size(btn_control, lv_pct(40), lv_pct(20));
    lv_obj_align(btn_control, LV_ALIGN_TOP_MID, 0, lv_pct(56));
    lv_obj_t * btn_control_label = lv_label_create(btn_control);
    lv_label_set_text(btn_control_label, LV_SYMBOL_PLAY "");
    lv_obj_center(btn_control_label);
    lv_obj_add_event_cb(btn_control, control_click, LV_EVENT_CLICKED, page);
    page->btn_control = btn_control;
    page->btn_control_label = btn_control_label;

    lv_obj_t * btn_back = lv_btn_create(screen);
    lv_obj_set_size(btn_back, lv_pct(25), lv_pct(12));
    lv_obj_align(btn_back, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    lv_obj_t * btn_back_label = lv_label_create(btn_back);
    lv_label_set_text(btn_back_label, CUSTOM_SYMBOL_BACK "");
    lv_obj_center(btn_back_label);
    lv_obj_add_event_cb(btn_back, back_click, LV_EVENT_CLICKED, page);

    page->timer = lv_timer_create(timer_tick, 500, page);

    return screen;
}

static void control_click(lv_event_t * e)
{
    RecorderPage * page = (RecorderPage *)e->user_data;
    if(!page || !page->recorder) return;

    if(!page->recording) {
        mkdir(page->dir, 0755);

        time_t now = time(NULL);
        struct tm * tm = localtime(&now);
        char name[32];
        strftime(name, sizeof(name), "%Y%m%d-%H%M%S.aac", tm);
        snprintf(page->filename, sizeof(page->filename), "%s/%s", page->dir, name);


        int ret = recorder_start(page->recorder, page->filename);
        if (ret == 0) {
            page->start_time = now;
            page->recording  = true;
            lv_label_set_text(page->label_status, "Started");
            lv_label_set_text(page->btn_control_label, LV_SYMBOL_STOP "");
            lv_label_set_text(page->label_filename, page->filename);
        } else {
            lv_label_set_text_fmt(page->label_filename, "Start Error: %d", ret);
        }
        
    } else {
        recorder_stop(page->recorder);
        page->recording = false;

        lv_label_set_text(page->btn_control_label, LV_SYMBOL_PLAY "");
        lv_label_set_text(page->label_status, "Saved");
    }
}

static void timer_tick(lv_timer_t * e)
{
    RecorderPage * page = (RecorderPage *)e->user_data;
    if(!page) return;

    if(page->recording) {
        int elapsed = (int)(time(NULL) - page->start_time);
        lv_label_set_text_fmt(page->label_status, "Recording  %02d:%02d:%02d",
                              elapsed / 3600, (elapsed / 60) % 60, elapsed % 60);
    }
}

static void back_click(lv_event_t * e)
{
    page_back();
}

static bool page_recorder_on_key(void * p, key_code_t key_code, key_action_t key_action)
{
    if(!p) return false;
    if(key_code != KEY_CODE_HOME) return false;

    RecorderPage * page = (RecorderPage *)p;
    if(!page || !page->recorder) return false;

    // 使用一点小巧思，实现了点按、长按都支持
    uint64_t now = ms_get();
    if(key_action == KEY_ACTION_DOWN) {
        // 按下 HOME 键时，如果不是正在录音，那么开始录音
        if(!page->recording) {
            page->home_down_time = now;
            lv_event_send(page->btn_control, LV_EVENT_CLICKED, NULL);
        }
    }
    if(key_action == KEY_ACTION_UP) {
        // 抬起 HOME 键时，如果正在录音且录音长于 300ms，那么停止录音
        if(page->recording) {
            if(now - page->home_down_time > 300) {
                lv_event_send(page->btn_control, LV_EVENT_CLICKED, NULL);
            }
        }
    }

    return true;
}

static void page_recorder_destroy(void * p)
{
    RecorderPage * page = (RecorderPage *)p;
    if(page->timer) lv_timer_del(page->timer);
    if(page->recorder) {
        recorder_destroy(page->recorder);
        page->recorder = NULL;
    }
    sys_set_dont_deep_sleep(false);
}
