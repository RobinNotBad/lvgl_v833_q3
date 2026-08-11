#include "page_demo.h"

#include "views/ime_helper.h"
#include "cJSON/json_path_tool.h"
#include "cJSON/cJSON.h"

typedef struct {
    BasePage base;

} DemoPage;

static lv_obj_t * page_demo_obj(DemoPage * page);
static void btn_back_click(lv_event_t * e);

BasePage * demo_page_create(void)
{
    DemoPage * page = malloc(sizeof(DemoPage));
    if(!page) return NULL;
    memset(page, 0, sizeof(DemoPage));
    page->base.obj  = page_demo_obj(page);
	
    return (BasePage *)page;
}

static lv_obj_t * page_demo_obj(DemoPage * page) {
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

    lv_obj_t * textarea1 = lv_textarea_create(container);
    lv_obj_set_size(textarea1, lv_pct(100), LV_SIZE_CONTENT);
    lv_textarea_bind_ime(textarea1);

    lv_obj_t * textarea2 = lv_textarea_create(container);
    lv_obj_set_size(textarea2, lv_pct(100), LV_SIZE_CONTENT);
    lv_textarea_bind_ime(textarea2);
    
    return screen;
}


static void btn_back_click(lv_event_t * e)
{
    page_back();
}

