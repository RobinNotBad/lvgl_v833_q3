#include "ime_helper.h"

/**
 * 悬浮输入法模式（类似手机）
 */

#if IME_USE_FULLSCREEN == 0

#include "main.h"
#include "lv_ime_pinyin.h"

pinyin_ime_t * pinyin_ime;
static lv_obj_t * lv_ime = NULL;
static void textarea_ime_event_cb(lv_event_t * e);

/**
 * @brief 初始化输入法
 */
void ime_helper_init(void) {
    if (!lv_ime) {
        lv_ime = lv_ime_pinyin_create(lv_scr_act());
        lv_obj_set_size(lv_ime, lv_pct(100), lv_pct(64));
        //lv_obj_t * keyboard = lv_ime_pinyin_get_kb(lv_ime);
        //lv_obj_t * cand_panel = lv_ime_pinyin_get_cand_panel(lv_ime);
        lv_obj_add_flag(lv_ime, LV_OBJ_FLAG_HIDDEN);
    }

    if (!pinyin_ime) {
        pinyin_ime = pinyin_ime_init(IME_PINYIN_MAP_FILE, IME_DICTIONARY_FILE);
        lv_ime_pinyin_init(lv_ime, pinyin_ime);
    }
    printf("[ime_helper] init\n");
}

/**
 * @brief 给textarea添加输入法相关回调
 * @param textarea 要绑定的textarea
 */
void lv_textarea_bind_ime(lv_obj_t * textarea) {
    if(!textarea || !lv_ime) return;
    lv_obj_add_event_cb(textarea, textarea_ime_event_cb, LV_EVENT_ALL, NULL);
}

static void textarea_ime_event_cb(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * textarea = lv_event_get_target(e);

    if(lv_ime == NULL || textarea == NULL) return;

    lv_obj_t * keyboard = lv_ime_pinyin_get_kb(lv_ime);
    //lv_obj_t * cand_panel = lv_ime_pinyin_get_cand_panel(lv_ime);

    if(code == LV_EVENT_CLICKED) {
        lv_keyboard_set_textarea(keyboard, textarea);
        lv_obj_clear_flag(lv_ime, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(lv_ime);
        printf("[ime_helper] show keyboard\n");
    }
    else if(code == LV_EVENT_READY || code == LV_EVENT_DELETE /*|| code == LV_EVENT_CANCEL*/) {
        lv_obj_add_flag(lv_ime, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_state(textarea, LV_STATE_FOCUSED);
        lv_indev_reset(NULL, textarea);   /*To forget the last clicked object to make it focusable again*/
        printf("[ime_helper] hide keyboard\n");
    }
}

#endif
