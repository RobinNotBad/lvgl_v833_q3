#ifndef PLAT_LV_UTILS_H
#define PLAT_LV_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include <unistd.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "lvgl/lvgl.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/
lv_font_t * font_get(uint16_t weight, uint16_t font_style);
lv_coord_t lv_obj_get_width_pct(lv_obj_t * obj, float pct);
lv_coord_t lv_obj_get_height_pct(lv_obj_t * obj, float pct);


/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
