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
#define PATH_MAX_LENGTH 1024
#define SCREEN_TIMEOUT_MS 30000
#define THEME_COLOR 0xff78c05d
#define TIMIDITY_CFG_DEFAULT "/mnt/app/dendro/midi/timidity.cfg"
#define CPU_POWER_CTRL_ENABLED 0
#define TOUCH_REVERSE_X_DEFAULT false
#define TOUCH_REVERSE_Y_DEFAULT false

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/
extern char homepath[PATH_MAX_LENGTH];

extern uint32_t ts_sleep;
extern uint32_t ts_background;

extern uint8_t dont_deep_sleep_enabled;
extern uint8_t dont_timeout_enabled;

void sys_sleep(void);
void sys_wake(void);
void sys_deep_sleep(void);
void sys_set_dont_deep_sleep(bool b);
void sys_set_dont_timeout(bool b);

uint32_t tick_get(void);
uint64_t ms_get(void);

/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
