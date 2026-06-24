#include "page_file_manager.h"

#include "page_apple.h"
#include "page_audio.h"
#include "page_midi.h"
#include "page_image.h"
#include "page_txt.h"
#include "platform/str_utils.h"
#include "views/custom_msgbox.h"

typedef struct
{
    BasePage base;
    lv_obj_t * file_explorer;
    lv_obj_t * container_act;
    lv_obj_t * label_filename;
} FileManagerPage;

static const char * btn_txts[] = {"YES", "NO", NULL};

static lv_obj_t * page_file_manager_obj(FileManagerPage * page);
static void explorer_event_handler(lv_event_t * e);
static void back_click(lv_event_t * e);
static void container_act_click(lv_event_t * e);
static void act_delete_click(lv_event_t * e);
static void act_msgbox_delete(lv_event_t * e);
static bool page_file_manager_on_key(void * p, key_code_t key_code, key_action_t key_action);

BasePage * page_file_manager_create(void)
{
    FileManagerPage * page = malloc(sizeof(FileManagerPage));
    if(!page) return NULL;
    memset(page, 0, sizeof(FileManagerPage));

    page->base.obj        = page_file_manager_obj(page);
    page->base.on_key     = page_file_manager_on_key;
    return (BasePage *)page;
}

lv_obj_t * page_file_manager_obj(FileManagerPage * page)
{
    lv_obj_t * screen = lv_obj_create(lv_scr_act());
    lv_obj_remove_style_all(screen);
    lv_obj_set_size(screen, lv_pct(100), lv_pct(100));

    lv_obj_t * file_explorer = lv_100ask_file_explorer_create(screen);
    lv_obj_add_event_cb(file_explorer, explorer_event_handler, 
							LV_EVENT_ALL, page);
    lv_100ask_file_explorer_open_dir(file_explorer, "//mnt");
    page->file_explorer = file_explorer;

    lv_obj_t * btn_back = lv_btn_create(screen);
    lv_obj_set_size(btn_back, lv_pct(25), lv_pct(12));
    lv_obj_align(btn_back, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_t * btn_back_label = lv_label_create(btn_back);
    lv_label_set_text(btn_back_label, CUSTOM_SYMBOL_BACK "");
    lv_obj_center(btn_back_label);
    lv_obj_add_event_cb(btn_back, back_click, LV_EVENT_CLICKED, page);

    lv_obj_t * container_act = lv_obj_create(screen);
    lv_obj_remove_style_all(container_act);
    lv_obj_clear_flag(container_act, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_align(container_act, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_size(container_act, lv_pct(100), lv_pct(88));
    lv_obj_set_style_bg_opa(container_act, LV_OPA_60, LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(container_act, lv_color_black(), LV_STATE_DEFAULT);
    lv_obj_add_flag(container_act, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_event_cb(container_act, container_act_click, LV_EVENT_CLICKED, page);
    page->container_act = container_act;

    lv_obj_t * list_act = lv_obj_create(container_act);
    lv_obj_align(list_act, LV_ALIGN_TOP_RIGHT, 0, 0);
    lv_obj_set_size(list_act, lv_pct(75), lv_pct(100));
    lv_obj_set_flex_flow(list_act, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_scroll_dir(list_act, LV_DIR_VER);

    lv_obj_t * label_filename = lv_label_create(list_act);
    lv_label_set_long_mode(label_filename, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_width(label_filename, lv_pct(100));
    page->label_filename = label_filename;

    lv_obj_t * btn_delete = lv_btn_create(list_act);
    lv_obj_set_size(btn_delete, lv_pct(100), lv_pct(22));
    lv_obj_set_style_bg_color(btn_delete, lv_palette_main(LV_PALETTE_RED), LV_STATE_DEFAULT);
    lv_obj_t * btn_label_delete = lv_label_create(btn_delete);
    lv_label_set_text(btn_label_delete, "Delete");
    lv_obj_center(btn_label_delete);
    lv_obj_add_event_cb(btn_delete, act_delete_click, LV_EVENT_CLICKED, page);

    return screen;
}

static void explorer_event_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj       = lv_event_get_target(e);
    FileManagerPage * page = (FileManagerPage *)e->user_data;
    char file_name[LV_100ASK_FILE_EXPLORER_PATH_MAX_LEN];

    char * cur_path = lv_100ask_file_explorer_get_cur_path(obj) + 1;
    char * sel_fn   = lv_100ask_file_explorer_get_sel_fn(obj);
    lv_snprintf(file_name, sizeof(file_name), "%s%s", cur_path, sel_fn);

    if(code == LV_EVENT_CLICKED) {
        printf("[file_manager] clicked %s\n", file_name);

        if(str_end_with(file_name, ".png", false) || str_end_with(file_name, ".jpg", false) ||
            str_end_with(file_name, ".jpeg", false) || str_end_with(file_name, ".bmp", false) ||
            str_end_with(file_name, ".gif", false)) 
            {
                page_open(page_image_create(file_name));
            }

        if(str_end_with(file_name, ".mp3", false) || str_end_with(file_name, ".wav", false) ||
            str_end_with(file_name, ".ogg", false) || str_end_with(file_name, ".m4a", false) ||
            str_end_with(file_name, ".aac", false) || str_end_with(file_name, ".pcm", false))
            {
                page_open(page_audio_create(file_name));
            }
            
        if(str_end_with(file_name, ".mp4", false) || str_end_with(file_name, ".avi", false)) {
            page_open(page_video_create(file_name));
        }

        if(str_end_with(file_name, ".mid", false) || str_end_with(file_name, ".midi", false)) {
            page_open(page_midi_create(file_name));
        }

        if(str_end_with(file_name, ".txt", false) || str_end_with(file_name, ".json", false) || 
            str_end_with(file_name, ".conf", false) || str_end_with(file_name, ".log", false) ||
            str_end_with(file_name, ".cfg", false)) {
            page_open(page_txt_create(file_name));
        }   
    }

    if(code == LV_EVENT_LONG_PRESSED) {
        printf("[file_manager] long-pressed %s\n", file_name);
        lv_label_set_text(page->label_filename, sel_fn);
        lv_obj_clear_flag(page->container_act, LV_OBJ_FLAG_HIDDEN);
    }
}

static void act_delete_click(lv_event_t * e)
{
    FileManagerPage * page = (FileManagerPage *)e->user_data;
    lv_obj_t * mbox = custom_msgbox_create("Delete File:", 
                lv_100ask_file_explorer_get_sel_fn(page->file_explorer),
                btn_txts, false);
    lv_obj_add_event_cb(mbox, act_msgbox_delete, LV_EVENT_VALUE_CHANGED, page);
}

static void act_msgbox_delete(lv_event_t * e)
{
    FileManagerPage * page = (FileManagerPage *)e->user_data;
    lv_obj_t * file_explorer = page->file_explorer;
    // 请注意：这里的消息是冒泡上来的，target获取到的是里面的btn_matrix
    lv_obj_t * msgbox = lv_obj_get_parent(lv_event_get_target(e));
    char * txt = lv_msgbox_get_active_btn_text(msgbox);

    if (strcmp(txt, "YES") == 0) {
        // file_explorer获取的路径形如 "//mnt/UDISK/lvgl/"
        // 将其指针向前移动一位，路径变为 "/mnt/UDISK/lvgl/"
        char * cur_path = lv_100ask_file_explorer_get_cur_path(file_explorer) + 1;
        char * sel_fn   = lv_100ask_file_explorer_get_sel_fn(file_explorer);

        if((str_begin_with(cur_path, "/mnt/UDISK/", true) || str_begin_with(cur_path, "/mnt/app/dendro/", true)
            || str_begin_with(cur_path, "/mnt/sdcard/", true)) && !str_begin_with(cur_path, "/mnt/UDISK/lvgl", true)) {

            int cmd_length = 10 + strlen(cur_path) + strlen(sel_fn);
            char * cmd = malloc(cmd_length);
            if (cmd == NULL) {
                perror("malloc");
                exit(EXIT_FAILURE);
            }
            lv_snprintf(cmd, cmd_length, "rm -rf \"%s%s\"", cur_path, sel_fn);
            system(cmd);
            printf("[file_manager] %s\n", cmd);
            free(cmd);

            // 前面获取的指针直接传进去会出问题，所以要临时拷贝一下
            // cur_path最后的"/"符号要去掉，这样整个字符串长度加上\0刚好是1 + strlen(cur_path)
            int path_length = 1 + strlen(cur_path);
            char * path = malloc(path_length);
            if (path == NULL) {
                perror("malloc");
                exit(EXIT_FAILURE);
            }
            lv_snprintf(path, path_length, "/%s", cur_path);
            lv_100ask_file_explorer_open_dir(file_explorer, path);
            free(path);
            lv_obj_add_flag(page->container_act, LV_OBJ_FLAG_HIDDEN);
            lv_msgbox_close_async(msgbox);
        }
        else {
            custom_toast_create("Action Not Allowed!");
        }
    }
    else {
        lv_msgbox_close_async(msgbox);
    }
}

static void back_click(lv_event_t * e)
{
    FileManagerPage * page = (FileManagerPage *)e->user_data;
    if(!lv_obj_has_flag(page->container_act, LV_OBJ_FLAG_HIDDEN)) 
        lv_obj_add_flag(page->container_act, LV_OBJ_FLAG_HIDDEN);
    else page_back();
}

static void container_act_click(lv_event_t * e)
{
    FileManagerPage * page = (FileManagerPage *)e->user_data;
    lv_obj_add_flag(page->container_act, LV_OBJ_FLAG_HIDDEN);
}

static bool page_file_manager_on_key(void * p, key_code_t key_code, key_action_t key_action)
{
    if(!p) return false;
    if(key_code != KEY_CODE_HOME) return false;
    if(key_action == KEY_ACTION_DOWN) return true;
    // KEY_CODE_HOME & KEY_ACTION_UP

    FileManagerPage * page = (FileManagerPage *)p;

    if(!lv_obj_has_flag(page->container_act, LV_OBJ_FLAG_HIDDEN)) {
        lv_obj_add_flag(page->container_act, LV_OBJ_FLAG_HIDDEN);
        return true;
    }

    char * cur_path        = lv_100ask_file_explorer_get_cur_path(page->file_explorer);

    //printf("%s\n", cur_path);
    if(strcmp(cur_path, "//") == 0) return false;

    char parent_path[LV_100ASK_FILE_EXPLORER_PATH_MAX_LEN];
    strcpy(parent_path, cur_path);

    // 路径的最后还有一个斜杠，需要截取两次
    char * last_slash = strrchr(parent_path, '/');
    if(last_slash) *last_slash = '\0';
    last_slash = strrchr(parent_path, '/');
    if(last_slash) *last_slash = '\0';
    
    lv_100ask_file_explorer_open_dir(page->file_explorer, parent_path);

    return true;
}