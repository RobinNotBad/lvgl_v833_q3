#ifndef DENDRO_MAIN_H
#define DENDRO_MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdbool.h>
#include "lvgl/lvgl.h"

/*********************
 *      DEFINES
 *********************/
#define DISP_BUF_SIZE (LV_SCR_WIDTH * LV_SCR_HEIGHT)

#define PATH_MAX_LENGTH 1024

#define SCREEN_TIMEOUT_MS 30000
#define SCREEN_BRIGHTNESS_DEFAULT 25

#define THEME_COLOR 0xff78c05d

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/
extern char homepath[PATH_MAX_LENGTH];

extern int dispd;  // 背光
extern int fbd;    // 帧缓冲设备
extern int powerd; // 电源按钮
extern int homed;  // 主页按钮

extern uint32_t sleepTs;
extern uint32_t homeClickTs;
extern uint32_t backgroundTs;

extern uint8_t dont_deep_sleep_enabled;
extern uint8_t dont_timeout_enabled;

void lcd_set_brightness(int brightness);
uint32_t lcd_get_brightness(void);
void sys_sleep(void);
void sys_wake(void);
void sys_deep_sleep(void);
void sys_set_dont_deep_sleep(bool b);
void sys_set_dont_timeout(bool b);
void switch_robot(void);
void switch_background(void);
void switch_foreground(void);

uint32_t tick_get(void);
uint64_t ms_get(void);

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
