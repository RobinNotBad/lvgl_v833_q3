#ifndef HW_SCREEN_H
#define HW_SCREEN_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/types.h>

/*********************
 *      DEFINES
 *********************/
#define DISP_BUF_SIZE (LV_SCR_WIDTH * LV_SCR_HEIGHT)
#define SCREEN_BRIGHTNESS_DEFAULT 25

#define BACKLIGHT_DEVICE "/dev/disp"
#define TP_SWITCH_DEVICE "/proc/sprocomm_tpInfo"

extern int dispd;  // 背光
extern int fbd;    // 帧缓冲设备
extern uint32_t lcd_brightness;

/**********************
 *      TYPEDEFS
 **********************/
void lcd_init(void);
void lcd_close(void);
void lcd_on(void);
void lcd_off(void);
void lcd_refresh(void);

void lcd_set_brightness(int brightness);
void lcd_set_brightness_inner(int brightness);
uint32_t lcd_get_brightness(void);

void touch_on(void);
void touch_off(void);


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
