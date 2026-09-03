/**
 * @file lv_ime_pinyin.c
 * 从8.3主线拿来的拼音输入法
 * 尝试升级lvgl版本结果出现一堆毛病，于是不升了，稳定最重要
 * 这个输入法也是毛病多多
 * 100ask是怎么在有明显数组越界问题的情况下把这玩意提交到主线里的？？？
 */

/*********************
 *      INCLUDES
 *********************/
#include "lv_ime_pinyin.h"
#if LV_USE_IME_PINYIN != 0

#include <stdio.h>
#include <stdint.h>

/*********************
 *      DEFINES
 *********************/
#define MY_CLASS    &lv_ime_pinyin_class

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void lv_ime_pinyin_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj);
static void lv_ime_pinyin_destructor(const lv_obj_class_t * class_p, lv_obj_t * obj);
static void lv_ime_pinyin_style_change_event(lv_event_t * e);
static void lv_ime_pinyin_kb_event(lv_event_t * e);
static void lv_ime_pinyin_cand_panel_event(lv_event_t * e);
static void lv_ime_pinyin_clear_data(lv_obj_t * obj);

static void lv_ime_pinyin_input_proc(lv_obj_t * obj);
static void lv_ime_pinyin_set_cand_page(lv_obj_t * obj, uint16_t cand_page);
static void lv_ime_pinyin_set_k9_cand_page(lv_obj_t * obj, uint16_t k9_cand_page);
static void lv_ime_pinyin_select_k9_cand(lv_obj_t * obj, uint16_t k9_cand_index);

/**********************
 *  STATIC VARIABLES
 **********************/
const lv_obj_class_t lv_ime_pinyin_class = {
    .constructor_cb = lv_ime_pinyin_constructor,
    .destructor_cb  = lv_ime_pinyin_destructor,
    .width_def      = LV_SIZE_CONTENT,
    .height_def     = LV_SIZE_CONTENT,
    .group_def      = LV_OBJ_CLASS_GROUP_DEF_TRUE,
    .instance_size  = sizeof(lv_ime_pinyin_t),
    .base_class     = &lv_obj_class
};

#define LV_KB_BTN(width) LV_BTNMATRIX_CTRL_POPOVER | width

// K26 的键盘布局
static const char * const kb_map_k26[] = {"1#", "q", "w", "e", "r", "t", "y", "u", "i", "o", "p", LV_SYMBOL_BACKSPACE, "\n",
                                                 "'", "a", "s", "d", "f", "g", "h", "j", "k", "l", LV_SYMBOL_NEW_LINE, "\n",
                                                 "_", "-", "z", "x", "c", "v", "b", "n", "m", "。", "，", "、", "\n",
                                                 LV_SYMBOL_KEYBOARD, LV_SYMBOL_LEFT, " ", LV_SYMBOL_RIGHT, LV_SYMBOL_OK, ""
                                                };

static const lv_btnmatrix_ctrl_t kb_ctrl_k26[] = {
    LV_KEYBOARD_CTRL_BTN_FLAGS | 5, LV_KB_BTN(4), LV_KB_BTN(4), LV_KB_BTN(4), LV_KB_BTN(4), LV_KB_BTN(4), LV_KB_BTN(4), LV_KB_BTN(4), LV_KB_BTN(4), LV_KB_BTN(4), LV_KB_BTN(4), LV_BTNMATRIX_CTRL_CHECKED | 7,
    LV_KEYBOARD_CTRL_BTN_FLAGS | 6, LV_KB_BTN(3), LV_KB_BTN(3), LV_KB_BTN(3), LV_KB_BTN(3), LV_KB_BTN(3), LV_KB_BTN(3), LV_KB_BTN(3), LV_KB_BTN(3), LV_KB_BTN(3), LV_BTNMATRIX_CTRL_CHECKED | 7,
    LV_BTNMATRIX_CTRL_CHECKED | LV_KB_BTN(1), LV_BTNMATRIX_CTRL_CHECKED | LV_KB_BTN(1), LV_KB_BTN(1), LV_KB_BTN(1), LV_KB_BTN(1), LV_KB_BTN(1), LV_KB_BTN(1), LV_KB_BTN(1), LV_KB_BTN(1), LV_BTNMATRIX_CTRL_CHECKED | LV_KB_BTN(1), LV_BTNMATRIX_CTRL_CHECKED | LV_KB_BTN(1), LV_BTNMATRIX_CTRL_CHECKED | LV_KB_BTN(1),
    LV_KEYBOARD_CTRL_BTN_FLAGS | 2, LV_BTNMATRIX_CTRL_CHECKED | 2, 6, LV_BTNMATRIX_CTRL_CHECKED | 2, LV_KEYBOARD_CTRL_BTN_FLAGS | 2
};

// K9 的键盘布局
// 气笑了，lv_keyboard里会把 abc 键当成大小写切换，最终我通过移除默认事件解决了
static const char * const k9_py_map[10] = {"", "", "abc", "def", "ghi", "jkl", "mno", "pqrs", "tuv", "wxyz"};
static char * kb_map_k9[] = 
{
    "，", "1#",   "abc", "def",  LV_SYMBOL_BACKSPACE"", "\n",
    "。", "ghi",  "jkl", "mno",  LV_SYMBOL_NEW_LINE"", "\n",
    "？", "pqrs", "tuv", "wxyz", LV_SYMBOL_KEYBOARD"", "\n",
    LV_SYMBOL_LEFT"", " ", " ", " ", LV_SYMBOL_RIGHT"", LV_SYMBOL_OK"", ""
};
#define KB_K9_EXACT_OFFSET 19
#define KB_K9_EXACT_BTNID (KB_K9_EXACT_OFFSET - 3)

static lv_btnmatrix_ctrl_t kb_ctrl_k9[24] = 
{
    1, LV_KEYBOARD_CTRL_BTN_FLAGS | 2, LV_KEYBOARD_CTRL_BTN_FLAGS | 2, LV_KEYBOARD_CTRL_BTN_FLAGS | 2, 1,
    1, LV_KEYBOARD_CTRL_BTN_FLAGS | 2, LV_KEYBOARD_CTRL_BTN_FLAGS | 2, LV_KEYBOARD_CTRL_BTN_FLAGS | 2, LV_BTNMATRIX_CTRL_CHECKED | 1,
    1, LV_KEYBOARD_CTRL_BTN_FLAGS | 2, LV_KEYBOARD_CTRL_BTN_FLAGS | 2, LV_KEYBOARD_CTRL_BTN_FLAGS | 2, LV_KEYBOARD_CTRL_BTN_FLAGS | 1,
    LV_KEYBOARD_CTRL_BTN_FLAGS | 1, LV_KEYBOARD_CTRL_BTN_FLAGS | 2, LV_KEYBOARD_CTRL_BTN_FLAGS | 2, LV_KEYBOARD_CTRL_BTN_FLAGS | 2, LV_KEYBOARD_CTRL_BTN_FLAGS | 1, LV_KEYBOARD_CTRL_BTN_FLAGS | 1
};

// 选词框的文本列表
static char * lv_btnm_def_pinyin_sel_map[LV_IME_PINYIN_CAND_TEXT_NUM + 3];

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
lv_obj_t * lv_ime_pinyin_create(lv_obj_t * parent)
{
    LV_LOG_INFO("begin");
    lv_obj_t * obj = lv_obj_class_create_obj(MY_CLASS, parent);
    lv_obj_class_init_obj(obj);
    return obj;
}

/*=====================
 * Setter functions
 *====================*/
void lv_ime_pinyin_init(lv_obj_t * obj, pinyin_ime_t * pinyin_ime)
{
    lv_ime_pinyin_t * lv_ime = (lv_ime_pinyin_t *)obj;
    lv_ime->pinyin_ime = pinyin_ime;
}


/**
 * Set mode, 26-key input(k26) or 9-key input(k9).
 * @param obj  pointer to a Pinyin input method object
 * @param mode   the mode from 'lv_keyboard_mode_t'
 */
void lv_ime_pinyin_set_mode(lv_obj_t * obj, lv_ime_pinyin_mode_t mode)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    lv_ime_pinyin_t * lv_ime = (lv_ime_pinyin_t *)obj;

    LV_ASSERT_OBJ(lv_ime->kb, &lv_keyboard_class);

    lv_ime->mode = mode;

    switch (mode)
    {
    case LV_IME_PINYIN_MODE_K26:
        lv_keyboard_set_map(lv_ime->kb, LV_KEYBOARD_MODE_USER_1, (const char **)kb_map_k26,
                            (const lv_btnmatrix_ctrl_t *)kb_ctrl_k26);
        lv_keyboard_set_mode(lv_ime->kb, LV_KEYBOARD_MODE_USER_1);
        break;

    case LV_IME_PINYIN_MODE_K9:
        lv_keyboard_set_map(lv_ime->kb, LV_KEYBOARD_MODE_USER_2, (const char **)kb_map_k9,
                            (const lv_btnmatrix_ctrl_t *)kb_ctrl_k9);
        lv_keyboard_set_mode(lv_ime->kb, LV_KEYBOARD_MODE_USER_2);
        break;

    case LV_IME_PINYIN_MODE_EN:
        lv_keyboard_set_mode(lv_ime->kb, LV_KEYBOARD_MODE_TEXT_LOWER);
        break;
    }
    
}

/*=====================
 * Getter functions
 *====================*/

/**
 * Set the dictionary of Pinyin input method.
 * @param obj  pointer to a Pinyin IME object
 * @return     pointer to the Pinyin IME keyboard
 */
lv_obj_t * lv_ime_pinyin_get_kb(lv_obj_t * obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_ime_pinyin_t * lv_ime = (lv_ime_pinyin_t *)obj;

    return lv_ime->kb;
}

/**
 * Set the dictionary of Pinyin input method.
 * @param obj  pointer to a Pinyin input method object
 * @return     pointer to the Pinyin input method candidate panel
 */
lv_obj_t * lv_ime_pinyin_get_cand_panel(lv_obj_t * obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_ime_pinyin_t * lv_ime = (lv_ime_pinyin_t *)obj;

    return lv_ime->cand_panel;
}

/*=====================
 * Other functions
 *====================*/

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void lv_ime_pinyin_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj)
{
    LV_UNUSED(class_p);
    LV_TRACE_OBJ_CREATE("begin");

    lv_ime_pinyin_t * lv_ime = (lv_ime_pinyin_t *)obj;

    uint16_t btnm_i = 0;
    for(btnm_i = 0; btnm_i < (LV_IME_PINYIN_CAND_TEXT_NUM + 3); btnm_i++) {
        if(btnm_i == 0) {
            lv_btnm_def_pinyin_sel_map[btnm_i] = "<";
        }
        else if(btnm_i == (LV_IME_PINYIN_CAND_TEXT_NUM + 1)) {
            lv_btnm_def_pinyin_sel_map[btnm_i] = ">";
        }
        else if(btnm_i == (LV_IME_PINYIN_CAND_TEXT_NUM + 2)) {
            lv_btnm_def_pinyin_sel_map[btnm_i] = "";
        }
        else {
            lv_btnm_def_pinyin_sel_map[btnm_i] = " ";
        }
    }

    lv_ime->pinyin_ime = NULL;
    lv_memset_00(lv_ime->pinyin_input, sizeof(lv_ime->pinyin_input));

    lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);

    lv_obj_set_size(obj, LV_PCT(100), LV_PCT(55));
    lv_obj_align(obj, LV_ALIGN_BOTTOM_MID, 0, 0);

    /* Init lv_ime->cand_panel */
    
    lv_ime->cand_panel = lv_btnmatrix_create(obj);
    lv_ime->kb = lv_keyboard_create(obj);
    
    lv_obj_set_size(lv_ime->kb, LV_PCT(100), LV_PCT(85));
    lv_obj_set_size(lv_ime->cand_panel, LV_PCT(100), LV_PCT(15));

    lv_obj_align_to(lv_ime->cand_panel, lv_ime->kb, LV_ALIGN_OUT_TOP_MID, 0, 0);
    lv_btnmatrix_set_map(lv_ime->cand_panel, (const char **)lv_btnm_def_pinyin_sel_map);
    //lv_obj_add_flag(lv_ime->cand_panel, LV_OBJ_FLAG_HIDDEN);

    lv_btnmatrix_set_one_checked(lv_ime->cand_panel, true);
    lv_obj_clear_flag(lv_ime->cand_panel, LV_OBJ_FLAG_CLICK_FOCUSABLE);

    /* Set cand_panel style*/
    // Default style
    lv_obj_set_style_bg_opa(lv_ime->cand_panel, LV_OPA_0, 0);
    lv_obj_set_style_border_width(lv_ime->cand_panel, 0, 0);
    lv_obj_set_style_pad_all(lv_ime->cand_panel, 8, 0);
    lv_obj_set_style_pad_gap(lv_ime->cand_panel, 0, 0);
    lv_obj_set_style_radius(lv_ime->cand_panel, 0, 0);
    lv_obj_set_style_pad_gap(lv_ime->cand_panel, 0, 0);
    lv_obj_set_style_base_dir(lv_ime->cand_panel, LV_BASE_DIR_LTR, 0);

    // LV_PART_ITEMS style
    lv_obj_set_style_radius(lv_ime->cand_panel, 12, LV_PART_ITEMS);
    lv_obj_set_style_bg_color(lv_ime->cand_panel, lv_color_white(), LV_PART_ITEMS);
    lv_obj_set_style_bg_opa(lv_ime->cand_panel, LV_OPA_0, LV_PART_ITEMS);
    lv_obj_set_style_shadow_opa(lv_ime->cand_panel, LV_OPA_0, LV_PART_ITEMS);

    // LV_PART_ITEMS | LV_STATE_PRESSED style
    lv_obj_set_style_bg_opa(lv_ime->cand_panel, LV_OPA_COVER, LV_PART_ITEMS | LV_STATE_PRESSED);
    lv_obj_set_style_bg_color(lv_ime->cand_panel, lv_color_white(), LV_PART_ITEMS | LV_STATE_PRESSED);

    lv_ime_pinyin_set_mode(obj, LV_IME_PINYIN_MODE_K26);

    /* event handler */
    // 删除键盘的默认回调，让我们自己来处理事件
    lv_obj_remove_event_cb(lv_ime->kb, lv_keyboard_def_event_cb);
    lv_obj_add_event_cb(lv_ime->cand_panel, lv_ime_pinyin_cand_panel_event, LV_EVENT_VALUE_CHANGED, obj);
    lv_obj_add_event_cb(obj, lv_ime_pinyin_style_change_event, LV_EVENT_STYLE_CHANGED, NULL);
    lv_obj_add_event_cb(lv_ime->kb, lv_ime_pinyin_kb_event, LV_EVENT_VALUE_CHANGED, obj);
}

static void lv_ime_pinyin_destructor(const lv_obj_class_t * class_p, lv_obj_t * obj)
{
    LV_UNUSED(class_p);

}

/**
 * 键盘按键事件回调（输入法核心入口）。
 * 用户在软键盘上每按下一个按键都会触发一次 LV_EVENT_VALUE_CHANGED，
 * 本函数根据按键文本 txt 分发到不同的处理分支：
 *   - 回车 / 换行：清空输入数据
 *   - 退格：删除输入缓冲区最后一个字符并重新检索候选字
 *   - 字母键（K26 全键盘）：追加到拼音串并触发候选字检索
 *   - 字母键（K9 九宫格）：按键位映射到 2~9 数字编码，再枚举合法拼音
 *   - 功能键（切换模式 / 确认 / 左右翻页等）
 */
static void lv_ime_pinyin_kb_event(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * kb = lv_event_get_target(e);         /* 触发事件的键盘对象 */
    lv_obj_t * obj = lv_event_get_user_data(e);     /* 绑定时传入的输入法对象 */
    lv_obj_t * ta = lv_keyboard_get_textarea(kb);

    lv_ime_pinyin_t * lv_ime = (lv_ime_pinyin_t *)obj;
    pinyin_ime_t * pinyin_ime = lv_ime->pinyin_ime;
    lv_obj_t * cand_panel = lv_ime->cand_panel;

    uint16_t pinyin_count = strlen(lv_ime->pinyin_input);

    if(code == LV_EVENT_VALUE_CHANGED) {
        uint16_t btn_id  = lv_btnmatrix_get_selected_btn(kb);   /* 被按下按键的索引 */
        if(btn_id == LV_BTNMATRIX_BTN_NONE) return;             /* 无按键被按下则直接返回 */

        const char * txt = lv_btnmatrix_get_btn_text(kb, lv_btnmatrix_get_selected_btn(kb));
        if(txt == NULL) return;                                  /* 按键文本为空则直接返回 */

        /* 回车 */
        if(strcmp(txt, "Enter") == 0 || strcmp(txt, LV_SYMBOL_NEW_LINE) == 0) {
            lv_ime_pinyin_clear_data(obj);
            lv_keyboard_def_event_cb(e);
        }
        /* 空格 */
        else if(strcmp(txt, " ") == 0) {
            if(pinyin_ime_get_candidate_count(pinyin_ime) != 0) {
                lv_btnmatrix_set_selected_btn(cand_panel, 1);
                lv_event_send(cand_panel, LV_EVENT_VALUE_CHANGED, obj);
            }
            else {
                lv_ime_pinyin_clear_data(obj);
                lv_keyboard_def_event_cb(e);
            }
        }
        /* 确认键 */
        else if(strcmp(txt, LV_SYMBOL_OK) == 0) {
            pinyin_ime_save(pinyin_ime, NULL);
            lv_keyboard_def_event_cb(e);
        }
        /* 数字键 */
        else if(strcmp(txt, "1#") == 0) {
            lv_ime_pinyin_clear_data(obj);
            lv_keyboard_def_event_cb(e);
        }
        /* 键盘切换键 */
        else if(strcmp(txt, LV_SYMBOL_KEYBOARD) == 0) {
            lv_ime_pinyin_clear_data(obj);

            lv_ime_pinyin_mode_t mode_new;
            if(lv_keyboard_get_mode(kb) != LV_KEYBOARD_MODE_SPECIAL) {
                switch(lv_ime->mode) {
                    case LV_IME_PINYIN_MODE_K26: mode_new = LV_IME_PINYIN_MODE_K9; break;
                    case LV_IME_PINYIN_MODE_K9: mode_new = LV_IME_PINYIN_MODE_EN; break;
                    case LV_IME_PINYIN_MODE_EN: mode_new = LV_IME_PINYIN_MODE_K26; break;
                    default: mode_new = LV_IME_PINYIN_MODE_K26;
                }
            }
            else mode_new = lv_ime->mode;
            lv_ime_pinyin_set_mode(obj, mode_new);
        }
        /* K26 模式 */
        else if(lv_ime->mode == LV_IME_PINYIN_MODE_K26) {
            // 字母
            if (txt[0] >= 'a' && txt[0] <= 'z') {
                if(pinyin_count < sizeof(lv_ime->pinyin_input) - 1) {
                    lv_textarea_add_char(ta, txt[0]);
                    lv_ime->pinyin_input[pinyin_count] = txt[0];
                    lv_ime_pinyin_input_proc(obj);
                }
            }
            // 分词符  不能处理，也不需要处理
            else if (txt[0] == '\'') {
                if(pinyin_count < sizeof(lv_ime->pinyin_input) - 1) {
                    lv_textarea_add_char(ta, '\'');
                    lv_ime->pinyin_input[pinyin_count] = txt[0];
                }
            }
            // 退格
            else if(strcmp(txt, LV_SYMBOL_BACKSPACE) == 0) {
                lv_textarea_del_char(ta);
                if(pinyin_count > 0) {
                    lv_ime->pinyin_input[pinyin_count - 1] = '\0';
                    lv_ime_pinyin_input_proc(obj);
                }
            }
            else lv_keyboard_def_event_cb(e);
        }
        /* K9 模式 */
        else if(lv_ime->mode == LV_IME_PINYIN_MODE_K9) {
            // 精确拼音
            if(btn_id >= KB_K9_EXACT_BTNID && btn_id <= KB_K9_EXACT_BTNID + 2) {
                if(pinyin_ime_get_k9_exact_count(pinyin_ime) != 0) {
                    int16_t k9_cand_index = (lv_ime->k9_cand_page * 3) + (btn_id - KB_K9_EXACT_BTNID);
                    lv_ime_pinyin_select_k9_cand(obj, k9_cand_index);
                }
            }
            // 向左翻页
            else if(btn_id == KB_K9_EXACT_BTNID - 1) {
                if(lv_ime->k9_cand_page <= 0) return;
                lv_ime_pinyin_set_k9_cand_page(obj, lv_ime->k9_cand_page - 1);
            }
            // 向右翻页
            else if(btn_id == KB_K9_EXACT_BTNID + 3) {
                int k9_cand_num = pinyin_ime_get_k9_exact_count(pinyin_ime);
                uint16_t page_count = k9_cand_num / 3;
                uint16_t sur = k9_cand_num % 3;
                if(sur) page_count++;
                if(lv_ime->k9_cand_page >= page_count - 1) return;
                lv_ime_pinyin_set_k9_cand_page(obj, lv_ime->k9_cand_page + 1);
            }
            // 字母  input_proc里会重置输入内容的，此处无需add_char
            else if(txt[0] >= 'a' && txt[0] <= 'z') {
                uint8_t match = 2;
                for(; match <= 9; match++) {
                    if(strcmp(txt, k9_py_map[match]) == 0) break;
                }
                if(match == 10) return;

                char c = '0' + match;
                lv_textarea_add_char(ta, c);
                lv_ime->pinyin_input[pinyin_count] = c;
                lv_ime_pinyin_input_proc(obj);
            }
            // 退格
            else if(strcmp(txt, LV_SYMBOL_BACKSPACE) == 0) {
                lv_textarea_del_char(ta);
                if(pinyin_count > 0) {
                    lv_ime->pinyin_input[pinyin_count - 1] = '\0';
                    lv_ime_pinyin_input_proc(obj);
                }
            }
            else lv_keyboard_def_event_cb(e);
        }
        /* 英文模式 */
        else if(lv_ime->mode == LV_IME_PINYIN_MODE_EN) {
            lv_keyboard_def_event_cb(e);
        }
    }
}

/**
 * 候选面板按键事件回调。
 * 候选面板是一个按钮矩阵，首尾两个按钮分别是 "<"(上一页) 和 ">"(下一页)，
 * 中间的按钮是候选汉字。用户点击候选汉字后，用该字替换 textarea 中的拼音串。
 */
static void lv_ime_pinyin_cand_panel_event(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * cand_panel = lv_event_get_target(e);   /* 候选面板对象 */
    lv_obj_t * obj = (lv_obj_t *)lv_event_get_user_data(e);

    lv_ime_pinyin_t * lv_ime = (lv_ime_pinyin_t *)obj;

    pinyin_ime_t * pinyin_ime = lv_ime->pinyin_ime;
    if (!pinyin_ime) return;

    if(code == LV_EVENT_VALUE_CHANGED) {
        uint32_t id = lv_btnmatrix_get_selected_btn(cand_panel);

        /* 点击 "<": 翻到上一页候选 */
        if(id == 0) {
            if(lv_ime->cand_page <= 0) return;
            lv_ime_pinyin_set_cand_page(obj, lv_ime->cand_page - 1);
            return;
        }
        /* 点击 ">": 翻到下一页候选 */
        if(id == (LV_IME_PINYIN_CAND_TEXT_NUM + 1)) {
            int cand_num = pinyin_ime_get_candidate_count(pinyin_ime);
            uint16_t page_count = cand_num / LV_IME_PINYIN_CAND_TEXT_NUM;
            uint16_t sur = cand_num % LV_IME_PINYIN_CAND_TEXT_NUM;
            if(sur) page_count++;
            if(lv_ime->cand_page >= page_count - 1) return;
            lv_ime_pinyin_set_cand_page(obj, lv_ime->cand_page + 1);
            return;
        }

        /* 点击了某个候选汉字: 把拼音串从 textarea 中删除，替换为选中的汉字 */
        
        const char * txt = lv_btnmatrix_get_btn_text(cand_panel, id);
        lv_obj_t * ta = lv_keyboard_get_textarea(lv_ime->kb);

        for (uint32_t i = 0; i < strlen(lv_ime->pinyin_input); i++)
        {
            lv_textarea_del_char(ta);
        }
        lv_textarea_add_text(ta, txt);

        int16_t cand_index = (lv_ime->cand_page * LV_IME_PINYIN_CAND_TEXT_NUM) + (id - 1);
        pinyin_ime_select(pinyin_ime, cand_index);

        if(pinyin_ime_is_finished(pinyin_ime)) {
            lv_obj_add_flag(cand_panel, LV_OBJ_FLAG_HIDDEN);
            lv_memset_00(lv_ime->pinyin_input, sizeof(lv_ime->pinyin_input));
            return;
        }
        
        char * segments = pinyin_ime_get_segments(pinyin_ime);
        lv_snprintf(lv_ime->pinyin_input, sizeof(lv_ime->pinyin_input), "%s", segments);
        lv_textarea_add_text(ta, lv_ime->pinyin_input);

        lv_ime_pinyin_set_cand_page(obj, 0);
        if(lv_ime->mode == LV_IME_PINYIN_MODE_K9) lv_ime_pinyin_set_k9_cand_page(obj, 0);
    }
}

static void lv_ime_pinyin_style_change_event(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target(e);

    lv_ime_pinyin_t * lv_ime = (lv_ime_pinyin_t *)obj;

    if(code == LV_EVENT_STYLE_CHANGED) {
        const lv_font_t * font = lv_obj_get_style_text_font(obj, LV_PART_MAIN);
        lv_obj_set_style_text_font(lv_ime->cand_panel, font, 0);
    }
}

static void lv_ime_pinyin_clear_data(lv_obj_t * obj)
{
    lv_ime_pinyin_t * lv_ime = (lv_ime_pinyin_t *)obj;
    lv_memset_00(lv_ime->pinyin_input, sizeof(lv_ime->pinyin_input));

    lv_ime_pinyin_input_proc(obj);
}

static void lv_ime_pinyin_input_proc(lv_obj_t * obj)
{
    lv_ime_pinyin_t * lv_ime = (lv_ime_pinyin_t *)obj;

    pinyin_ime_t * pinyin_ime = lv_ime->pinyin_ime;
    if (!pinyin_ime) return;
    pinyin_ime_input(pinyin_ime, lv_ime->pinyin_input);

    //int cand_num = pinyin_ime_get_candidate_count(pinyin_ime);
    char * segments = pinyin_ime_get_segments(pinyin_ime);
    //char * result = pinyin_ime_get_result(pinyin_ime);

    if(strlen(lv_ime->pinyin_input) > 0 && strlen(segments) > 0) {
        lv_obj_t * ta = lv_keyboard_get_textarea(lv_ime->kb);
        for(uint32_t i = 0; i < strlen(lv_ime->pinyin_input); i++) {
            lv_textarea_del_char(ta);
        }

        lv_snprintf(lv_ime->pinyin_input, sizeof(lv_ime->pinyin_input), "%s", segments);
        lv_textarea_add_text(ta, lv_ime->pinyin_input);
    }

    /*
    printf("[pinyin_ime] 剩余分词: '%s'  结果: '%s'  候选(%d):", 
	       segments ? segments : "",
	       result ? result : "",
	       cand_num);
	for (int i = 0; i < cand_num; i++)
		printf(" %d.%s", i, pinyin_ime_get_candidate(pinyin_ime, i));
	printf("\n");
    */
    
    lv_ime_pinyin_set_cand_page(obj, 0);
    if(lv_ime->mode == LV_IME_PINYIN_MODE_K9) lv_ime_pinyin_set_k9_cand_page(obj, 0);
}

static void lv_ime_pinyin_set_cand_page(lv_obj_t * obj, uint16_t cand_page)
{
    lv_ime_pinyin_t * lv_ime = (lv_ime_pinyin_t *)obj;
    lv_ime->cand_page = cand_page;

    pinyin_ime_t * pinyin_ime = lv_ime->pinyin_ime;
    if (!pinyin_ime) return;

    int cand_num = pinyin_ime_get_candidate_count(pinyin_ime);

    uint16_t page_count = cand_num / LV_IME_PINYIN_CAND_TEXT_NUM;   /* 完整页数 */
    uint16_t sur = cand_num % LV_IME_PINYIN_CAND_TEXT_NUM;        /* 最后一页剩余候选数 */
    if(sur) page_count++;

    /* 清空候选显示缓冲 */
    for(uint8_t i = 0; i < LV_IME_PINYIN_CAND_TEXT_NUM; i++) {
        lv_btnm_def_pinyin_sel_map[i+1] = " ";
    }

    /* 重新填充候选汉字 */
    //printf("[pinyin_ime] cand_page=%d ", cand_page);
    uint16_t offset = cand_page * LV_IME_PINYIN_CAND_TEXT_NUM;
    for(uint8_t i = 0; i < LV_IME_PINYIN_CAND_TEXT_NUM; i++) {
        if(offset + i > cand_num - 1) break;
        char * candidate = pinyin_ime_get_candidate(pinyin_ime, offset + i);
        lv_btnm_def_pinyin_sel_map[i+1] = candidate ? candidate : " ";
        //printf("%s", lv_btnm_def_pinyin_sel_map[i+1]);
    }
    //printf("\n");

    lv_obj_t * ta = lv_keyboard_get_textarea(lv_ime->kb);
    if(page_count > 0) {
        lv_obj_clear_flag(lv_ime->cand_panel, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(ta, LV_OBJ_FLAG_CLICKABLE);
    }
    else {
        lv_obj_add_flag(lv_ime->cand_panel, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(ta, LV_OBJ_FLAG_CLICKABLE);
    }
}

static void lv_ime_pinyin_set_k9_cand_page(lv_obj_t * obj, uint16_t k9_cand_page)
{
    lv_ime_pinyin_t * lv_ime = (lv_ime_pinyin_t *)obj;
    lv_ime->k9_cand_page = k9_cand_page;

    pinyin_ime_t * pinyin_ime = lv_ime->pinyin_ime;
    if (!pinyin_ime) return;

    int cand_num = pinyin_ime_get_k9_exact_count(pinyin_ime);

    uint16_t page_count = cand_num / 3;   /* 完整页数 */
    uint16_t sur = cand_num % 3;        /* 最后一页剩余候选数 */
    if(sur) page_count++;
    

    /* 清空候选显示缓冲 */
    for(uint8_t i = 0; i < 3; i++) {
        kb_map_k9[KB_K9_EXACT_OFFSET + i] = " ";
    }

    /* 重新填充候选拼音 */
    //printf("[pinyin_ime] k9_exact_page=%d ", k9_cand_page);
    uint16_t cand_offset = k9_cand_page * 3;
    for(uint8_t i = 0; i < 3; i++) {
        if(cand_offset + i > cand_num - 1) break;
        char * candidate = pinyin_ime_get_k9_exact(pinyin_ime, cand_offset + i);
        kb_map_k9[KB_K9_EXACT_OFFSET + i] = candidate ? candidate : " ";
        //printf("%s,", kb_map_k9[KB_K9_EXACT_OFFSET + i]);
    }
    //printf("\n");
}

static void lv_ime_pinyin_select_k9_cand(lv_obj_t * obj, uint16_t k9_cand_index)
{
    lv_ime_pinyin_t * lv_ime = (lv_ime_pinyin_t *)obj;

    pinyin_ime_t * pinyin_ime = lv_ime->pinyin_ime;
    if (!pinyin_ime) return;

    //printf("[pinyin_ime] k9_select_exact=%d\n", k9_cand_index);
    pinyin_ime_select_k9_exact(pinyin_ime, k9_cand_index);

    char * segments = pinyin_ime_get_segments(pinyin_ime);

    if(strlen(lv_ime->pinyin_input) > 0 && strlen(segments) > 0) {
        lv_obj_t * ta = lv_keyboard_get_textarea(lv_ime->kb);
        for(uint32_t i = 0; i < strlen(lv_ime->pinyin_input); i++) {
            lv_textarea_del_char(ta);
        }

        lv_snprintf(lv_ime->pinyin_input, sizeof(lv_ime->pinyin_input), "%s", segments);
        lv_textarea_add_text(ta, lv_ime->pinyin_input);
    }

    //printf("[%s]\n", lv_ime->pinyin_input);
    
    lv_ime_pinyin_set_cand_page(obj, 0);
    if (lv_ime->mode == LV_IME_PINYIN_MODE_K9)
        lv_ime_pinyin_set_k9_cand_page(obj, 0);
}

#endif  /*LV_USE_IME_PINYIN*/