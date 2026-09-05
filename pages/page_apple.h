#ifndef PROJ_PAGE_APPLE_H
#define PROJ_PAGE_APPLE_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "../lvgl/lvgl.h"
#include "lv_drv_conf.h"
#include "../lv_lib_100ask/lv_lib_100ask.h"
#include <unistd.h>
#include <stdio.h>
#include <stdbool.h>

#include "page_manager.h"


/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/
BasePage * page_video_create(char * filename);

/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
