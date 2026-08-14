#include "ime_helper.h"

/**
 * 全屏输入法模式（类似手表）
 */

#if IME_USE_FULLSCREEN == 1

#include "main.h"
#include "string.h"
#include "platform/page_manager.h"
#include "lv_ime_pinyin.h"

typedef struct
{
    BasePage base;
    lv_obj_t * textarea;
    lv_obj_t * ime_input;
} ImeFullscrPage;

static void textarea_clicked_cb(lv_event_t * e);
static void ime_input_done_cb(lv_event_t * e);
static void page_ime_destroy(void * p);

/**
 * @brief 初始化输入法（占位空函数）
 */
void ime_helper_init(void) {

}

/**
 * 显示输入法页面
 */
static void page_ime_show(lv_obj_t * textarea) {
    ImeFullscrPage * page = malloc(sizeof(ImeFullscrPage));
    if(!page) return;
    memset(page, 0, sizeof(ImeFullscrPage));

    lv_obj_t * ime_container = lv_obj_create(lv_scr_act());
    lv_obj_set_style_pad_all(ime_container, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ime_container, 0, LV_STATE_DEFAULT);
    lv_obj_clear_flag(ime_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(ime_container, lv_pct(100), lv_pct(100));

    lv_obj_t * ime_input = lv_textarea_create(ime_container);
    lv_obj_t * pinyin_ime = lv_ime_pinyin_create(ime_container);
    lv_obj_clear_flag(pinyin_ime, LV_OBJ_FLAG_HIDDEN);

#if IME_LAYOUT_DIRECTION == 0
    // HORIZONTAL
    lv_obj_set_size(ime_input, lv_pct(25), lv_pct(100));
    lv_obj_align(ime_input, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_set_size(pinyin_ime, lv_pct(75), lv_pct(100));
    lv_obj_align(pinyin_ime, LV_ALIGN_RIGHT_MID, 0, 0);
#else
    // VERTICAL
    lv_obj_set_size(ime_input, lv_pct(100), lv_pct(25));
    lv_obj_align(ime_input, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_size(pinyin_ime, lv_pct(100), lv_pct(75));
    lv_obj_align(pinyin_ime, LV_ALIGN_BOTTOM_MID, 0, 0);
#endif  // IME_FULLSCREEN_LAYOUT == 0

    ime_input->flags = textarea->flags;
    lv_textarea_set_text(ime_input, lv_textarea_get_text(textarea));
    lv_obj_t * keyboard = lv_ime_pinyin_get_kb(pinyin_ime);
    lv_obj_add_state(ime_input, LV_STATE_FOCUSED);
    
    lv_keyboard_set_textarea(keyboard, ime_input);

    lv_obj_add_event_cb(ime_input, ime_input_done_cb, LV_EVENT_ALL, NULL);
    
    page->base.obj   = ime_container;
    page->ime_input = ime_input;
    page->textarea   = textarea;
    page->base.on_destroy = page_ime_destroy;
    page_open((BasePage *)page);
}

/**
 * @brief 给textarea添加输入法相关回调
 * @param textarea 要绑定的textarea
 */
void lv_textarea_bind_ime(lv_obj_t * textarea) {
    if(textarea == NULL) return;
    lv_obj_add_event_cb(textarea, textarea_clicked_cb, LV_EVENT_CLICKED, NULL);
}

static void textarea_clicked_cb(lv_event_t * e) {
    lv_obj_t * textarea = lv_event_get_target(e);

    page_ime_show(textarea);
    printf("[ime_helper] show keyboard\n");
}

static void ime_input_done_cb(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_READY /*|| code == LV_EVENT_CANCEL*/) {
        page_back();
    }
}

static void page_ime_destroy(void * p) 
{
    if(!p) return;
    ImeFullscrPage * page = (ImeFullscrPage *)p;

    lv_obj_t * ime_input = page->ime_input;
    lv_obj_t * current_textarea = page->textarea;

    lv_textarea_set_text(current_textarea, lv_textarea_get_text(ime_input));
    lv_event_send(current_textarea, LV_EVENT_READY, NULL);
    lv_obj_clear_state(current_textarea, LV_STATE_FOCUSED);
    lv_indev_reset(NULL, current_textarea); /*To forget the last clicked object to make it focusable again*/

    printf("[ime_helper] hide keyboard\n");
}

#endif
