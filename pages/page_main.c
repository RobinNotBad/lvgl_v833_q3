#include "page_main.h"

static void btn_adb_click(lv_event_t * e);
static void btn_demo_click(lv_event_t * e);
static void btn_robot_click(lv_event_t * e);
static void btn_file_manager_click(lv_event_t * e);
static void btn_calculator_click(lv_event_t * e);
static void btn_apple_click(lv_event_t * e);

lv_obj_t * page_main() {
	lv_obj_t * screen = lv_obj_create(lv_scr_act());
    //lv_obj_remove_style_all(screen);
    lv_obj_set_size(screen, lv_pct(100), lv_pct(100));

    lv_obj_set_flex_flow(screen, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_scroll_dir(screen, LV_DIR_VER);

    lv_obj_t * label1 = lv_label_create(screen);
	lv_label_set_text(label1, "main page");
    lv_obj_align(label1, LV_FLEX_ALIGN_CENTER, 0, 0);

    /*
    lv_obj_t * btn_adb = lv_btn_create(screen);
    lv_obj_set_size(btn_adb, lv_pct(45), lv_pct(25));
    lv_obj_align(btn_adb, LV_FLEX_ALIGN_CENTER, 0, 0);
    lv_obj_t * btn_adb_label = lv_label_create(btn_adb);
    lv_label_set_text(btn_adb_label, "open adb");
    lv_obj_center(btn_adb_label);
    lv_obj_add_event_cb(btn_adb, btn_adb_click, LV_EVENT_CLICKED, NULL);
    */

    lv_obj_t * btn_robot = lv_btn_create(screen);
    lv_obj_set_size(btn_robot, lv_pct(45), lv_pct(25));
    lv_obj_align(btn_robot, LV_FLEX_ALIGN_CENTER, 0, 0);
    lv_obj_t * btn_label_robot = lv_label_create(btn_robot);
    lv_label_set_text(btn_label_robot, "robot");
    lv_obj_center(btn_label_robot);
    lv_obj_add_event_cb(btn_robot, btn_robot_click, LV_EVENT_CLICKED, NULL);

    lv_obj_t * btn_demo = lv_btn_create(screen);
	lv_obj_set_size(btn_demo, lv_pct(45), lv_pct(25));
    lv_obj_align(btn_demo, LV_FLEX_ALIGN_CENTER, 0, 0);
    lv_obj_t * btn_label_demo = lv_label_create(btn_demo);
	lv_label_set_text(btn_label_demo, "demo page");
	lv_obj_center(btn_label_demo);
	lv_obj_add_event_cb(btn_demo, btn_demo_click, LV_EVENT_CLICKED, NULL);

    lv_obj_t * btn_file_manager = lv_btn_create(screen);
    lv_obj_set_size(btn_file_manager, lv_pct(45), lv_pct(25));
    lv_obj_align(btn_file_manager, LV_FLEX_ALIGN_CENTER, 0, 0);
    lv_obj_t * btn_label_file_manager = lv_label_create(btn_file_manager);
    lv_label_set_text(btn_label_file_manager, "file manager");
    lv_obj_center(btn_label_file_manager);
    lv_obj_add_event_cb(btn_file_manager, btn_file_manager_click, LV_EVENT_CLICKED, NULL);

    lv_obj_t * btn_calculator = lv_btn_create(screen);
    lv_obj_set_size(btn_calculator, lv_pct(45), lv_pct(25));
    lv_obj_align(btn_calculator, LV_FLEX_ALIGN_CENTER, 0, 0);
    lv_obj_t * btn_label_calculator = lv_label_create(btn_calculator);
    lv_label_set_text(btn_label_calculator, "calculator");
    lv_obj_center(btn_label_calculator);
    lv_obj_add_event_cb(btn_calculator, btn_calculator_click, LV_EVENT_CLICKED, NULL);

    lv_obj_t * btn_apple = lv_btn_create(screen);
    lv_obj_set_size(btn_apple, lv_pct(45), lv_pct(25));
    lv_obj_align(btn_apple, LV_FLEX_ALIGN_CENTER, 0, 0);
    lv_obj_t * btn_label_apple = lv_label_create(btn_apple);
    lv_label_set_text(btn_label_apple, "apple");
    lv_obj_center(btn_label_apple);
    lv_obj_add_event_cb(btn_apple, btn_apple_click, LV_EVENT_CLICKED, NULL);
    return screen;
}

static void btn_adb_click(lv_event_t * e)
{
    system("chmod 777 set_usb_mode");
    system("sh ./set_usb_mode init");
    usleep(100000);
    system("sh ./set_usb_mode adb_on");
}

static void btn_demo_click(lv_event_t * e)      //static可以防止同名冲突
{	
    page_open(page_demo(), NULL);
}

static void btn_robot_click(lv_event_t * e)
{
    switchRobot();
}

static void btn_file_manager_click(lv_event_t * e)
{
    page_open(page_file_manager(), NULL);
}

static void btn_calculator_click(lv_event_t * e)
{
    page_open(page_calculator(), NULL);
}

static void btn_apple_click(lv_event_t * e)
{
    page_open(page_apple(), NULL);
}
