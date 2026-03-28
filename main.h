#ifndef DENDRO_MAIN_H
#define DENDRO_MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/

/*********************
 *      DEFINES
 *********************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdbool.h>
#include "lvgl/lvgl.h"

/**********************
 *      TYPEDEFS
 **********************/
#define DISP_BUF_SIZE (LV_SCR_WIDTH * LV_SCR_HEIGHT)

#define PATH_MAX_LENGTH 1024

#define SCREEN_TIMEOUT_MS 30000
#define SCREEN_BRIGHTNESS_DEFAULT 25

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

extern bool dontDeepSleep;

void lcdSetBrightness(int brightness);
void sysSleep(void);
void sysWake(void);
void sysDeepSleep(void);
void setDontDeepSleep(bool b);
void switchRobot(void);
void switchBackground(void);
void switchForeground(void);

uint32_t tick_get(void);
uint64_t ms_get(void);

lv_style_t getFontStyle(const char * filename, uint16_t weight, uint16_t style);

/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
