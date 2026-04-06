// page_txt.h
#ifndef PROJ_PAGE_TXT_H
#define PROJ_PAGE_TXT_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
 
#include "../lvgl/lvgl.h"
#include "page_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lvgl/src/misc/lv_txt.h"

/*********************
 *      DEFINES
 *********************/
 
#define MAX_LINES 9

BasePage * page_txt_create(char * filename);

#ifdef __cplusplus
}
#endif

#endif