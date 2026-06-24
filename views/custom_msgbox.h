#ifndef CUSTOM_MSGBOX_H
#define CUSTOM_MSGBOX_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "lvgl/lvgl.h"
#include <unistd.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * 自定义msgbox，自带半透明黑色背景，销毁时自动连带背景一起跑路
 * @param title         标题
 * @param txt           文本
 * @param btn_txts      按钮文本，列表最后一个需要为NULL
 * @param add_close_btn 是否添加关闭按钮
 * @return              lv_msgbox的指针
 */
lv_obj_t * custom_msgbox_create(const char *title, const char *txt, const char **btn_txts, bool add_close_btn);

/**
 * 仿照安卓toast，自带半透明灰色背景，3s后自动销毁
 * @param txt           文本
 * @return              lv_label的指针
 */
lv_obj_t * custom_toast_create(const char *txt);
/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
