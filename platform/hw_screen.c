
#include "hw_screen.h"

#include "lv_drivers/display/fbdev.h"
#include "lv_drivers/indev/evdev.h"
#include <fcntl.h>
#include <sys/ioctl.h>
#include "main.h"
#include "page_manager.h"
#include <linux/fb.h>
#include <sys/stat.h>
#include "lvgl/lvgl.h"

int dispd;  // 背光
int fbd;    // 帧缓冲设备
static lv_color_t * bufA = NULL;
static lv_color_t * bufB = NULL;

struct fb_var_screeninfo * vinfo; // 屏幕参数
uint32_t lcd_brightness;

/**
 * 初始化LCD，设置旋转方向
 */
void lcd_init(void)
{
    dispd = open(BACKLIGHT_DEVICE, O_RDWR);
    fbdev_init();
    fbd = fbdev_get_fbd();

    vinfo         = fbdev_get_vinfo();
    vinfo->rotate = 3;
    ioctl(fbd, 0x4601u, vinfo);

    bufA = malloc(DISP_BUF_SIZE * sizeof(lv_color_t));
    bufB = malloc(DISP_BUF_SIZE * sizeof(lv_color_t));

    static lv_disp_draw_buf_t disp_buf;
    lv_disp_draw_buf_init(&disp_buf, bufA, bufB, DISP_BUF_SIZE);

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.draw_buf = &disp_buf;
    disp_drv.flush_cb = fbdev_flush;
    disp_drv.hor_res  = LV_SCR_WIDTH;
    disp_drv.ver_res  = LV_SCR_HEIGHT;
    lv_disp_t * disp  = lv_disp_drv_register(&disp_drv);
    lv_disp_set_default(disp);

    evdev_init();
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type     = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb  = evdev_read;
    /*lv_indev_t *indev =  */lv_indev_drv_register(&indev_drv);

    lcd_off();
    lcd_on();
    touch_on();
}

/**
 * 释放LCD
 */
void lcd_close(void)
{
    if (dispd) close(dispd);
    if (fbd) close(fbd);
    // 此处不要释放bufA和bufB，别问为什么，问就是会崩
}

/**
 * 点亮LCD
 */
void lcd_on(void)
{
    int buffer[8] = {0};
    buffer[1]     = 1;
    ioctl(dispd, 0xFu, buffer);
    printf("[lcd]on\n");
}

/**
 * 熄灭LCD
 */
void lcd_off(void)
{
    int buffer[8] = {0};
    ioctl(dispd, 0xFu, buffer);
    printf("[lcd]off\n");
}

/**
 * LCD刷屏
 */
void lcd_refresh(void)
{
    ioctl(fbd, 0x4606u, vinfo);
}

/**
 * 设置LCD背光亮度
 * 内部函数，不保存亮度值
 */
void lcd_set_brightness_inner(int brightness)
{
    int buffer[8] = {0};
    buffer[1]     = brightness;
    ioctl(dispd, 0x102u, buffer);
}

/**
 * 设置LCD背光亮度
 * 对外接口，会保存亮度值
 */
void lcd_set_brightness(int brightness)
{
    lcd_brightness = brightness;
    lcd_set_brightness_inner(brightness);
}

/**
 * 获取LCD背光亮度
 */
uint32_t lcd_get_brightness(void)
{
    return lcd_brightness;
}



/**
 * 启用触摸
 */
void touch_on(void)
{
    int tpd = open(TP_SWITCH_DEVICE, 526338);
    write(tpd, "1", 1u);
    close(tpd);
    printf("[tp]on\n");
}

/**
 * 关闭触摸
 */
void touch_off(void)
{
    int tpd = open(TP_SWITCH_DEVICE, 526338);
    write(tpd, "0", 1u);
    close(tpd);
    printf("[tp]off\n");
}
