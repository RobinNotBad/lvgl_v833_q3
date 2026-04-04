#include "page_home.h"

#include "platform/battery_manager.h"
#include "page_menu.h"
#include "main.h"
#include "platform/str_utils.h"

#define TEXT_CLOCK_COLOR 0xff8fc07c

typedef struct
{
    BasePage base;
    lv_timer_t * timer_time;
    lv_timer_t * timer_battery;
    lv_obj_t * label_time;
    lv_obj_t * label_date;
    lv_obj_t * label_battery;
    lv_font_t * font_time;
    lv_font_t * font_date;
} HomePage;

static void btn_scan_click(lv_event_t * e);
static void btn_menu_click(lv_event_t * e);
static void timer_time_tick(lv_timer_t * e);
static void timer_battery_tick(lv_timer_t * e);
static lv_obj_t * page_home_obj(HomePage * page);
static void page_home_destroy(void * p);

BasePage * page_home_create(void)
{
    HomePage * page = malloc(sizeof(HomePage));
    if(!page) return NULL;
    memset(page, 0, sizeof(HomePage));

    page->base.obj        = page_home_obj(page);
    page->base.on_destroy = page_home_destroy;
    return (BasePage *)page;
}

lv_obj_t * page_home_obj(HomePage * page)
{
    lv_obj_t * screen = lv_obj_create(lv_scr_act());
    // lv_obj_remove_style_all(screen);
    lv_obj_set_size(screen, LV_PCT(100), LV_PCT(100));

    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

    lv_font_t * font_time = font_get(46, FT_FONT_STYLE_NORMAL);
    page->font_time       = font_time;
    lv_font_t * font_date = font_get(28, FT_FONT_STYLE_NORMAL);
    page->font_date       = font_date;

    lv_obj_t * label_time = lv_label_create(screen);
    lv_label_set_text(label_time, "23:33:33");
    lv_obj_set_size(label_time, LV_PCT(100), LV_PCT(22));
    lv_obj_align(label_time, LV_ALIGN_TOP_MID, 0, LV_PCT(10));
    lv_obj_set_style_text_font(label_time, font_time, LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(label_time, LV_TEXT_ALIGN_CENTER, LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(label_time, lv_color_hex(TEXT_CLOCK_COLOR), LV_STATE_DEFAULT);
    page->label_time = label_time;

    lv_obj_t * label_date = lv_label_create(screen);
    lv_label_set_text(label_date, "10-27 Sun");
    lv_obj_set_size(label_date, LV_PCT(100), LV_PCT(20));
    lv_obj_align(label_date, LV_ALIGN_TOP_MID, 0, LV_PCT(36));
    lv_obj_set_style_text_font(label_date, font_date, LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(label_date, LV_TEXT_ALIGN_CENTER, LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(label_date, lv_color_hex(TEXT_CLOCK_COLOR), LV_STATE_DEFAULT);
    page->label_date = label_date;

    lv_obj_t * label_battery = lv_label_create(screen);
    lv_obj_set_size(label_battery, LV_PCT(100), LV_PCT(12));
    lv_label_set_text(label_battery, "Ciallo Dendro!");
    lv_obj_align(label_battery, LV_ALIGN_TOP_RIGHT, 0, 0);
    lv_obj_set_style_text_color(label_battery, lv_color_hex(TEXT_CLOCK_COLOR), LV_STATE_DEFAULT);
    page->label_battery = label_battery;

    lv_obj_t * img_divider = lv_img_create(screen);
    lv_obj_set_size(img_divider, 240, 40);
    lv_img_set_src(img_divider, "./res/ui/home_divider.png");
    lv_obj_align(img_divider, LV_ALIGN_TOP_MID, 0, LV_PCT(50));

    page->timer_time = lv_timer_create(timer_time_tick, 250, page);
    page->timer_battery = lv_timer_create(timer_battery_tick, 2500, page);

    lv_obj_t * btn_robot = lv_btn_create(screen);
    lv_obj_set_size(btn_robot, LV_PCT(48), LV_PCT(25));
    lv_obj_t * btn_robot_label = lv_label_create(btn_robot);
    lv_label_set_text(btn_robot_label, "Scan");
    lv_obj_center(btn_robot_label);
    lv_obj_add_event_cb(btn_robot, btn_scan_click, LV_EVENT_CLICKED, NULL);
    lv_obj_align(btn_robot, LV_ALIGN_TOP_LEFT, 0, LV_PCT(72));

    lv_obj_t * btn_menu = lv_btn_create(screen);
    lv_obj_set_size(btn_menu, LV_PCT(48), LV_PCT(25));
    lv_obj_t * btn_menu_label = lv_label_create(btn_menu);
    lv_label_set_text(btn_menu_label, "Menu");
    lv_obj_center(btn_menu_label);
    lv_obj_add_event_cb(btn_menu, btn_menu_click, LV_EVENT_CLICKED, NULL);
    lv_obj_align(btn_menu, LV_ALIGN_TOP_RIGHT, 0, LV_PCT(72));

    return screen;
}

static void btn_scan_click(lv_event_t * e)
{
    switch_robot();
}

static void btn_menu_click(lv_event_t * e)
{
    page_open(page_menu_create());
}

static void timer_time_tick(lv_timer_t * e)
{
    HomePage * page = (HomePage *)e->user_data;
    if(!page) return;

    struct timeval tv;
    gettimeofday(&tv, NULL);

    struct tm * tm;
    tm = localtime((time_t *)&tv);

    char time_text[24];
    lv_snprintf(time_text, sizeof(time_text), "%02d:%02d:%02d", tm->tm_hour, tm->tm_min, tm->tm_sec);
    lv_label_set_text(page->label_time, time_text);

    char date_text[24];
    lv_snprintf(date_text, sizeof(date_text), "%02d-%02d  %s", tm->tm_mon + 1, tm->tm_mday, days_of_week[tm->tm_wday]);
    lv_label_set_text(page->label_date, date_text);
}

static void timer_battery_tick(lv_timer_t * e)
{
    HomePage * page = (HomePage *)e->user_data;
    if(!page) return;

    char battery_text[24];
    uint8_t capacity        = battery_get_capacity();
    battery_status_t status = battery_get_status();

    char status_str[24];
    switch(status) {
        case BATTERY_DISCHARGING: strcpy(status_str, ""); break;
        case BATTERY_CHARGING: strcpy(status_str, "Charging"); break;
        case BATTERY_FULL: strcpy(status_str, "Full"); break;
        default: strcpy(status_str, "Unknown"); break;
    }

    snprintf(battery_text, sizeof(battery_text), LV_SYMBOL_CHARGE "%d%% %s", capacity, status_str);
    lv_label_set_text(page->label_battery, battery_text);
}

static void page_home_destroy(void * p)
{
    HomePage * page = (HomePage *)p;
    if(page->timer_battery) lv_timer_del(page->timer_battery);
    if(page->timer_time) lv_timer_del(page->timer_time);
    if(page->font_time) lv_ft_font_destroy(page->font_time);
    if(page->font_date) lv_ft_font_destroy(page->font_date);
}
