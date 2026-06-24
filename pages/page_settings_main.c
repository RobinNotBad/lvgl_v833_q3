#include "page_settings_main.h"

#include "page_demo.h"
#include "main.h"
#include "platform/str_utils.h"
#include "platform/audio_ctrl.h"
#include "platform/config_manager.h"
#include "views/custom_msgbox.h"

static void btn_demo_click(lv_event_t * e);
static void btn_about_click(lv_event_t * e);
static void btn_developer_click(lv_event_t * e);
static void btn_back_click(lv_event_t * e);
static void slider_brightness_changed(lv_event_t * e);
static void slider_brightness_set(lv_event_t * e);
static void slider_volume_changed(lv_event_t * e);
static void slider_volume_set(lv_event_t * e);

lv_obj_t * page_settings_main(void)
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
    lv_obj_update_layout(container);

    lv_obj_t * btn_back = lv_btn_create(screen);
    lv_obj_set_size(btn_back, lv_pct(25), lv_pct(12));
    lv_obj_align(btn_back, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_t * btn_back_label = lv_label_create(btn_back);
    lv_label_set_text(btn_back_label, CUSTOM_SYMBOL_BACK "");
    lv_obj_center(btn_back_label);
    lv_obj_add_event_cb(btn_back, btn_back_click, LV_EVENT_CLICKED, NULL);

    lv_obj_t * label_brightness = lv_label_create(container);
    lv_label_set_text(label_brightness, "Brightness");
    lv_obj_t * slider_brightness = lv_slider_create(container);
	lv_obj_set_size(slider_brightness, lv_pct(80), lv_pct(10));
    lv_obj_set_style_translate_x(slider_brightness, lv_obj_get_width_pct(container, 5), LV_STATE_DEFAULT);
	lv_slider_set_range(slider_brightness, 1, 255);
    lv_slider_set_value(slider_brightness, lcd_get_brightness(), LV_ANIM_OFF);
	lv_obj_add_event_cb(slider_brightness, slider_brightness_changed, LV_EVENT_VALUE_CHANGED, NULL);
	lv_obj_add_event_cb(slider_brightness, slider_brightness_set, LV_EVENT_RELEASED, NULL);

    lv_obj_t * label_volume = lv_label_create(container);
    lv_label_set_text(label_volume, "Volume");
    lv_obj_t * slider_volume = lv_slider_create(container);
	lv_obj_set_size(slider_volume, lv_pct(80), lv_pct(10));
    lv_obj_set_style_translate_x(slider_volume, lv_obj_get_width_pct(container, 5), LV_STATE_DEFAULT);
    lv_slider_set_range(slider_volume, 0, 100);
    lv_slider_set_value(slider_volume, audio_volume_get(), LV_ANIM_OFF);
	lv_obj_add_event_cb(slider_volume, slider_volume_changed, LV_EVENT_VALUE_CHANGED, NULL);
	lv_obj_add_event_cb(slider_volume, slider_volume_set, LV_EVENT_RELEASED, NULL);

    lv_obj_t * btn_demo = lv_btn_create(container);
    lv_obj_set_size(btn_demo, lv_pct(64), lv_pct(32));
    lv_obj_align(btn_demo, LV_FLEX_ALIGN_START, lv_pct(10), 0);
    lv_obj_t * btn_label_demo = lv_label_create(btn_demo);
    lv_label_set_text(btn_label_demo, "Test");
    lv_obj_center(btn_label_demo);
    lv_obj_add_event_cb(btn_demo, btn_demo_click, LV_EVENT_CLICKED, NULL);

    lv_obj_t * btn_about = lv_btn_create(container);
    lv_obj_set_size(btn_about, lv_pct(64), lv_pct(32));
    lv_obj_align(btn_about, LV_FLEX_ALIGN_START, lv_pct(10), 0);
    lv_obj_t * btn_label_about = lv_label_create(btn_about);
    lv_label_set_text(btn_label_about, "About Dendro");
    lv_obj_center(btn_label_about);
    lv_obj_add_event_cb(btn_about, btn_about_click, LV_EVENT_CLICKED, NULL);

    lv_obj_t * btn_developer = lv_btn_create(container);
    lv_obj_set_size(btn_developer, lv_pct(64), lv_pct(32));
    lv_obj_align(btn_developer, LV_FLEX_ALIGN_START, lv_pct(10), 0);
    lv_obj_t * btn_label_developer = lv_label_create(btn_developer);
    lv_label_set_text(btn_label_developer, "Developers");
    lv_obj_center(btn_label_developer);
    lv_obj_add_event_cb(btn_developer, btn_developer_click, LV_EVENT_CLICKED, NULL);

    return screen;
}

static void btn_demo_click(lv_event_t * e) // static可以防止同名冲突
{
    custom_toast_create("May all the beauty be blessed.\nMay all the dreams shine with their light.");
    //page_open(demo_page_create());
}

static void btn_about_click(lv_event_t * e)
{
    custom_msgbox_create("About", "Dendro is a simple multimedia provider program with a GUI, targeting unusual Linux devices.", NULL, true);
}

static void btn_developer_click(lv_event_t * e)
{
    custom_msgbox_create("Developers", "Main Developer: RobinNotBad\nOpen Source: https://github.com/RobinNotBad/lvgl_v833_q3", NULL, true);
}

static void slider_brightness_changed(lv_event_t * e)
{
    lv_obj_t * slider = lv_event_get_target(e);
    int value = lv_slider_get_value(slider);
    lcd_set_brightness(value);
}

static void slider_brightness_set(lv_event_t * e)
{
    lv_obj_t * slider = lv_event_get_target(e);
    int value = lv_slider_get_value(slider);
    config_write_int(MAIN_CONFIG_FILE, CFG_BRIGHTNESS, value);
}

static void slider_volume_changed(lv_event_t * e)
{
    lv_obj_t * slider = lv_event_get_target(e);
    int value = lv_slider_get_value(slider);
    audio_volume_set(value);
}

static void slider_volume_set(lv_event_t * e)
{
    lv_obj_t * slider = lv_event_get_target(e);
    int value = lv_slider_get_value(slider);
    config_write_int(MAIN_CONFIG_FILE, CFG_VOLUME, value);
}

static void btn_back_click(lv_event_t * e)
{
    page_back();
}
