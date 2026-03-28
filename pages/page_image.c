#include "page_image.h"

typedef struct
{
    BasePage base;
    lv_obj_t * img;
    lv_obj_t * btn_back;
    lv_obj_t * slider_scale;
    lv_obj_t * touch_area;
    lv_coord_t touch_last_x;
    lv_coord_t touch_last_y;
    lv_img_header_t header;
    lv_coord_t bound_x;
    lv_coord_t bound_y;
    double scale_default;
    bool touching;
    bool moved;
    bool ui_hidden;
} ImagePage;

static lv_obj_t * page_image_obj(ImagePage * page, char * src);
static void back_click(lv_event_t * e);
static void slider_scale_changed(lv_event_t * e);
static void touch_pressing(lv_event_t * e);
static void page_image_destroy(void * p);

BasePage * page_image_create(char * filename)
{
    ImagePage * page = malloc(sizeof(ImagePage));
    if(!page) return NULL;
    memset(page, 0, sizeof(ImagePage));

    page->base.obj        = page_image_obj(page, filename);
    page->base.on_destroy = page_image_destroy;
    return (BasePage *)page;
}

static lv_obj_t * page_image_obj(ImagePage * page, char * src)
{
    lv_obj_t * screen = lv_obj_create(lv_scr_act());
    lv_obj_remove_style_all(screen);
    lv_obj_set_size(screen, lv_pct(100), lv_pct(100));
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

    page->touching  = false;
    page->ui_hidden = false;

    ffmpeg_get_img_header(src, &page->header);

    lv_obj_t * img = lv_img_create(screen);
    page->img      = img;
    lv_obj_set_size(img, page->header.w, page->header.h);
    lv_obj_center(img);
    if(page->header.w != 0 && page->header.h != 0) {
        lv_img_set_src(img, src);

        double scale_default = fmin((double)LV_SCR_WIDTH / page->header.w, (double)LV_SCR_HEIGHT / page->header.h);
        page->scale_default  = scale_default;

        lv_coord_t img_scaled_w = (lv_coord_t)(page->header.w * scale_default);
        lv_coord_t img_scaled_h = (lv_coord_t)(page->header.h * scale_default);

        page->bound_x = fmax(0, (img_scaled_w - LV_SCR_WIDTH) / 2);
        page->bound_y = fmax(0, (img_scaled_h - LV_SCR_HEIGHT) / 2);

        lv_obj_set_style_transform_zoom(img, (lv_coord_t)(256 * scale_default), 0);
        LV_LOG_USER("[page_image]%dx%d, scale=%.8g\n", page->header.w, page->header.h, scale_default);
    }

    lv_obj_t * touch_area = lv_obj_create(screen);
    page->touch_area      = touch_area;
    lv_obj_set_size(touch_area, LV_SCR_WIDTH, LV_SCR_HEIGHT);
    lv_obj_set_pos(touch_area, 0, 0);
    lv_obj_set_style_bg_opa(touch_area, LV_OPA_0, 0);
    lv_obj_set_style_border_opa(touch_area, LV_OPA_0, 0);
    lv_obj_set_style_outline_opa(touch_area, LV_OPA_0, 0);
    lv_obj_set_style_shadow_opa(touch_area, LV_OPA_0, 0);
    lv_obj_add_event_cb(touch_area, touch_pressing, LV_EVENT_PRESSED, page);
    lv_obj_add_event_cb(touch_area, touch_pressing, LV_EVENT_PRESSING, page);
    lv_obj_add_event_cb(touch_area, touch_pressing, LV_EVENT_RELEASED, page);
    lv_obj_add_event_cb(touch_area, touch_pressing, LV_EVENT_CLICKED, page);

    lv_obj_t * slider_scale = lv_slider_create(screen);
    page->slider_scale      = slider_scale;
    lv_obj_set_size(slider_scale, lv_pct(10), lv_pct(80));
    lv_obj_align(slider_scale, LV_ALIGN_LEFT_MID, lv_pct(90), 0);
    lv_slider_set_range(slider_scale, 0, 100);
    lv_slider_set_value(slider_scale, 0, LV_ANIM_OFF);
    lv_obj_add_event_cb(slider_scale, slider_scale_changed, LV_EVENT_VALUE_CHANGED, page);

    lv_obj_t * btn_back = lv_btn_create(screen);
    page->btn_back      = btn_back;
    lv_obj_set_size(btn_back, lv_pct(25), lv_pct(12));
    lv_obj_align(btn_back, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_t * btn_back_label = lv_label_create(btn_back);
    lv_label_set_text(btn_back_label, CUSTOM_SYMBOL_BACK "");
    lv_obj_center(btn_back_label);
    lv_obj_add_event_cb(btn_back, back_click, LV_EVENT_CLICKED, NULL);

    return screen;
}

static void slider_scale_changed(lv_event_t * e)
{
    ImagePage * page = (ImagePage *)e->user_data;
    if(!page) return;

    lv_obj_t * img = page->img;

    lv_obj_t * slider = lv_event_get_target(e);
    int value         = lv_slider_get_value(slider);
    double scale_new  = pow(2, log2(10) * value / 100) * page->scale_default;

    lv_coord_t img_scaled_w = (lv_coord_t)(page->header.w * scale_new);
    lv_coord_t img_scaled_h = (lv_coord_t)(page->header.h * scale_new);
    lv_obj_set_style_transform_zoom(img, (int)(256 * scale_new), 0);

    lv_coord_t img_x = lv_obj_get_x_aligned(img);
    lv_coord_t img_y = lv_obj_get_y_aligned(img);

    lv_coord_t bound_x = fmax(0, (img_scaled_w - LV_SCR_WIDTH) / 2);
    lv_coord_t bound_y = fmax(0, (img_scaled_h - LV_SCR_HEIGHT) / 2);
    page->bound_x      = bound_x;
    page->bound_y      = bound_y;

    lv_obj_set_pos(img, fmax(fmin(img_x, 0 + bound_x), 0 - bound_x), fmax(fmin(img_y, 0 + bound_y), 0 - bound_y));
}

static void touch_pressing(lv_event_t * e)
{
    ImagePage * page = (ImagePage *)e->user_data;
    if(!page) return;

    // 这些在函数中不需要进行修改，可以直接一次性拉出来
    lv_obj_t * img          = page->img;
    lv_coord_t bound_x      = page->bound_x;
    lv_coord_t bound_y      = page->bound_y;

    lv_point_t point;
    lv_indev_get_point(lv_indev_get_act(), &point);

    switch(e->code) {
        case LV_EVENT_PRESSED:
            page->touching     = true;
            page->moved        = false;
            page->touch_last_x = point.x;
            page->touch_last_y = point.y;
            break;

        case LV_EVENT_PRESSING:
            if(page->touching) {
                lv_coord_t dx = point.x - page->touch_last_x;
                lv_coord_t dy = point.y - page->touch_last_y;
                if(abs(dx) > 0 || abs(dy) > 0) {
                    page->moved = true;

                    lv_coord_t img_x = lv_obj_get_x_aligned(img) + dx;
                    lv_coord_t img_y = lv_obj_get_y_aligned(img) + dy;

                    lv_obj_set_pos(img, fmax(fmin(img_x, 0 + bound_x), 0 - bound_x),
                                   fmax(fmin(img_y, 0 + bound_y), 0 - bound_y));
                }
                page->touch_last_x = point.x;
                page->touch_last_y = point.y;
            }
            break;

        case LV_EVENT_RELEASED:
            if(!page->moved) {
                lv_obj_t * btn_back     = page->btn_back;
                lv_obj_t * slider_scale = page->slider_scale;

                page->ui_hidden = !page->ui_hidden;
                if(page->ui_hidden) {
                    lv_obj_add_flag(btn_back, LV_OBJ_FLAG_HIDDEN);
                    lv_obj_add_flag(slider_scale, LV_OBJ_FLAG_HIDDEN);
                } else {
                    lv_obj_clear_flag(btn_back, LV_OBJ_FLAG_HIDDEN);
                    lv_obj_clear_flag(slider_scale, LV_OBJ_FLAG_HIDDEN);
                }
            }
            page->touching = false;
            break;

        default: break;
    }
}

static void back_click(lv_event_t * e)
{
    page_back();
}

static void page_image_destroy(void * p)
{
    lv_img_cache_invalidate_src(NULL);
}
