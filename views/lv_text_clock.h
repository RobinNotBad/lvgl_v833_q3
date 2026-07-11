/**
 * @file lv_text_clock.h
 * 文字时钟控件
 */

#ifndef LV_TEXT_CLOCK_H
#define LV_TEXT_CLOCK_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "../lvgl/lvgl.h"
#include <stdio.h>
#include <time.h>
#include <string.h>

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/* 时钟数据结构 */
typedef struct {
    lv_label_t label;               /* 基础对象，必须放在第一位 */
    lv_timer_t * timer;         /* 更新定时器 */
    char time_str[32];          /* 时间字符串缓冲区 */
} lv_text_clock_t;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * 创建文字时钟对象
 * @param parent 父对象
 * @return 指向新时钟对象的指针
 */
lv_obj_t * lv_text_clock_create(lv_obj_t * parent);

/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_TEXT_CLOCK_H*/