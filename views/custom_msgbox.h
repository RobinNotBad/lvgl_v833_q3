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
 * @brief               创建自定义弹窗
 * @param title         标题
 * @param txt           文本
 * @param btn_txts      按钮文本，最后一个需要为NULL
 * @param add_close_btn 是否添加关闭按钮
 * @return              返回msgbox对象
 */
lv_obj_t * custom_msgbox_create(const char *title, const char *txt, const char **btn_txts, bool *add_close_btn);

/**
 * @brief               关闭自定义弹窗
 * @param msgbox        要关掉的自定义弹窗
 */
void custom_msgbox_close(lv_obj_t *msgbox);

/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
