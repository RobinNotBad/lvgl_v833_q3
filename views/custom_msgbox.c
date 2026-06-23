#include "custom_msgbox.h"

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
    return mbox;
}

void custom_msgbox_close(lv_obj_t *msgbox)
{
    lv_obj_del_async(lv_obj_get_parent(msgbox));
    lv_msgbox_close_async(msgbox);
}