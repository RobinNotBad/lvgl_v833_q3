#include "page_file_manager.h"

#include "page_apple.h"
#include "page_audio.h"
#include "page_midi.h"
#include "page_image.h"

static void explorer_event_handler(lv_event_t * e);
static void back_click(lv_event_t * e);

lv_obj_t * page_file_manager(void)
{
    lv_obj_t * screen = lv_obj_create(lv_scr_act());
    lv_obj_remove_style_all(screen);
    lv_obj_set_size(screen, lv_pct(100), lv_pct(100));

    lv_obj_t * file_explorer = lv_100ask_file_explorer_create(screen);
    lv_obj_add_event_cb(file_explorer, explorer_event_handler, 
							LV_EVENT_VALUE_CHANGED, NULL);
    lv_100ask_file_explorer_open_dir(file_explorer, "//mnt");

    lv_obj_t * btn_back = lv_btn_create(screen);
    lv_obj_set_size(btn_back, lv_pct(25), lv_pct(12));
    lv_obj_align(btn_back, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_t * btn_back_label = lv_label_create(btn_back);
    lv_label_set_text(btn_back_label, CUSTOM_SYMBOL_BACK "");
    lv_obj_center(btn_back_label);
    lv_obj_add_event_cb(btn_back, back_click, LV_EVENT_CLICKED, NULL);

    return screen;
}

static void explorer_event_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj       = lv_event_get_target(e);

    if(code == LV_EVENT_VALUE_CHANGED) {
        char * cur_path = lv_100ask_file_explorer_get_cur_path(obj);
        char * sel_fn   = lv_100ask_file_explorer_get_sel_fn(obj);
        char file_name[LV_100ASK_FILE_EXPLORER_PATH_MAX_LEN];

        lv_snprintf(file_name, sizeof(file_name), "%s%s", cur_path+1, sel_fn);

        printf("%s\n", file_name);

        if(str_end_with(file_name, ".png", false) || str_end_with(file_name, ".jpg", false) ||
            str_end_with(file_name, ".jpeg", false) || str_end_with(file_name, ".bmp", false) ||
            str_end_with(file_name, ".gif", false)) 
            {
                page_open(page_image_create(&file_name));
            }

        if(str_end_with(file_name, ".mp3", false) || str_end_with(file_name, ".wav", false) ||
            str_end_with(file_name, ".ogg", false) || str_end_with(file_name, ".m4a", false) ||
            str_end_with(file_name, ".aac", false) || str_end_with(file_name, ".pcm", false))
            {
                page_open(page_audio_create(&file_name));
            }
            
        if(str_end_with(file_name, ".mp4", false)) {
            page_open(page_video_create(&file_name));
        }

        if(str_end_with(file_name, ".mid", false) || str_end_with(file_name, ".midi", false)) {
            page_open(page_midi_create(&file_name));
        }
        }
}

static void back_click(lv_event_t * e)
{
    page_back();
}