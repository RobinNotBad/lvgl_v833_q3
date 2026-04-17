#include "page_menu.h"

#include "platform/battery_manager.h"
#include "page_file_manager.h"
#include "page_bird.h"
#include "page_calculator.h"
#include "page_demo.h"
#include "page_ftp.h"
#include "page_usb.h"
#include "main.h"
#include "platform/str_utils.h"

typedef struct
{
    BasePage base;
    lv_obj_t * label_time;
    lv_obj_t * label_battery;
} MenuPage;

static void btn_demo_click(lv_event_t * e);
static void btn_back_click(lv_event_t * e);
static void btn_file_manager_click(lv_event_t * e);
static void btn_calculator_click(lv_event_t * e);
static void btn_bird_click(lv_event_t * e);
static void btn_ftp_click(lv_event_t * e);
static void btn_usb_click(lv_event_t * e);
static lv_obj_t * page_menu_obj(MenuPage * page);

BasePage * page_menu_create(void)
{
    MenuPage * page = malloc(sizeof(MenuPage));
    if(!page) return NULL;
    memset(page, 0, sizeof(MenuPage));

    page->base.obj = page_menu_obj(page);
    return (BasePage *)page;
}

lv_obj_t * page_menu_obj(MenuPage * page)
{
    lv_obj_t * screen = lv_obj_create(lv_scr_act());
    lv_obj_set_size(screen, lv_pct(100), lv_pct(100));
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(screen, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(screen, 0, LV_STATE_DEFAULT);

    lv_obj_t * container = lv_obj_create(screen);
    lv_obj_align(container, LV_ALIGN_TOP_MID, 0, lv_pct(12));
    lv_obj_set_size(container, lv_pct(100), lv_pct(88));
    lv_obj_set_flex_flow(container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_scroll_dir(container, LV_DIR_VER);

    lv_obj_t * btn_back = lv_btn_create(screen);
    lv_obj_set_size(btn_back, lv_pct(25), lv_pct(12));
    lv_obj_align(btn_back, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_t * btn_back_label = lv_label_create(btn_back);
    lv_label_set_text(btn_back_label, CUSTOM_SYMBOL_BACK "");
    lv_obj_center(btn_back_label);
    lv_obj_add_event_cb(btn_back, btn_back_click, LV_EVENT_CLICKED, NULL);

    lv_obj_t * btn_file_manager = lv_btn_create(container);
    lv_obj_set_size(btn_file_manager, lv_pct(64), lv_pct(32));
    lv_obj_align(btn_file_manager, LV_FLEX_ALIGN_CENTER, 0, 0);
    lv_obj_t * btn_label_file_manager = lv_label_create(btn_file_manager);
    lv_label_set_text(btn_label_file_manager, "File Manager");
    lv_obj_center(btn_label_file_manager);
    lv_obj_add_event_cb(btn_file_manager, btn_file_manager_click, LV_EVENT_CLICKED, NULL);

    lv_obj_t * btn_calculator = lv_btn_create(container);
    lv_obj_set_size(btn_calculator, lv_pct(64), lv_pct(32));
    lv_obj_align(btn_calculator, LV_FLEX_ALIGN_CENTER, 0, 0);
    lv_obj_t * btn_label_calculator = lv_label_create(btn_calculator);
    lv_label_set_text(btn_label_calculator, "Calculator");
    lv_obj_center(btn_label_calculator);
    lv_obj_add_event_cb(btn_calculator, btn_calculator_click, LV_EVENT_CLICKED, NULL);

    lv_obj_t * btn_bird = lv_btn_create(container);
    lv_obj_set_size(btn_bird, lv_pct(64), lv_pct(32));
    lv_obj_align(btn_bird, LV_FLEX_ALIGN_CENTER, 0, 0);
    lv_obj_t * btn_label_bird = lv_label_create(btn_bird);
    lv_label_set_text(btn_label_bird, "Flappy Bird");
    lv_obj_center(btn_label_bird);
    lv_obj_add_event_cb(btn_bird, btn_bird_click, LV_EVENT_CLICKED, NULL);

    lv_obj_t * btn_ftp = lv_btn_create(container);
    lv_obj_set_size(btn_ftp, lv_pct(64), lv_pct(32));
    lv_obj_align(btn_ftp, LV_FLEX_ALIGN_CENTER, 0, 0);
    lv_obj_t * btn_label_ftp = lv_label_create(btn_ftp);
    lv_label_set_text(btn_label_ftp, "FTP");
    lv_obj_center(btn_label_ftp);
    lv_obj_add_event_cb(btn_ftp, btn_ftp_click, LV_EVENT_CLICKED, NULL);

    if(access("/dev/by-name/sdcard", F_OK) == 0) {
        lv_obj_t * btn_usb = lv_btn_create(container);
        lv_obj_set_size(btn_usb, lv_pct(64), lv_pct(32));
        lv_obj_align(btn_usb, LV_FLEX_ALIGN_CENTER, 0, 0);
        lv_obj_t * btn_label_usb = lv_label_create(btn_usb);
        lv_label_set_text(btn_label_usb, "USB");
        lv_obj_center(btn_label_usb);
        lv_obj_add_event_cb(btn_usb, btn_usb_click, LV_EVENT_CLICKED, NULL);
    }

    lv_obj_t * btn_demo = lv_btn_create(container);
    lv_obj_set_size(btn_demo, lv_pct(64), lv_pct(32));
    lv_obj_align(btn_demo, LV_FLEX_ALIGN_CENTER, 0, 0);
    lv_obj_t * btn_label_demo = lv_label_create(btn_demo);
    lv_label_set_text(btn_label_demo, "Test Page");
    lv_obj_center(btn_label_demo);
    lv_obj_add_event_cb(btn_demo, btn_demo_click, LV_EVENT_CLICKED, NULL);

    return screen;
}

static void btn_demo_click(lv_event_t * e) // static可以防止同名冲突
{
    page_open(demo_page_create());
}

static void btn_back_click(lv_event_t * e)
{
    page_back();
}

static void btn_file_manager_click(lv_event_t * e)
{
    page_open(page_file_manager_create());
}

static void btn_calculator_click(lv_event_t * e)
{
    page_open_obj(page_calc());
}

static void btn_bird_click(lv_event_t * e)
{
    page_open(page_bird_create());
}

static void btn_ftp_click(lv_event_t * e)
{
    page_open_obj(page_ftp());
}

static void btn_usb_click(lv_event_t * e)
{
    page_open_obj(page_usb());
}
