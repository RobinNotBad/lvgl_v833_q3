#include "page_selector.h"

#include <string.h>

#include "page_apple.h"
#include "page_audio.h"
#include "page_midi.h"
#include "page_image.h"
#include "page_txt.h"

#define SELECTOR_PAGE_ID "page_selector"

typedef struct
{
    BasePage base;
    char filename[PATH_MAX_LENGTH];
} SelectorPage;

static lv_obj_t * page_selector_obj(SelectorPage * page, char * filename);
static void back_click(lv_event_t * e);
static void btn_txt_click(lv_event_t * e);
static void btn_image_click(lv_event_t * e);
static void btn_audio_click(lv_event_t * e);
//static void btn_midi_click(lv_event_t * e);
static void btn_video_click(lv_event_t * e);

BasePage * page_selector_create(char * filename)
{
    SelectorPage * page = malloc(sizeof(SelectorPage));
    if(!page) return NULL;
    memset(page, 0, sizeof(SelectorPage));

    page->base.obj        = page_selector_obj(page, filename);
    strcpy(page->base.page_id, SELECTOR_PAGE_ID);
    return (BasePage *)page;
}

static lv_obj_t * page_selector_obj(SelectorPage * page, char * filename)
{
    if (filename) strcpy(page->filename, filename);

    lv_obj_t * screen = lv_obj_create(lv_scr_act());
    lv_obj_set_size(screen, lv_pct(100), lv_pct(100));
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(screen, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(screen, 0, LV_STATE_DEFAULT);

    lv_obj_t * btn_back = lv_btn_create(screen);
    lv_obj_set_size(btn_back, lv_pct(25), lv_pct(12));
    lv_obj_align(btn_back, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_t * btn_back_label = lv_label_create(btn_back);
    lv_label_set_text(btn_back_label, CUSTOM_SYMBOL_BACK "");
    lv_obj_center(btn_back_label);
    lv_obj_add_event_cb(btn_back, back_click, LV_EVENT_CLICKED, page);

    lv_obj_t * container = lv_obj_create(screen);
    lv_obj_align(container, LV_ALIGN_TOP_MID, 0, lv_pct(12));
    lv_obj_set_size(container, lv_pct(100), lv_pct(88));
    lv_obj_set_flex_flow(container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_scroll_dir(container, LV_DIR_VER);
    
    lv_obj_t * label_file_name = lv_label_create(container);
    lv_label_set_long_mode(label_file_name, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_width(label_file_name, lv_pct(100));
    lv_label_set_text(label_file_name, page->filename);
    
    lv_obj_t * btn_txt = lv_btn_create(container);
    lv_obj_set_size(btn_txt, lv_pct(100), lv_pct(32));
    lv_obj_align(btn_txt, LV_FLEX_ALIGN_CENTER, 0, 0);
    lv_obj_t * btn_label_txt = lv_label_create(btn_txt);
    lv_label_set_text(btn_label_txt, "text viewer");
    lv_obj_center(btn_label_txt);
    lv_obj_add_event_cb(btn_txt, btn_txt_click, LV_EVENT_CLICKED, page);

    lv_obj_t * btn_image = lv_btn_create(container);
    lv_obj_set_size(btn_image, lv_pct(100), lv_pct(32));
    lv_obj_align(btn_image, LV_FLEX_ALIGN_CENTER, 0, 0);
    lv_obj_t * btn_label_image = lv_label_create(btn_image);
    lv_label_set_text(btn_label_image, "image viewer");
    lv_obj_center(btn_label_image);
    lv_obj_add_event_cb(btn_image, btn_image_click, LV_EVENT_CLICKED, page);

    lv_obj_t * btn_audio = lv_btn_create(container);
    lv_obj_set_size(btn_audio, lv_pct(100), lv_pct(32));
    lv_obj_align(btn_audio, LV_FLEX_ALIGN_CENTER, 0, 0);
    lv_obj_t * btn_label_audio = lv_label_create(btn_audio);
    lv_label_set_text(btn_label_audio, "audio player");
    lv_obj_center(btn_label_audio);
    lv_obj_add_event_cb(btn_audio, btn_audio_click, LV_EVENT_CLICKED, page);

    lv_obj_t * btn_video = lv_btn_create(container);
    lv_obj_set_size(btn_video, lv_pct(100), lv_pct(32));
    lv_obj_align(btn_video, LV_FLEX_ALIGN_CENTER, 0, 0);
    lv_obj_t * btn_label_video = lv_label_create(btn_video);
    lv_label_set_text(btn_label_video, "video player");
    lv_obj_center(btn_label_video);
    lv_obj_add_event_cb(btn_video, btn_video_click, LV_EVENT_CLICKED, page);

    return screen;
}

static void btn_txt_click(lv_event_t * e)
{
    SelectorPage * page = (SelectorPage *)e->user_data;
    page_open(page_txt_create(page->filename));
    page_close_existing(SELECTOR_PAGE_ID);
}

static void btn_image_click(lv_event_t * e)
{
    SelectorPage * page = (SelectorPage *)e->user_data;
    page_open(page_image_create(page->filename));
    page_close_existing(SELECTOR_PAGE_ID);
}

static void btn_audio_click(lv_event_t * e)
{
    SelectorPage * page = (SelectorPage *)e->user_data;
    page_open(page_audio_create(page->filename));
    page_close_existing(SELECTOR_PAGE_ID);
}

static void btn_video_click(lv_event_t * e)
{
    SelectorPage * page = (SelectorPage *)e->user_data;
    page_open(page_video_create(page->filename));
    page_close_existing(SELECTOR_PAGE_ID);
}

static void back_click(lv_event_t * e)
{
    page_back();
}
