#include "custom_msgbox.h"

static void custom_msgbox_delete_cb(lv_event_t * e) {
    lv_obj_del_async(lv_obj_get_parent(lv_event_get_target(e)));
}

lv_obj_t * custom_msgbox_create(const char *title, const char *txt, const char **btn_txts, bool *add_close_btn)
{
    lv_obj_t * mbox_bg = lv_obj_create(lv_scr_act());
    lv_obj_remove_style_all(mbox_bg);
    lv_obj_clear_flag(mbox_bg, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(mbox_bg, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_opa(mbox_bg, LV_OPA_60, LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(mbox_bg, lv_color_black(), LV_STATE_DEFAULT);
    lv_obj_t * mbox = lv_msgbox_create(mbox_bg, title, txt,
                btn_txts, add_close_btn);
    lv_obj_center(mbox);
    lv_obj_add_event_cb(mbox, custom_msgbox_delete_cb, LV_EVENT_DELETE, NULL);
    return mbox;
}

static void custom_toast_timer_cb(lv_timer_t * t) {
    lv_obj_t * mbox = (lv_obj_t *)t->user_data;
    lv_obj_del_async(mbox);
    lv_timer_del(t);
}

lv_obj_t * custom_toast_create(const char *txt)
{
    lv_obj_t * mbox = lv_label_create(lv_scr_act());
    lv_label_set_long_mode(mbox, LV_LABEL_LONG_WRAP);
    lv_label_set_text(mbox, txt);
    lv_obj_set_style_bg_opa(mbox, LV_OPA_60, LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(mbox, lv_palette_main(LV_PALETTE_GREY), LV_STATE_DEFAULT);
    lv_obj_set_size(mbox, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_align(mbox, LV_ALIGN_TOP_MID, 0, lv_pct(15));
    lv_obj_set_style_text_align(mbox, LV_TEXT_ALIGN_CENTER, LV_STATE_DEFAULT);
    
    lv_timer_create(custom_toast_timer_cb, 3000, mbox);
    return mbox;
}
