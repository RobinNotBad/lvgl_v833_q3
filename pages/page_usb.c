#include "page_usb.h"

#include "main.h"

#define SET_USB_MODE_SH "/mnt/app/robot/shell/set_usb_mode"

static void back_click(lv_event_t * e);
static void btn_adb_only_click(lv_event_t * e);
static void btn_adb_ums_click(lv_event_t * e);
static void refresh_text(lv_obj_t * label);
static bool is_vsftpd_running(void);

lv_obj_t * page_usb(void)
{
    lv_obj_t * screen = lv_obj_create(lv_scr_act());
    lv_obj_remove_style_all(screen);
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(screen, lv_pct(100), lv_pct(100));

    lv_obj_t * btn_adb_only = lv_btn_create(screen);
    lv_obj_set_size(btn_adb_only, lv_pct(60), lv_pct(25));
    lv_obj_align(btn_adb_only, LV_ALIGN_TOP_MID, 0, lv_pct(25));
    lv_obj_t * btn_adb_only_label = lv_label_create(btn_adb_only);
    lv_label_set_text(btn_adb_only_label, "adb only");
    lv_obj_center(btn_adb_only_label);
    lv_obj_add_event_cb(btn_adb_only, btn_adb_only_click, LV_EVENT_CLICKED, NULL);

    lv_obj_t * btn_adb_ums = lv_btn_create(screen);
    lv_obj_set_size(btn_adb_ums, lv_pct(60), lv_pct(25));
    lv_obj_align(btn_adb_ums, LV_ALIGN_TOP_MID, 0, lv_pct(55));
    lv_obj_t * btn_adb_ums_label = lv_label_create(btn_adb_ums);
    lv_label_set_text(btn_adb_ums_label, "adb & ums");
    lv_obj_center(btn_adb_ums_label);
    lv_obj_add_event_cb(btn_adb_ums, btn_adb_ums_click, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t * btn_back = lv_btn_create(screen);
    lv_obj_set_size(btn_back, lv_pct(25), lv_pct(12));
    lv_obj_align(btn_back, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_t * btn_back_label = lv_label_create(btn_back);
    lv_label_set_text(btn_back_label, CUSTOM_SYMBOL_BACK "");
    lv_obj_center(btn_back_label);
    lv_obj_add_event_cb(btn_back, back_click, LV_EVENT_CLICKED, NULL);

    return screen;
}

static void back_click(lv_event_t * e)
{
    page_back();
}

static void btn_adb_only_click(lv_event_t * e)
{
    system("chmod 777 " SET_USB_MODE_SH);
    system("sh " SET_USB_MODE_SH " mass_storage_off");
    system("sh " SET_USB_MODE_SH " adb_on");
}

static void btn_adb_ums_click(lv_event_t * e)
{
    system("chmod 777 " SET_USB_MODE_SH);
    system("sh " SET_USB_MODE_SH " adb,mass_storage");
}
