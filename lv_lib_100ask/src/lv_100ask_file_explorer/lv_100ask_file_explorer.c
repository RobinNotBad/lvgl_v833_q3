/**
 * @file lv_100ask_file_explorer.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "lv_100ask_file_explorer.h"

#if LV_USE_100ASK_FILE_EXPLORER != 0
#include <string.h>

/*********************
 *      DEFINES
 *********************/
#define MY_CLASS &lv_100ask_file_explorer_class

#define FILE_EXPLORER_HEAD_HEIGHT               (60)
#define FILE_EXPLORER_QUICK_ACCESS_AREA_WIDTH   (22)
#define FILE_EXPLORER_BROWER_AREA_WIDTH         (100 - FILE_EXPLORER_QUICK_ACCESS_AREA_WIDTH)

// 我不想引用我自己的头文件，尽量保持独立性，所以这里也定义了一遍
static const char * AUDIO_FILE_EXT[] = {".mp3", ".wav", ".ogg", ".m4a", ".aac", ".pcm", NULL};
static const char * IMAGE_FILE_EXT[] = {".png", ".jpg", ".jpeg", ".bmp", ".gif", NULL};
static const char * VIDEO_FILE_EXT[] = {".mp4", ".avi", ".mov", ".mkv", ".flv", ".rm", ".rmvb", NULL};
static const char * MIDI_FILE_EXT[] = {".mid", ".midi", NULL};
static const char * TEXT_FILE_EXT[] = {".txt", ".json", ".conf", ".log", ".cfg", NULL};

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void lv_100ask_file_explorer_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj);
static void lv_100ask_file_explorer_destructor(const lv_obj_class_t * class_p, lv_obj_t * obj);

static void brower_file_event_handler(lv_event_t * e);
#if LV_100ASK_FILE_EXPLORER_QUICK_ACCESS
    static void quick_access_event_handler(lv_event_t * e);
    static void quick_access_ctrl_btn_event_handler(lv_event_t * e);
#endif

static void init_style(lv_obj_t * obj);
static void show_dir(lv_obj_t * obj, char * path);
static void strip_ext(char * dir);
static void sort_table_by_name(lv_obj_t * tb, int16_t lo, int16_t hi);
static void exch_table_item(lv_obj_t * tb, int16_t i, int16_t j);

static bool file_ext_match(const char * file_name, const char * file_ext[]);
static bool is_begin_with(const char * str1, const char * str2, bool case_sensitivity);
static bool is_end_with(const char * str1, const char * str2, bool case_sensitivity);

static char to_upper_case(char c);
static bool is_lower_letter(char c);
static bool is_upper_letter(char c);

/**********************
 *  STATIC VARIABLES
 **********************/
const lv_obj_class_t lv_100ask_file_explorer_class = {
    .constructor_cb = lv_100ask_file_explorer_constructor,
    .destructor_cb  = lv_100ask_file_explorer_destructor,
    .width_def      = LV_SIZE_CONTENT,
    .height_def     = LV_SIZE_CONTENT,
    .instance_size  = sizeof(lv_100ask_file_explorer_t),
    .base_class     = &lv_obj_class
};

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

lv_obj_t * lv_100ask_file_explorer_create(lv_obj_t * parent)
{
    LV_LOG_INFO("begin");
    lv_obj_t * obj = lv_obj_class_create_obj(MY_CLASS, parent);
    lv_obj_class_init_obj(obj);
    return obj;
}


/*=====================
 * Setter functions
 *====================*/
#if LV_100ASK_FILE_EXPLORER_QUICK_ACCESS
void lv_100ask_file_explorer_set_quick_access_path(lv_obj_t * obj, lv_100ask_file_explorer_dir_t dir, char * path)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_100ask_file_explorer_t * explorer = (lv_100ask_file_explorer_t *)obj;

    /*If path is unavailable */
    if((path == NULL) || (strlen(path) <= 0)) return;

    char ** dir_str = NULL;
    switch (dir)
    {
        case LV_100ASK_EXPLORER_HOME_DIR:
            dir_str = &(explorer->home_dir);
            break;
        case LV_100ASK_EXPLORER_MUSIC_DIR:
            dir_str = &(explorer->music_dir);
            break;
        case LV_100ASK_EXPLORER_PICTURES_DIR:
            dir_str = &(explorer->pictures_dir);
            break;
        case LV_100ASK_EXPLORER_VIDEO_DIR:
            dir_str = &(explorer->video_dir);
            break;
        case LV_100ASK_EXPLORER_DOCS_DIR:
            dir_str = &(explorer->docs_dir);
            break;
        //case LV_100ASK_EXPLORER_MNT_DIR:
        //    dir_str = &(explorer->home_dir);
        //    break;
        //case LV_100ASK_EXPLORER_FS_DIR:
        //    dir_str = &(explorer->home_dir);
        //    break;

        default:
            return;
            break;
    }

    /*Free the old text*/
    if(*dir_str != NULL) {
        lv_mem_free(*dir_str);
        *dir_str = NULL;
    }

    /*Get the size of the text*/
    size_t len = strlen(path) + 1;

    /*Allocate space for the new text*/
    *dir_str = lv_mem_alloc(len);
    LV_ASSERT_MALLOC(*dir_str);
    if(*dir_str == NULL) return;

    strcpy(*dir_str, path);
}


void lv_100ask_file_explorer_set_quick_access_state(lv_obj_t * obj, bool state)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_100ask_file_explorer_t * explorer = (lv_100ask_file_explorer_t *)obj;

    if(state != (lv_obj_has_state(explorer->quick_access_ctrl_btn, LV_STATE_CHECKED)))
        return;

    if (!lv_obj_has_state(explorer->quick_access_ctrl_btn, LV_STATE_CHECKED))
        lv_obj_add_state(explorer->quick_access_ctrl_btn, LV_STATE_CHECKED);
    else lv_obj_clear_state(explorer->quick_access_ctrl_btn, LV_STATE_CHECKED);

    lv_event_send(explorer->quick_access_ctrl_btn, LV_EVENT_VALUE_CHANGED, NULL);
}
#endif


/*=====================
 * Getter functions
 *====================*/
char * lv_100ask_file_explorer_get_sel_fn(lv_obj_t * obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_100ask_file_explorer_t * explorer = (lv_100ask_file_explorer_t *)obj;

    return explorer->sel_fp;
}

char * lv_100ask_file_explorer_get_cur_path(lv_obj_t * obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_100ask_file_explorer_t * explorer = (lv_100ask_file_explorer_t *)obj;

    return explorer->cur_path;
}

lv_obj_t * lv_100ask_file_explorer_get_file_list(lv_obj_t * obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_100ask_file_explorer_t * explorer = (lv_100ask_file_explorer_t *)obj;

    return explorer->file_list;
}

lv_obj_t * lv_100ask_file_explorer_get_head(lv_obj_t * obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_100ask_file_explorer_t * explorer = (lv_100ask_file_explorer_t *)obj;

    return explorer->head_area;
}

lv_obj_t * lv_100ask_file_explorer_get_path_obj(lv_obj_t * obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_100ask_file_explorer_t * explorer = (lv_100ask_file_explorer_t *)obj;

    return explorer->path_label;
}

#if LV_100ASK_FILE_EXPLORER_QUICK_ACCESS
lv_obj_t * lv_100ask_file_explorer_get_places_list(lv_obj_t * obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_100ask_file_explorer_t * explorer = (lv_100ask_file_explorer_t *)obj;

    return explorer->list_places;
}

lv_obj_t * lv_100ask_file_explorer_get_device_list(lv_obj_t * obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_100ask_file_explorer_t * explorer = (lv_100ask_file_explorer_t *)obj;

    return explorer->list_device;
}

lv_obj_t * lv_100ask_file_explorer_get_quick_access_ctrl_btn(lv_obj_t * obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_100ask_file_explorer_t * explorer = (lv_100ask_file_explorer_t *)obj;

    return explorer->quick_access_ctrl_btn;
}
#endif


/*=====================
 * Other functions
 *====================*/
void lv_100ask_file_explorer_open_dir(lv_obj_t * obj, char * dir)
{
    //lv_100ask_file_explorer_t * explorer = (lv_100ask_file_explorer_t *)obj;

    show_dir(obj, dir);
}

void lv_100ask_file_explorer_refresh(lv_obj_t * obj) 
{
    //lv_100ask_file_explorer_t * explorer = (lv_100ask_file_explorer_t *)obj;
    // 获取的指针直接传进去会出问题，所以要临时拷贝一下
    char * cur_path = lv_100ask_file_explorer_get_cur_path(obj);
    char * path     = strdup(cur_path);
    if(path == NULL) return;
    path[strlen(path) - 1] = '\0'; // 最后的"/"符号要去掉
    lv_100ask_file_explorer_open_dir(obj, path);
    lv_mem_free(path);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/
static void lv_100ask_file_explorer_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj)
{
    LV_UNUSED(class_p);
    LV_TRACE_OBJ_CREATE("begin");

    lv_100ask_file_explorer_t * explorer = (lv_100ask_file_explorer_t *)obj;

#if LV_100ASK_FILE_EXPLORER_QUICK_ACCESS
    explorer->home_dir = NULL;
    explorer->video_dir = NULL;
    explorer->pictures_dir = NULL;
    explorer->music_dir = NULL;
    explorer->docs_dir = NULL;
#endif

    lv_memset_00(explorer->cur_path, sizeof(explorer->cur_path));

    lv_obj_set_size(obj, LV_PCT(100), LV_PCT(100));
    lv_obj_set_flex_flow(obj, LV_FLEX_FLOW_COLUMN);

    explorer->cont = lv_obj_create(obj);
    lv_obj_set_width(explorer->cont, LV_PCT(100));
    lv_obj_set_flex_grow(explorer->cont, 1);

#if LV_100ASK_FILE_EXPLORER_QUICK_ACCESS
    // 左侧快速访问栏区域
    explorer->quick_access_area = lv_obj_create(explorer->cont);
    lv_obj_set_size(explorer->quick_access_area, LV_PCT(FILE_EXPLORER_QUICK_ACCESS_AREA_WIDTH), LV_PCT(100));
    lv_obj_set_flex_flow(explorer->quick_access_area, LV_FLEX_FLOW_COLUMN);
#endif

    /* 右侧文件浏览区域 */
    explorer->browser_area = lv_obj_create(explorer->cont);
#if LV_100ASK_FILE_EXPLORER_QUICK_ACCESS
    lv_obj_set_size(explorer->browser_area, LV_PCT(FILE_EXPLORER_BROWER_AREA_WIDTH), LV_PCT(100));
#else
   lv_obj_set_size(explorer->browser_area, LV_PCT(100), LV_PCT(100));
#endif
    lv_obj_set_flex_flow(explorer->browser_area, LV_FLEX_FLOW_COLUMN);

    // 展示在文件浏览列表之上的区域(head)
    explorer->head_area = lv_obj_create(explorer->browser_area);
    lv_obj_set_size(explorer->head_area, LV_PCT(100), LV_PCT(12));
    lv_obj_clear_flag(explorer->head_area, LV_OBJ_FLAG_SCROLLABLE);

#if LV_100ASK_FILE_EXPLORER_QUICK_ACCESS
    // 快速访问栏控制按钮
    explorer->quick_access_ctrl_btn = lv_btn_create(explorer->head_area);
    lv_obj_align(explorer->quick_access_ctrl_btn, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_add_flag(explorer->quick_access_ctrl_btn, LV_OBJ_FLAG_CHECKABLE);

    // 快速访问控制按钮上的label
    lv_obj_t * label = lv_label_create(explorer->quick_access_ctrl_btn);
    lv_label_set_text(label, LV_SYMBOL_LIST);

    // 快速访问区域的两个列表
    lv_obj_t * btn;
    // 列表1
    explorer->list_device = lv_list_create(explorer->quick_access_area);
    lv_obj_set_size(explorer->list_device, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(lv_list_add_text(explorer->list_device, "DEVICE"), lv_palette_main(LV_PALETTE_ORANGE), 0);

    btn = lv_list_add_btn(explorer->list_device, NULL, LV_SYMBOL_DRIVE " File System");
    lv_obj_add_event_cb(btn, quick_access_event_handler, LV_EVENT_CLICKED, obj);
    btn = lv_list_add_btn(explorer->list_device, NULL, LV_SYMBOL_DRIVE " Mnt");
    lv_obj_add_event_cb(btn, quick_access_event_handler, LV_EVENT_CLICKED, obj);

    // 列表2
    explorer->list_places = lv_list_create(explorer->quick_access_area);
    lv_obj_set_size(explorer->list_places, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(lv_list_add_text(explorer->list_places, "PLACES"), lv_palette_main(LV_PALETTE_LIME), 0);

    btn = lv_list_add_btn(explorer->list_places, NULL, LV_SYMBOL_HOME " HOME");
    lv_obj_add_event_cb(btn, quick_access_event_handler, LV_EVENT_CLICKED, obj);
    btn = lv_list_add_btn(explorer->list_places, NULL, LV_SYMBOL_VIDEO " Video");
    lv_obj_add_event_cb(btn, quick_access_event_handler, LV_EVENT_CLICKED, obj);
    btn = lv_list_add_btn(explorer->list_places, NULL, LV_SYMBOL_IMAGE " Pictures");
    lv_obj_add_event_cb(btn, quick_access_event_handler, LV_EVENT_CLICKED, obj);
    btn = lv_list_add_btn(explorer->list_places, NULL, LV_SYMBOL_AUDIO " Music");
    lv_obj_add_event_cb(btn, quick_access_event_handler, LV_EVENT_CLICKED, obj);
    btn = lv_list_add_btn(explorer->list_places, NULL, LV_SYMBOL_FILE "  Documents");
    lv_obj_add_event_cb(btn, quick_access_event_handler, LV_EVENT_CLICKED, obj);
#endif

    // 展示当前路径
    explorer->path_label = lv_label_create(explorer->head_area);
    lv_label_set_long_mode(explorer->path_label, LV_LABEL_LONG_CLIP);
    lv_obj_set_size(explorer->path_label, LV_PCT(100), LV_PCT(100));
    lv_label_set_text(explorer->path_label, "/"/*"https://lvgl.100ask.net"*/);
    lv_obj_set_style_text_align(explorer->path_label, LV_TEXT_ALIGN_RIGHT, LV_STATE_DEFAULT);
    lv_obj_align(explorer->path_label, LV_ALIGN_TOP_MID, 0, LV_PCT(50));
    

    // 目录内容展示列表
    explorer->file_list = lv_table_create(explorer->browser_area);
    lv_obj_set_width(explorer->file_list, LV_PCT(100));
    lv_obj_set_flex_grow(explorer->file_list, 1);
    lv_table_set_col_width(explorer->file_list, 0, LV_PCT(100));
    lv_table_set_col_cnt(explorer->file_list, 1);
    lv_obj_add_event_cb(explorer->file_list, brower_file_event_handler, LV_EVENT_ALL, obj);
    // only scroll up and down
    lv_obj_set_scroll_dir(explorer->file_list, LV_DIR_TOP | LV_DIR_BOTTOM);

    // 初始化样式
    init_style(obj);

    LV_TRACE_OBJ_CREATE("finished");
}

static void lv_100ask_file_explorer_destructor(const lv_obj_class_t * class_p, lv_obj_t * obj)
{
    LV_UNUSED(class_p);
    LV_TRACE_OBJ_CREATE("begin");
}

static void init_style(lv_obj_t * obj)
{
    lv_100ask_file_explorer_t * explorer = (lv_100ask_file_explorer_t *)obj;

    // cont区域风格样式
    static lv_style_t cont_style;
    lv_style_init(&cont_style);
    lv_style_set_radius(&cont_style, 0);
    lv_style_set_bg_opa(&cont_style, LV_OPA_0);
    lv_style_set_border_width(&cont_style, 0);
    lv_style_set_outline_width(&cont_style, 0);
    lv_style_set_pad_column(&cont_style, 0);
    lv_style_set_pad_row(&cont_style, 0);
    lv_style_set_flex_flow(&cont_style, LV_FLEX_FLOW_ROW);
    lv_style_set_pad_all(&cont_style, 0);
    lv_style_set_layout(&cont_style, LV_LAYOUT_FLEX);

#if LV_100ASK_FILE_EXPLORER_QUICK_ACCESS
    // quick_access区域风格样式
    static lv_style_t quick_access_area_style;
    lv_style_init(&quick_access_area_style);
    lv_style_set_pad_all(&quick_access_area_style, 0);
    lv_style_set_pad_row(&quick_access_area_style, 20);
    lv_style_set_radius(&quick_access_area_style, 0);
    lv_style_set_border_width(&quick_access_area_style, 1);
    lv_style_set_outline_width(&quick_access_area_style, 0);
    lv_style_set_bg_color(&quick_access_area_style, lv_color_hex(0xf2f1f6));
#endif

    // 文件查看、浏览区域风格样式
    static lv_style_t brower_area_style;
    lv_style_init(&brower_area_style);
    lv_style_set_pad_all(&brower_area_style, 0);
    lv_style_set_radius(&brower_area_style, 0);
    lv_style_set_border_width(&brower_area_style, 0);
    lv_style_set_outline_width(&brower_area_style, 0);
    //lv_style_set_bg_color(&brower_area_style, lv_color_hex(0xffffff));

#if LV_100ASK_FILE_EXPLORER_QUICK_ACCESS
    // 左侧菜单的风格样式
    static lv_style_t quick_access_list_style;
    lv_style_init(&quick_access_list_style);
    lv_style_set_border_width(&quick_access_list_style, 0);
    lv_style_set_outline_width(&quick_access_list_style, 0);
    //lv_style_set_bg_color(&quick_access_list_style, lv_color_hex(0x222d36));
    lv_style_set_radius(&quick_access_list_style, 0);
    lv_style_set_pad_all(&quick_access_list_style, 0);

    static lv_style_t quick_access_list_btn_style;
    lv_style_init(&quick_access_list_btn_style);
    lv_style_set_border_width(&quick_access_list_btn_style, 0);
    lv_style_set_bg_color(&quick_access_list_btn_style, lv_color_hex(0xf2f1f6));
#endif

    // 右侧文件浏览区域的样式风格
    static lv_style_t file_list_style;
    lv_style_init(&file_list_style);
    //lv_style_set_bg_color(&file_list_style, lv_color_hex(0xffffff));
    lv_style_set_pad_all(&file_list_style, 0);
    lv_style_set_radius(&file_list_style, 0);
    lv_style_set_border_width(&file_list_style, 0);
    lv_style_set_outline_width(&file_list_style, 0);

    // 设置 file_explorer 的样式
    lv_obj_set_style_radius(obj, 0, 0);
    //lv_obj_set_style_bg_color(obj, lv_color_hex(0xf2f1f6), 0);

    // 在文件浏览列表之上的区域的样式
    lv_obj_set_style_radius(explorer->head_area, 0, 0);
    lv_obj_set_style_border_width(explorer->head_area, 0, 0);
    lv_obj_set_style_pad_top(explorer->head_area, 0, 0);

#if LV_100ASK_FILE_EXPLORER_QUICK_ACCESS
    // 设置快速访问栏控制按钮的样式
    lv_obj_set_style_radius(explorer->quick_access_ctrl_btn, 0, 0);
    lv_obj_set_style_pad_all(explorer->quick_access_ctrl_btn, 5, 0);
    lv_obj_add_event_cb(explorer->quick_access_ctrl_btn, quick_access_ctrl_btn_event_handler, LV_EVENT_VALUE_CHANGED, explorer);
#endif

    lv_obj_add_style(explorer->cont, &cont_style, 0);
    lv_obj_add_style(explorer->browser_area, &brower_area_style, 0);
#if LV_100ASK_FILE_EXPLORER_QUICK_ACCESS
    lv_obj_add_style(explorer->quick_access_area, &quick_access_area_style, 0);
    lv_obj_add_style(explorer->list_device, &quick_access_list_style, 0);
    lv_obj_add_style(explorer->list_places, &quick_access_list_style, 0);
#endif
    lv_obj_add_style(explorer->file_list, &file_list_style, 0);

#if LV_100ASK_FILE_EXPLORER_QUICK_ACCESS
    uint32_t i, j;
    for(i = 0; i < lv_obj_get_child_cnt(explorer->quick_access_area); i++) {
        lv_obj_t * child = lv_obj_get_child(explorer->quick_access_area, i);
        if(lv_obj_check_type(child, &lv_list_class)) {
            for(j = 0; j < lv_obj_get_child_cnt(child); j++) {
                lv_obj_t * list_child = lv_obj_get_child(child, j);
                if(lv_obj_check_type(list_child, &lv_list_btn_class)) {
                    lv_obj_add_style(list_child, &quick_access_list_btn_style, 0);
                }
            }
        }
    }
#endif

}

#if LV_100ASK_FILE_EXPLORER_QUICK_ACCESS
static void quick_access_event_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * btn = lv_event_get_target(e);
    lv_obj_t * obj = lv_event_get_user_data(e);

    lv_100ask_file_explorer_t * explorer = (lv_100ask_file_explorer_t *)obj;

    if(code == LV_EVENT_CLICKED) {
        char ** path = NULL;
        lv_obj_t * label = lv_obj_get_child(btn, -1);
        char * label_text = lv_label_get_text(label);

        if((strcmp(label_text, LV_SYMBOL_HOME " HOME") == 0)) {
            path = &(explorer->home_dir);
        }
        else if((strcmp(label_text, LV_SYMBOL_VIDEO " Video") == 0)) {
            path = &(explorer->video_dir);
        }
        else if((strcmp(label_text, LV_SYMBOL_IMAGE " Pictures") == 0)) {
            path = &(explorer->pictures_dir);
        }
        else if((strcmp(label_text, LV_SYMBOL_AUDIO " Music") == 0)) {
            path = &(explorer->music_dir);
        }
        else if((strcmp(label_text, LV_SYMBOL_FILE "  Documents") == 0)) {
            path = &(explorer->docs_dir);
        }
        else if((strcmp(label_text, LV_SYMBOL_DRIVE " File System") == 0)) {
#if LV_USE_FS_WIN32
            path = NULL;
#else
            char * tmp_str = "/";
            path = &tmp_str;
#endif
        }
        else if((strcmp(label_text, LV_SYMBOL_DRIVE " Mnt") == 0)) {
#if LV_USE_FS_WIN32
            path = NULL;
#else
            char * tmp_str = "/mnt";
            path = &tmp_str;
#endif
        }

        if (path != NULL)
            show_dir(obj, *path);
    }
}

static void quick_access_ctrl_btn_event_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * btn = lv_event_get_target(e);
    lv_obj_t * obj = lv_event_get_user_data(e);

    lv_100ask_file_explorer_t * explorer = (lv_100ask_file_explorer_t *)obj;

    if(code == LV_EVENT_VALUE_CHANGED) {
        if (lv_obj_has_state(btn, LV_STATE_CHECKED)) {
            lv_obj_add_flag(explorer->quick_access_area, LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_size(explorer->browser_area, LV_PCT(100), LV_PCT(100));
        }
        else {
            lv_obj_clear_flag(explorer->quick_access_area, LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_size(explorer->browser_area, LV_PCT(FILE_EXPLORER_BROWER_AREA_WIDTH), LV_PCT(100));
        }
    }
}
#endif

static void brower_file_event_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_user_data(e);
    //lv_obj_t * btn = lv_event_get_target(e);

    lv_100ask_file_explorer_t * explorer = (lv_100ask_file_explorer_t *)obj;

    if(code == LV_EVENT_VALUE_CHANGED) {
        if(explorer->long_pressed) {
            explorer->long_pressed = false;
            return;
        }

        char * file_name = lv_mem_alloc(LV_100ASK_FILE_EXPLORER_PATH_MAX_LEN);
        lv_memset_00(file_name, LV_100ASK_FILE_EXPLORER_PATH_MAX_LEN);

        uint16_t row;  // 行
        uint16_t col;  // 列
        lv_table_get_selected_cell(explorer->file_list, &row, &col);

        char * str_fn = lv_table_get_cell_value(explorer->file_list, row, col);
        str_fn += 5;    // 图标占3位，加上2个空格，因此总计向后移5位

        if((strlen(str_fn) == 0) || (strcmp(str_fn, ".") == 0))  return;
        
        if((strcmp(str_fn, "..") == 0) && (strlen(explorer->cur_path) > 3))
        {
            strip_ext(explorer->cur_path);
            strip_ext(explorer->cur_path); // 去掉最后的 '/' 路径
            lv_snprintf(file_name, LV_100ASK_FILE_EXPLORER_PATH_MAX_LEN, "%s", explorer->cur_path);
        }
        else
        {
            if(strcmp(str_fn, "..") != 0){
                lv_snprintf(file_name, LV_100ASK_FILE_EXPLORER_PATH_MAX_LEN, "%s%s", explorer->cur_path, str_fn);
            }
        }

        lv_fs_dir_t dir;
        if(lv_fs_dir_open(&dir, file_name) == LV_FS_RES_OK) {
            lv_fs_dir_close(&dir);
            show_dir(obj, file_name);
            lv_event_send(obj, LV_EVENT_VALUE_CHANGED, e->user_data);
        }
        else {
            if(strcmp(str_fn, "..") != 0) {
                explorer->sel_fp = str_fn;
                lv_event_send(obj, LV_EVENT_CLICKED, e->user_data);
            }
        }
        lv_mem_free(file_name);
    }
    else if(code == LV_EVENT_SIZE_CHANGED) {
        lv_table_set_col_width(explorer->file_list, 0, lv_obj_get_width(explorer->file_list));
    }
    else if(code == LV_EVENT_LONG_PRESSED) {
        explorer->long_pressed = true;
        char * str_fn = NULL;
        uint16_t row;
        uint16_t col;

        lv_table_get_selected_cell(explorer->file_list, &row, &col);
        str_fn = lv_table_get_cell_value(explorer->file_list, row, col);

        str_fn = str_fn+5;
        if((strcmp(str_fn, ".") == 0) || (strcmp(str_fn, "..") == 0))  return;

        if(strcmp(str_fn, "..") != 0) {
            explorer->sel_fp = str_fn;
            lv_event_send(obj, LV_EVENT_LONG_PRESSED, e->user_data);
        }
    }
}


static void show_dir(lv_obj_t * obj, char * path)
{
    lv_100ask_file_explorer_t * explorer = (lv_100ask_file_explorer_t *)obj;

    char fn[LV_100ASK_FILE_EXPLORER_PATH_MAX_LEN];
    uint16_t index = 0;
    lv_fs_dir_t dir;
    lv_fs_res_t res;

    res = lv_fs_dir_open(&dir, path);
    if(res != LV_FS_RES_OK) {
        LV_LOG_USER("Open dir error %d!", res);
        return;
    }

    // 第0列是 图标+2个空格+文件名，第1列是文件类型
    //lv_table_set_cell_value_fmt(explorer->file_list, index++, 0, LV_SYMBOL_DIRECTORY "  %s", ".");
    lv_table_set_cell_value_fmt(explorer->file_list, index++, 0, LV_SYMBOL_DIRECTORY "  %s", "..");
    lv_table_set_cell_value(explorer->file_list, 0, 1, "0");
    //lv_table_set_cell_value(explorer->file_list, 1, 1, "0");

    while(1) {
        res = lv_fs_dir_read(&dir, fn);
        if(res != LV_FS_RES_OK) {
            LV_LOG_USER("Driver, file or directory does not exist: %d", res);
            break;
        }

        /*fn is empty, if not more files to read*/
        if(strlen(fn) == 0) {
            //LV_LOG_USER("No more files to read!");
            break;
        }

        // 识别并展示文件
        if(file_ext_match(fn, IMAGE_FILE_EXT)) {
            lv_table_set_cell_value_fmt(explorer->file_list, index, 0, LV_SYMBOL_IMAGE "  %s", fn);
            lv_table_set_cell_value(explorer->file_list, index, 1, "2");
        } else if(file_ext_match(fn, AUDIO_FILE_EXT)) {
            lv_table_set_cell_value_fmt(explorer->file_list, index, 0, LV_SYMBOL_AUDIO "  %s", fn);
            lv_table_set_cell_value(explorer->file_list, index, 1, "2");
        } else if(file_ext_match(fn, VIDEO_FILE_EXT)) {
            lv_table_set_cell_value_fmt(explorer->file_list, index, 0, LV_SYMBOL_VIDEO "  %s", fn);
            lv_table_set_cell_value(explorer->file_list, index, 1, "2");
        } else if(file_ext_match(fn, MIDI_FILE_EXT)) {
            lv_table_set_cell_value_fmt(explorer->file_list, index, 0, LV_SYMBOL_AUDIO "  %s", fn);
            lv_table_set_cell_value(explorer->file_list, index, 1, "2");
        } else if(file_ext_match(fn, TEXT_FILE_EXT)) {
            lv_table_set_cell_value_fmt(explorer->file_list, index, 0, LV_SYMBOL_FILE "  %s", fn);
            lv_table_set_cell_value(explorer->file_list, index, 1, "2");
        } else if(is_end_with(fn, ".", false) || is_end_with(fn, "..", false)) {
            /*is dir*/
            //lv_table_set_cell_value_fmt(explorer->file_list, index, 0, LV_SYMBOL_DIRECTORY "  %s", fn);
            continue;
        } else if(fn[0] == '/') { /*is dir*/
            lv_table_set_cell_value_fmt(explorer->file_list, index, 0, LV_SYMBOL_DIRECTORY "  %s", fn+1);
            lv_table_set_cell_value(explorer->file_list, index, 1, "1");
        } else {
            lv_table_set_cell_value_fmt(explorer->file_list, index, 0, LV_SYMBOL_FILE "  %s", fn);
            lv_table_set_cell_value(explorer->file_list, index, 1, "2");
        }

        index++;

        //LV_LOG_USER("%s", fn);
    }

    lv_fs_dir_close(&dir);
    lv_table_set_row_cnt(explorer->file_list, index);

    sort_table_by_name(explorer->file_list, 0, lv_table_get_row_cnt(explorer->file_list) - 1);
    
    lv_obj_scroll_to_y(explorer->file_list, 0, LV_ANIM_OFF);    // 滚动到最顶部

    lv_memset_00(explorer->cur_path, sizeof(explorer->cur_path));
    strcpy(explorer->cur_path, path);
    lv_label_set_text_fmt(explorer->path_label, "%s", path);

    size_t cur_path_len = strlen(explorer->cur_path);
    if((*((explorer->cur_path) + cur_path_len) != '/') && (cur_path_len < LV_100ASK_FILE_EXPLORER_PATH_MAX_LEN)) {
        *((explorer->cur_path) + cur_path_len) = '/';
    }  
}


// 去掉最后的后缀名
static void strip_ext(char *dir)
{
    char *end = dir + strlen(dir);

    while (end >= dir && *end != '/') {
        --end;
    }

    if (end > dir) {
        *end = '\0';
    }
    else if (end == dir) {
        *(end+1) = '\0';
    }

}



static void exch_table_item(lv_obj_t * tb, int16_t i,int16_t j )
{
    const char * i0 = lv_table_get_cell_value(tb, i, 0);
    const char * tmpi0 = lv_mem_alloc(strlen(i0) + 1);
    strcpy(tmpi0, i0);

    const char * i1 = lv_table_get_cell_value(tb, i, 1);
    const char * tmpi1 = lv_mem_alloc(strlen(i1) + 1);
    strcpy(tmpi1, i1);

    lv_table_set_cell_value(tb, i, 0, lv_table_get_cell_value(tb, j, 0));
    lv_table_set_cell_value(tb, i, 1, lv_table_get_cell_value(tb, j, 1));

    lv_table_set_cell_value(tb, j, 0, tmpi0);
    lv_table_set_cell_value(tb, j, 1, tmpi1);

    lv_mem_free(tmpi0);
    lv_mem_free(tmpi1);

    /*
    const char * tmp;
    tmp = lv_table_get_cell_value(tb, i, 0);
    lv_table_set_cell_value(tb, 0, 2, tmp);
    lv_table_set_cell_value(tb, i, 0, lv_table_get_cell_value(tb, j, 0));
    lv_table_set_cell_value(tb, j, 0, lv_table_get_cell_value(tb, 0, 2));

    tmp = lv_table_get_cell_value(tb, i, 1);
    lv_table_set_cell_value(tb, 0, 2, tmp);
    lv_table_set_cell_value(tb, i, 1, lv_table_get_cell_value(tb, j, 1));
    lv_table_set_cell_value(tb, j, 1, lv_table_get_cell_value(tb, 0, 2));
    */
    
}

/**
 * 先按照类型，再按照文件名排序
 * @param tb table
 * @param lo from index
 * @param hi to index
 */
static void sort_table_by_name(lv_obj_t * tb, int16_t lo, int16_t hi)
{
    if( lo >= hi ) return;  //单个元素或者没有元素的情况

    int16_t lt = lo;
    int16_t i = lo + 1;  //第一个元素是切分元素，所以指针i可以从lo+1开始
    int16_t gt = hi;

    const char * cut_type_ptr = lv_table_get_cell_value(tb, lo, 1);
    const char * cut_type = lv_mem_alloc(strlen(cut_type_ptr) + 1);
    strcpy(cut_type, cut_type_ptr);

    const char * cut_name_ptr = lv_table_get_cell_value(tb, lo, 0) + 5;    //右移5位，避开图标和两个空格
    const char * cut_name = lv_mem_alloc(strlen(cut_name_ptr) + 1);
    strcpy(cut_name, cut_name_ptr);

    while( i <= gt )
    {
        int type_cmp = strcmp(lv_table_get_cell_value(tb, i, 1), cut_type);
        int name_cmp = strcmp(lv_table_get_cell_value(tb, i, 0) + 5, cut_name);    //strcmp的正负是按照ascii来计算的
        bool result = (type_cmp != 0) ? (type_cmp < 0) : (name_cmp < 0);
        //printf("[file-manager] %s %s vs %s %s, %d %d, %d\n", lv_table_get_cell_value(tb, i, 1), lv_table_get_cell_value(tb, i, 0) + 5, cut_type, cut_name, type_cmp, name_cmp, result);

        if(result)  //小于切分元素的放在lt左边，因此指针lt和指针i整体右移
            exch_table_item(tb, lt++, i++);
        else  //大于切分元素的放在gt右边，因此指针gt需要左移
            exch_table_item(tb, i, gt--);
    }

    lv_mem_free(cut_type);
    lv_mem_free(cut_name);

    //lt-gt的元素已经排定，只需对it左边和gt右边的元素进行递归求解
    sort_table_by_name(tb, lo, lt-1);
    sort_table_by_name(tb, gt+1, hi);
}

static bool file_ext_match(const char * file_name, const char * file_ext[])
{
    for(int i = 0; file_ext[i] != NULL; i++) {
        if(is_end_with(file_name, file_ext[i], false)) return true;
    }
    return false;
}

static bool is_begin_with(const char * str1, const char * str2, bool case_sensitivity)
{
    if(str1 == NULL || str2 == NULL)
        return false;

    uint16_t len1 = strlen(str1);
    uint16_t len2 = strlen(str2);
    if((len1 < len2) || (len1 == 0 || len2 == 0))
        return false;

    uint16_t i = 0;
    char * p = str2;
    while(*p != '\0')
    {
        if(case_sensitivity) {
            if(*p != str1[i]) return false;
        }
        else {
            if(to_upper_case(*p) != to_upper_case(str1[i])) return false;
        }

        p++;
        i++;
    }

    return true;
}



static bool is_end_with(const char * str1, const char * str2, bool case_sensitivity)
{
    if(str1 == NULL || str2 == NULL)
        return false;
    
    uint16_t len1 = strlen(str1);
    uint16_t len2 = strlen(str2);
    if((len1 < len2) || (len1 == 0 || len2 == 0))
        return false;
    
    while(len2 >= 1)
    {
        if(case_sensitivity){
            if(str2[len2 - 1] != str1[len1 - 1]) return false;
        }
        else {
            if(to_upper_case(str2[len2 - 1]) != to_upper_case(str1[len1 - 1])) return false;
        }


        len2--;
        len1--;
    }

    return true;
}

static char to_upper_case(char c)
{
    if(is_upper_letter(c)) return c;
    if(is_lower_letter(c)) return c + 32;
    return c;
}

static bool is_lower_letter(char c)
{
    return 65 <= c && c <= 90;
}

static bool is_upper_letter(char c)
{
    return 97 <= c && c <= 122;
}

#endif  /*LV_USE_100ASK_FILE_EXPLORER*/
