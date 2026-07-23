#include "page_txt.h"
#include "main.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "lvgl/src/misc/lv_txt.h"

#define MAX_LINES   9

typedef struct
{
    BasePage base;
    char * file_content;
    long file_size;
    int current_page;
    int total_pages;
    long * page_starts;
    lv_obj_t * text_label;
    lv_obj_t * page_label;
} TxtPage;

static void back_click(lv_event_t * e);
static void next_page_click(lv_event_t * e);
static void prev_page_click(lv_event_t * e);
static void update_display(TxtPage * txt);
static void build_pages(TxtPage * txt);
static void txt_page_destroy(void * page);

static uint32_t get_next_utf8_char(const char * str, int * len)
{
    uint32_t ofs  = 0;
    uint32_t code = _lv_txt_encoded_next(str, &ofs);
    *len          = ofs;
    return code;
}

static int is_english_letter(uint32_t c)
{
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

BasePage * page_txt_create(char * filename)
{
    TxtPage * txt = malloc(sizeof(TxtPage));
    if(!txt) return NULL;
    memset(txt, 0, sizeof(TxtPage));
    
    sys_set_dont_timeout(true);
    
    lv_obj_t * screen = lv_obj_create(lv_scr_act());
    lv_obj_remove_style_all(screen);
    lv_obj_set_size(screen, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_color(screen, lv_color_white(), 0);

    FILE * fp = fopen(filename, "r");
    if(fp == NULL) {
        lv_obj_t * error_label = lv_label_create(screen);
        lv_label_set_text_fmt(error_label, "无法打开此文件\n%s", strerror(errno));
        lv_obj_align(error_label, LV_ALIGN_CENTER, 0, 0);
        lv_obj_set_style_text_align(error_label, LV_TEXT_ALIGN_CENTER, 0);

        lv_obj_t * btn_back = lv_btn_create(screen);
        lv_obj_set_size(btn_back, lv_pct(25), lv_pct(12));
        lv_obj_align(btn_back, LV_ALIGN_BOTTOM_LEFT, 0, 0);
        lv_obj_t * btn_back_label = lv_label_create(btn_back);
        lv_label_set_text(btn_back_label, CUSTOM_SYMBOL_BACK "");
        lv_obj_center(btn_back_label);
        lv_obj_add_event_cb(btn_back, back_click, LV_EVENT_CLICKED, txt);

        txt->base.obj        = screen;
        txt->base.on_destroy = txt_page_destroy;
        return (BasePage *)txt;
    }

    fseek(fp, 0, SEEK_END);
    txt->file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    txt->file_content = (char *)malloc(txt->file_size + 1);
    if(!txt->file_content) {
        fclose(fp);
        lv_obj_t * error_label = lv_label_create(screen);
        lv_label_set_text(error_label, "Error: 没有多余内存!");
        lv_obj_align(error_label, LV_ALIGN_CENTER, 0, 0);

        lv_obj_t * btn_back = lv_btn_create(screen);
        lv_obj_set_size(btn_back, lv_pct(25), lv_pct(12));
        lv_obj_align(btn_back, LV_ALIGN_BOTTOM_LEFT, 0, 0);
        lv_obj_t * btn_back_label = lv_label_create(btn_back);
        lv_label_set_text(btn_back_label, CUSTOM_SYMBOL_BACK "");
        lv_obj_center(btn_back_label);
        lv_obj_add_event_cb(btn_back, back_click, LV_EVENT_CLICKED, txt);

        txt->base.obj        = screen;
        txt->base.on_destroy = txt_page_destroy;
        return (BasePage *)txt;
    }

    size_t read_size             = fread(txt->file_content, 1, txt->file_size, fp);
    txt->file_content[read_size] = '\0';
    fclose(fp);

    // 创建文本标签
    txt->text_label = lv_label_create(screen);
    lv_obj_set_width(txt->text_label, lv_pct(95));
    lv_obj_align(txt->text_label, LV_ALIGN_TOP_MID, 0, 10);
    lv_label_set_long_mode(txt->text_label, LV_LABEL_LONG_WRAP);

    // 创建页码标签
    txt->page_label = lv_label_create(screen);
    lv_obj_align(txt->page_label, LV_ALIGN_BOTTOM_MID, 0, -5);

    // 强制更新布局，确保获取正确的宽度
    lv_obj_update_layout(screen);

    // 预分页
    build_pages(txt);

    lv_obj_t * btn_back = lv_btn_create(screen);
    lv_obj_set_size(btn_back, lv_pct(25), lv_pct(12));
    lv_obj_align(btn_back, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    lv_obj_t * btn_back_label = lv_label_create(btn_back);
    lv_label_set_text(btn_back_label, CUSTOM_SYMBOL_BACK "");
    lv_obj_center(btn_back_label);
    lv_obj_add_event_cb(btn_back, back_click, LV_EVENT_CLICKED, txt);

    lv_obj_t * btn_prev = lv_btn_create(screen);
    lv_obj_set_size(btn_prev, 40, 28);
    lv_obj_align(btn_prev, LV_ALIGN_BOTTOM_RIGHT, -43, 0);
    lv_obj_t * btn_prev_label = lv_label_create(btn_prev);
    lv_label_set_text(btn_prev_label, "<");
    lv_obj_center(btn_prev_label);
    lv_obj_add_event_cb(btn_prev, prev_page_click, LV_EVENT_CLICKED, txt);

    lv_obj_t * btn_next = lv_btn_create(screen);
    lv_obj_set_size(btn_next, 40, 28);
    lv_obj_align(btn_next, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
    lv_obj_t * btn_next_label = lv_label_create(btn_next);
    lv_label_set_text(btn_next_label, ">");
    lv_obj_center(btn_next_label);
    lv_obj_add_event_cb(btn_next, next_page_click, LV_EVENT_CLICKED, txt);

    // 设置 BasePage 成员
    txt->base.obj        = screen;
    txt->base.on_destroy = txt_page_destroy;

    // 初始显示第一页
    update_display(txt);

    return (BasePage *)txt;
}

// ---------- 预分页函数 ----------
static void build_pages(TxtPage * txt)
{
    if(!txt->file_content || txt->file_size == 0) {
        txt->total_pages = 1;
        txt->page_starts = malloc(sizeof(long));
        if(!txt->page_starts) return;
        txt->page_starts[0] = 0;
        return;
    }

    lv_font_t * font     = lv_obj_get_style_text_font(txt->text_label, 0);
    lv_coord_t max_width = lv_obj_get_content_width(txt->text_label);
    if(max_width <= 0) max_width = 200; // 默认宽度
    if(!font) font = lv_font_default();

    // 估算最大页数
    int max_pages    = (txt->file_size + 1) / 1 + 1;
    txt->page_starts = malloc(sizeof(long) * max_pages);
    if(!txt->page_starts) {
        txt->total_pages = 1;
        txt->page_starts = malloc(sizeof(long));
        if(!txt->page_starts) return;
        txt->page_starts[0] = 0;
        return;
    }

    int page_idx                 = 0;
    long pos                     = 0;
    txt->page_starts[page_idx++] = 0;

    while(pos < txt->file_size) {
        int line_count        = 0;
        lv_coord_t line_width = 0;

        while(pos < txt->file_size && line_count < MAX_LINES) {
            unsigned char c = txt->file_content[pos];

            // 处理原始换行符
            if(c == '\n' || c == '\r') {
                if(line_width > 0) {
                    line_count++;
                    if(line_count >= MAX_LINES) break;
                    line_width = 0;
                }
                if(c == '\r' && pos + 1 < txt->file_size && txt->file_content[pos + 1] == '\n') {
                    pos += 2;
                } else {
                    pos++;
                }
                while(pos < txt->file_size && (txt->file_content[pos] == '\n' || txt->file_content[pos] == '\r')) {
                    line_count++;
                    if(line_count >= MAX_LINES) break;
                    if(txt->file_content[pos] == '\r' && pos + 1 < txt->file_size &&
                       txt->file_content[pos + 1] == '\n') {
                        pos += 2;
                    } else {
                        pos++;
                    }
                }
                continue;
            }

            int char_len;
            uint32_t ch = get_next_utf8_char(&txt->file_content[pos], &char_len);

            if(is_english_letter(ch)) {
                long word_start       = pos;
                int word_len          = 0;
                lv_coord_t word_width = 0;
                while(pos + word_len < txt->file_size) {
                    int sub_len;
                    uint32_t sub_ch = get_next_utf8_char(&txt->file_content[pos + word_len], &sub_len);
                    if(!is_english_letter(sub_ch)) break;
                    word_width += lv_font_get_glyph_width(font, sub_ch, 0);
                    word_len += sub_len;
                }

                if(line_width + word_width <= max_width) {
                    line_width += word_width;
                    pos += word_len;
                    continue;
                } else {
                    if(line_width > 0) {
                        line_count++;
                        if(line_count >= MAX_LINES) break;
                        line_width = 0;
                        continue;
                    } else {
                        // 超长单词，强制拆分
                        lv_coord_t tmp_width = 0;
                        int split_len        = 0;
                        while(split_len < word_len) {
                            int sub_len;
                            uint32_t sub_ch = get_next_utf8_char(&txt->file_content[word_start + split_len], &sub_len);
                            lv_coord_t cw   = lv_font_get_glyph_width(font, sub_ch, 0);
                            if(tmp_width + cw > max_width) break;
                            tmp_width += cw;
                            split_len += sub_len;
                        }
                        if(split_len == 0) split_len = word_len;
                        pos += split_len;
                        line_width = 0;
                        line_count++;
                        if(line_count >= MAX_LINES) break;
                        continue;
                    }
                }
            }

            lv_coord_t char_width = lv_font_get_glyph_width(font, ch, 0);
            if(line_width + char_width <= max_width) {
                line_width += char_width;
                pos += char_len;
            } else {
                line_count++;
                if(line_count >= MAX_LINES) break;
                line_width = 0;
                continue;
            }
        }

        if(pos < txt->file_size) {
            txt->page_starts[page_idx++] = pos;
        } else {
            break;
        }
    }

    txt->total_pages = page_idx;
}

static void update_display(TxtPage * txt)
{
    if(!txt->file_content || !txt->text_label || !txt->page_label || !txt->page_starts) return;

    long start_pos = txt->page_starts[txt->current_page];
    long end_pos = (txt->current_page + 1 < txt->total_pages) ? txt->page_starts[txt->current_page + 1] : txt->file_size;

    long page_len = end_pos - start_pos;
    if(page_len <= 0) {
        lv_label_set_text(txt->text_label, "");
        return;
    }

    lv_font_t * font       = lv_obj_get_style_text_font(txt->text_label, 0);
    lv_coord_t max_width   = lv_obj_get_content_width(txt->text_label);
    lv_coord_t space_width = lv_font_get_glyph_width(font, ' ', 0);

    static char display_buffer[2048];       // 改为 static，避免栈溢出
    display_buffer[0] = '\0';              // 每次使用前清空
    int buf_idx           = 0;
    int line_count        = 0;
    lv_coord_t line_width = 0;

    long i = start_pos;
    while(i < end_pos && line_count < MAX_LINES) {
        unsigned char c = txt->file_content[i];

        if(c == '\n' || c == '\r') {
            if(line_width > 0) {
                if(line_count < MAX_LINES - 1 && buf_idx < sizeof(display_buffer) - 1) {
                    display_buffer[buf_idx++] = '\n';
                }
                line_count++;
                line_width = 0;
                if(line_count >= MAX_LINES) break;
            }
            if(c == '\r' && i + 1 < end_pos && txt->file_content[i + 1] == '\n') {
                i += 2;
            } else {
                i++;
            }
            while(i < end_pos && (txt->file_content[i] == '\n' || txt->file_content[i] == '\r')) {
                if(line_count < MAX_LINES - 1 && buf_idx < sizeof(display_buffer) - 1) {
                    display_buffer[buf_idx++] = '\n';
                }
                line_count++;
                if(line_count >= MAX_LINES) break;
                if(txt->file_content[i] == '\r' && i + 1 < end_pos && txt->file_content[i + 1] == '\n') {
                    i += 2;
                } else {
                    i++;
                }
            }
            continue;
        }

        int char_len;
        uint32_t ch = get_next_utf8_char(&txt->file_content[i], &char_len);

        if(is_english_letter(ch)) {
            long word_start       = i;
            int word_len          = 0;
            lv_coord_t word_width = 0;
            while(word_start + word_len < end_pos) {
                int sub_len;
                uint32_t sub_ch = get_next_utf8_char(&txt->file_content[word_start + word_len], &sub_len);
                if(!is_english_letter(sub_ch)) break;
                word_width += lv_font_get_glyph_width(font, sub_ch, 0);
                word_len += sub_len;
            }

            if(line_width + word_width <= max_width) {
                for(int j = 0; j < word_len; j++) {
                    display_buffer[buf_idx++] = txt->file_content[i + j];
                }
                line_width += word_width;
                i += word_len;
                continue;
            } else {
                if(line_width > 0) {
                    int fill_spaces = (max_width - line_width) / space_width;
                    for(int s = 0; s < fill_spaces; s++) {
                        if(buf_idx < sizeof(display_buffer) - 1) display_buffer[buf_idx++] = ' ';
                    }
                    if(line_count < MAX_LINES - 1 && buf_idx < sizeof(display_buffer) - 1) {
                        display_buffer[buf_idx++] = '\n';
                    }
                    line_count++;
                    if(line_count >= MAX_LINES) break;
                    line_width = 0;
                    continue;
                } else {
                    // 超长单词，强制拆分（实际不会发生）
                    lv_coord_t tmp_width = 0;
                    int split_len        = 0;
                    while(split_len < word_len) {
                        int sub_len;
                        uint32_t sub_ch = get_next_utf8_char(&txt->file_content[word_start + split_len], &sub_len);
                        lv_coord_t cw   = lv_font_get_glyph_width(font, sub_ch, 0);
                        if(tmp_width + cw > max_width) break;
                        tmp_width += cw;
                        split_len += sub_len;
                    }
                    if(split_len == 0) split_len = word_len;
                    for(int j = 0; j < split_len; j++) {
                        display_buffer[buf_idx++] = txt->file_content[i + j];
                    }
                    i += split_len;
                    line_width = 0;
                    line_count++;
                    if(line_count >= MAX_LINES) break;
                    continue;
                }
            }
        }

        lv_coord_t char_width = lv_font_get_glyph_width(font, ch, 0);
        if(line_width + char_width <= max_width) {
            for(int j = 0; j < char_len; j++) {
                display_buffer[buf_idx++] = txt->file_content[i + j];
            }
            line_width += char_width;
            i += char_len;
        } else {
            if(line_count < MAX_LINES - 1 && buf_idx < sizeof(display_buffer) - 1) {
                display_buffer[buf_idx++] = '\n';
            }
            line_count++;
            if(line_count >= MAX_LINES) break;
            line_width = 0;
            continue;
        }
    }

    display_buffer[buf_idx] = '\0';
    lv_label_set_text(txt->text_label, display_buffer);

    char page_info[32];
    snprintf(page_info, sizeof(page_info), "%d/%d", txt->current_page + 1, txt->total_pages);
    lv_label_set_text(txt->page_label, page_info);
}

static void back_click(lv_event_t * e)
{
    page_back();
}

static void next_page_click(lv_event_t * e)
{
    TxtPage * txt = (TxtPage *)lv_event_get_user_data(e);
    if(txt->current_page + 1 < txt->total_pages) {
        txt->current_page++;
        update_display(txt);
    }
}

static void prev_page_click(lv_event_t * e)
{
    TxtPage * txt = (TxtPage *)lv_event_get_user_data(e);
    if(txt->current_page > 0) {
        txt->current_page--;
        update_display(txt);
    }
}

static void txt_page_destroy(void * page)
{
    TxtPage * txt = (TxtPage *)page;
    if(txt->file_content) free(txt->file_content);
    if(txt->page_starts) free(txt->page_starts);
    sys_set_dont_timeout(false);
}