#include "page_main.h"

#include "platform/battery_manager.h"
#include "page_file_manager.h"
#include "page_bird.h"
#include "page_calculator.h"
#include "page_demo.h"
#include "page_ftp.h"
#include "main.h"

static lv_timer_t * timer_time;
static lv_timer_t * timer_battery;
static lv_obj_t * label_time;
static lv_obj_t * label_battery;

static void btn_demo_click(lv_event_t * e);
static void btn_robot_click(lv_event_t * e);
static void btn_file_manager_click(lv_event_t * e);
static void btn_calculator_click(lv_event_t * e);
static void btn_bird_click(lv_event_t * e);
static void btn_ftp_click(lv_event_t * e);
static void timer_time_tick(lv_timer_t * e);
static void timer_battery_tick(lv_timer_t * e);

lv_obj_t * page_main(void)
{
    lv_obj_t * screen = lv_obj_create(lv_scr_act());
    // lv_obj_remove_style_all(screen);
    lv_obj_set_size(screen, lv_pct(100), lv_pct(100));

    lv_obj_set_flex_flow(screen, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_scroll_dir(screen, LV_DIR_VER);

    label_time = lv_label_create(screen);
    lv_label_set_text(label_time, "Ciallo LVGL");
    lv_obj_set_size(label_time, lv_pct(100), lv_pct(10));
    // lv_obj_align(label_time, LV_ALIGN_CENTER, 0, 0);

    label_battery = lv_label_create(screen);
    lv_obj_set_size(label_battery, lv_pct(100), lv_pct(10));
    lv_label_set_text(label_battery, "Ciallo Dendro");
    // lv_obj_align(label_battery, LV_ALIGN_CENTER, 0, 0);

    timer_time    = lv_timer_create(timer_time_tick, 250, NULL);
    timer_battery = lv_timer_create(timer_battery_tick, 1000, NULL);
    timer_battery_tick(NULL);

    lv_obj_t * btn_robot = lv_btn_create(screen);
    lv_obj_set_size(btn_robot, lv_pct(60), lv_pct(25));
    lv_obj_align(btn_robot, LV_FLEX_ALIGN_CENTER, 0, 0);
    lv_obj_t * btn_label_robot = lv_label_create(btn_robot);
    lv_label_set_text(btn_label_robot, "robot");
    lv_obj_center(btn_label_robot);
    lv_obj_add_event_cb(btn_robot, btn_robot_click, LV_EVENT_CLICKED, NULL);

    lv_obj_t * btn_file_manager = lv_btn_create(screen);
    lv_obj_set_size(btn_file_manager, lv_pct(60), lv_pct(25));
    lv_obj_align(btn_file_manager, LV_FLEX_ALIGN_CENTER, 0, 0);
    lv_obj_t * btn_label_file_manager = lv_label_create(btn_file_manager);
    lv_label_set_text(btn_label_file_manager, "file manager");
    lv_obj_center(btn_label_file_manager);
    lv_obj_add_event_cb(btn_file_manager, btn_file_manager_click, LV_EVENT_CLICKED, NULL);

    lv_obj_t * btn_calculator = lv_btn_create(screen);
    lv_obj_set_size(btn_calculator, lv_pct(60), lv_pct(25));
    lv_obj_align(btn_calculator, LV_FLEX_ALIGN_CENTER, 0, 0);
    lv_obj_t * btn_label_calculator = lv_label_create(btn_calculator);
    lv_label_set_text(btn_label_calculator, "calculator");
    lv_obj_center(btn_label_calculator);
    lv_obj_add_event_cb(btn_calculator, btn_calculator_click, LV_EVENT_CLICKED, NULL);

    lv_obj_t * btn_bird = lv_btn_create(screen);
    lv_obj_set_size(btn_bird, lv_pct(60), lv_pct(25));
    lv_obj_align(btn_bird, LV_FLEX_ALIGN_CENTER, 0, 0);
    lv_obj_t * btn_label_bird = lv_label_create(btn_bird);
    lv_label_set_text(btn_label_bird, "flappy bird");
    lv_obj_center(btn_label_bird);
    lv_obj_add_event_cb(btn_bird, btn_bird_click, LV_EVENT_CLICKED, NULL);

    lv_obj_t * btn_ftp = lv_btn_create(screen);
    lv_obj_set_size(btn_ftp, lv_pct(60), lv_pct(25));
    lv_obj_align(btn_ftp, LV_FLEX_ALIGN_CENTER, 0, 0);
    lv_obj_t * btn_label_ftp = lv_label_create(btn_ftp);
    lv_label_set_text(btn_label_ftp, "ftp");
    lv_obj_center(btn_label_ftp);
    lv_obj_add_event_cb(btn_ftp, btn_ftp_click, LV_EVENT_CLICKED, NULL);

    lv_obj_t * btn_demo = lv_btn_create(screen);
    lv_obj_set_size(btn_demo, lv_pct(60), lv_pct(25));
    lv_obj_align(btn_demo, LV_FLEX_ALIGN_CENTER, 0, 0);
    lv_obj_t * btn_label_demo = lv_label_create(btn_demo);
    lv_label_set_text(btn_label_demo, "demo page");
    lv_obj_center(btn_label_demo);
    lv_obj_add_event_cb(btn_demo, btn_demo_click, LV_EVENT_CLICKED, NULL);

    return screen;
}

static void btn_demo_click(lv_event_t * e) // static可以防止同名冲突
{
    page_open(demo_page_create());
}

static void btn_robot_click(lv_event_t * e)
{
    switchRobot();
}

static void btn_file_manager_click(lv_event_t * e)
{
    page_open_obj(page_file_manager());
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

static void timer_time_tick(lv_timer_t * e)
{
    char time_text[24];
    struct timeval tv;
    gettimeofday(&tv, NULL);

    struct tm * tm;
    tm = localtime(&tv);

    lv_snprintf(time_text, sizeof(time_text), "%04d-%02d-%02d %02d:%02d:%02d", tm->tm_year + 1900, tm->tm_mon + 1,
                tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
    lv_label_set_text(label_time, time_text);
}

static void timer_battery_tick(lv_timer_t * e)
{
    char battery_text[24];
    uint8_t capacity        = battery_get_capacity();
    double voltage          = battery_get_voltage();
    battery_status_t status = battery_get_status();

    char status_str[24];
    switch(status) {
        case BATTERY_DISCHARGING: strcpy(status_str, "Discharging"); break;
        case BATTERY_CHARGING: strcpy(status_str, "Charging"); break;
        case BATTERY_FULL: strcpy(status_str, "Full"); break;
        default: strcpy(status_str, "Unknown"); break;
    }

    snprintf(battery_text, sizeof(battery_text), "%d%% %s %.3fV", capacity, status_str, voltage);
    lv_label_set_text(label_battery, battery_text);
}
