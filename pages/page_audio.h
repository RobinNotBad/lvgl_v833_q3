#ifndef PROJ_PAGE_AUDIO_H
#define PROJ_PAGE_AUDIO_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "../lvgl/lvgl.h"
#include "../lv_lib_100ask/lv_lib_100ask.h"
#include "page_manager.h"
#include "platform/ff_player.h"
#include "platform/audio_ctrl.h"
#include "views/lv_text_clock.h"
#include "battery_manager.h"
#include <unistd.h>
#include <stdio.h>
#include <stdbool.h>
#include <main.h>

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/
BasePage * page_audio_create(char * filename);

/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
