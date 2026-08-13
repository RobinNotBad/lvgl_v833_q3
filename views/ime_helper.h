#ifndef IME_HELPER_H
#define IME_HELPER_H

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

/**
 * 0: 使用非全屏输入页面（类似手机）
 * 1: 使用全屏输入页面，适合小屏设备（类似手表）
 */ 
#define IME_USE_FULLSCREEN 1

#if IME_USE_FULLSCREEN
    /**
     * 0：横向
     * 1：纵向
     */
    #define IME_LAYOUT_DIRECTION 1
#endif

/**********************
 *      TYPEDEFS
 **********************/
void ime_helper_init(void);
void lv_textarea_bind_ime(lv_obj_t * textarea);

/**********************
 * GLOBAL PROTOTYPES
 **********************/


/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
