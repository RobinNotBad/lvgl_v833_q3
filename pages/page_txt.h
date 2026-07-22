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
#include "platform/page_manager.h"

/*********************
 *      DEFINES
 *********************/

BasePage * page_txt_create(char * filename);

#ifdef __cplusplus
}
#endif

#endif