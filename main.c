#include "main.h"

#include "lv_drivers/display/fbdev.h"
#include "lv_drivers/indev/evdev.h"
#include "lv_lib_100ask/lv_lib_100ask.h"
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/stat.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <string.h>
#include "platform/audio_ctrl.h"
#include "platform/battery_manager.h"

// 请教DeepSeek实现了简易页面管理器，100ask那个实际上不太好用……
#include "pages/page_manager.h"
#include "pages/page_home.h"

/*
#include "pages/page_demo.h"
#include "pages/page_calculator.h"
#include "pages/page_audio.h"
#include "pages/page_file_manager.h"
#include "pages/page_apple.h"
#include "pages/page_image.h"
#include "pages/page_ftp.h"
*/

struct fb_var_screeninfo * vinfo; // 屏幕参数

char homepath[PATH_MAX_LENGTH];

int dispd;  // 背光
int fbd;    // 帧缓冲设备
int powerd; // 电源按钮
int homed;  // 主页按钮

uint32_t ts_sleep = -1;
uint32_t ts_home_click = -1;
uint32_t ts_background = -1;
uint32_t lcd_brightness = SCREEN_BRIGHTNESS_DEFAULT;

bool is_screen_timeout = false;
uint8_t dont_deep_sleep_enabled = 0;
uint8_t dont_timeout_enabled = 0;

void key_read_power(void);
void key_read_home(void);
void lcd_init(void);
void lcd_on(void);
void lcd_off(void);
void lcd_refresh(void);
void touch_on(void);
void touch_off(void);
void lcd_detect_timeout(void);
void lcd_set_brightness_inner(int brightness);

static lv_style_t style_default;

int main(int argc, char * argv[])
{

    printf("ciallo lvgl\n");
#if LV_USE_PERF_MONITOR
    printf("monitor on\n");
#endif

    // 获取可执行文件目录并直接切换，避免相对路径出错
    ssize_t len = readlink("/proc/self/exe", homepath, sizeof(homepath) - 1);
    if(len != -1) {
        char * last_slash = strrchr(homepath, '/');
        if(last_slash) {
            *last_slash = '\0';
            chdir(homepath);
            printf("running at: %s\n", homepath);
        }
    }

    powerd = open("/dev/input/event1", O_RDWR);
    fcntl(powerd, 4, 2048);
    homed = open("/dev/input/event2", O_RDWR);
    fcntl(homed, 4, 2048);

    bool isDaemonMode = true;

    for(uint32_t i = 0; i < argc; i++) {
        char * arg = argv[i];
        printf("argv[%d] = %s\n", i, arg);
        if(strcmp(arg, "-d") == 0) {
            isDaemonMode = false;
        }

        if(strcmp(arg, "-w") == 0) {
            daemon(1, 0);
            switch_background();
            while(1) {
                usleep(25000);
                key_read_home();
            }
        }
    }

    printf("kill robot\n");
    system("killall robotd");
    system("killall robot_run");
    system("killall robot_run_1");
    usleep(100000);


    if(isDaemonMode) daemon(1, 0);
    // daemon函数将本程序置于后台，脱离终端
    // 若要进行调试，请使用-d参数

    setenv("TZ", "CST-8", 1);
    tzset();

    dispd = open("/dev/disp", O_RDWR);
    fbdev_init();
    fbd = fbdev_get_fbd();
    lcd_init();
    lcd_off();
    lcd_on();
    lcd_set_brightness(SCREEN_BRIGHTNESS_DEFAULT);
    touch_on();

    lv_init();

    static lv_color_t bufA[DISP_BUF_SIZE];
    static lv_color_t bufB[DISP_BUF_SIZE];

    static lv_disp_draw_buf_t disp_buf;
    lv_disp_draw_buf_init(&disp_buf, bufA, bufB, DISP_BUF_SIZE);

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.draw_buf = &disp_buf;
    disp_drv.flush_cb = fbdev_flush;
    disp_drv.hor_res  = 240;
    disp_drv.ver_res  = 240;
    lv_disp_t * disp  = lv_disp_drv_register(&disp_drv);
    lv_disp_set_default(disp);

    evdev_init();
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type     = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb  = evdev_read;
    /*lv_indev_t *indev =  */lv_indev_drv_register(&indev_drv);

    lv_ffmpeg_init();

    audio_init();

    

    lv_freetype_init(128, 4, 0);

    lv_ft_info_t ft_info;
    ft_info.name   = "./res/font.ttf";
    ft_info.weight = 16;
    ft_info.style  = FT_FONT_STYLE_NORMAL;
    ft_info.mem    = NULL;

    if(lv_ft_font_init(&ft_info)) {
        lv_theme_t * theme =
            lv_theme_default_init(disp, lv_color_hex(THEME_COLOR), lv_color_hex(THEME_COLOR), true, ft_info.font);
        theme->font_normal = ft_info.font;
        theme->font_large  = ft_info.font;
        theme->font_small  = ft_info.font; // 为啥子设置不上？
        lv_disp_set_theme(disp, theme);

        lv_style_init(&style_default);
        lv_style_set_text_font(&style_default, ft_info.font);
        lv_obj_add_style(lv_scr_act(), &style_default, 0);
    }

    page_manager_init();
    page_open(page_home_create());

    while(1) {
        key_read_home();
        if(ts_background == -1) {
            key_read_power();
            if(ts_sleep == -1) {
                // 亮
                lv_timer_handler();
                lcd_refresh(); // 放在fbdev里不合适，反而会增大cpu占用且变卡，神金啊
                lcd_detect_timeout();
                usleep(5000);

            } else {
                // 灭
                // 如果插着电，别睡
                if(dont_deep_sleep_enabled){
                    ts_sleep = tick_get();
                }
                else if(tick_get() - ts_sleep >= 60000){
                    if(battery_get_status() == BATTERY_DISCHARGING) 
                        sys_deep_sleep();
                    else
                        ts_sleep = tick_get();
                }

                usleep(25000);
            }
        } else {
            usleep(25000);
        }
    }

    if(fbd) close(fbd);
    if(dispd) close(dispd);
    if(homed) close(homed);
    if(powerd) close(powerd);
    return 0;
}

/**
 * 获取时间
 */
uint32_t tick_get(void)
{
    static uint32_t start_ms = 0;
    if(start_ms == 0) {
        struct timeval tv_start;
        gettimeofday(&tv_start, NULL);
        start_ms = (tv_start.tv_sec * 1000000 + tv_start.tv_usec) / 1000;
    }

    struct timeval tv_now;
    gettimeofday(&tv_now, NULL);
    uint32_t now_ms;
    now_ms = (tv_now.tv_sec * 1000000 + tv_now.tv_usec) / 1000;

    uint32_t time_ms = now_ms - start_ms;
    return time_ms;
}

uint64_t ms_get(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec * 1000000 + tv.tv_usec) / 1000;
}

/**
 * 初始化LCD，设置旋转方向
 */
void lcd_init(void)
{
    vinfo         = fbdev_get_vinfo();
    vinfo->rotate = 3;
    ioctl(fbd, 0x4601u, vinfo);
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
 * 启用触摸
 */
void touch_on(void)
{
    int tpd = open("/proc/sprocomm_tpInfo", 526338);
    write(tpd, "1", 1u);
    close(tpd);
    printf("[tp]on\n");
}

/**
 * 关闭触摸
 */
void touch_off(void)
{
    int tpd = open("/proc/sprocomm_tpInfo", 526338);
    write(tpd, "0", 1u);
    close(tpd);
    printf("[tp]off\n");
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
 * 读取电源按钮
 */
void key_read_power(void)
{
    char buffer[16] = {0};
    while(read(powerd, buffer, 0x10u) > 0) {
        if(buffer[10] != 0x74) return;

        if(buffer[12] == 0x00) {
            printf("[key]power_up\n");
            evdev_refresh_press_ts();
            if(ts_sleep == -1)
                if(page_on_key(KEY_CODE_POWER, KEY_ACTION_UP)) continue;
            // 如果页面处理了按键事件，就不继续执行了

            if(ts_sleep == -1)
                sys_sleep(); // 没睡的给我睡
            else
                sys_wake(); // 睡着的起来

        } else if(buffer[12] == 0x01) {
            printf("[key]power_down\n");
            evdev_refresh_press_ts();
            if(ts_sleep == -1)
                if(page_on_key(KEY_CODE_POWER, KEY_ACTION_DOWN)) continue;
        }
    }
}

/**
 * 读取圆形HOME按钮
 */
void key_read_home(void)
{
    char buffer[16] = {0};
    while(read(homed, buffer, 0x10u) > 0) {
        if(buffer[10] != 0x73) return;

        if(buffer[12] == 0x00) {
            printf("[key]home_up\n");
            evdev_refresh_press_ts();
            if(ts_sleep == -1)
                if(page_on_key(KEY_CODE_HOME, KEY_ACTION_UP)) continue;
            // 如果页面处理了按键事件，就不继续执行了

            uint32_t ts = tick_get();
            if(ts_home_click != -1 && ts - ts_home_click <= 300) {
                switch_foreground();
                ts_home_click = -1;
            } else {
                ts_home_click = ts;
                if(ts_sleep == -1)
                    page_back(); // 没睡的返回
                else
                    sys_wake(); // 睡着的起来
            }
        } else if(buffer[12] == 0x01) {
            printf("[key]home_down\n");
            evdev_refresh_press_ts();
            if(ts_sleep == -1)
                if(page_on_key(KEY_CODE_HOME, KEY_ACTION_DOWN)) continue;
        }
    }
}

/**
 * 亮屏
 */
void sys_wake(void)
{
    if(ts_sleep != -1) {
        printf("[sys]wake\n");
        is_screen_timeout = false;
        ts_sleep       = -1;
        touch_on();
        lcd_on();
        lcd_set_brightness_inner(lcd_brightness);
        evdev_refresh_press_ts();
        system("echo interactive > /sys/devices/system/cpu/cpufreq/policy0/scaling_governor");
    }
}

/**
 * 熄屏
 */
void sys_sleep(void)
{
    if(ts_sleep == -1) {
        printf("[sys]sleep\n");
        ts_sleep = tick_get();
        touch_off();
        lcd_off();
        if(!dont_deep_sleep_enabled) system("echo powersave > /sys/devices/system/cpu/cpufreq/policy0/scaling_governor");
    }
}

/**
 * 睡死
 */
void sys_deep_sleep(void)
{
    printf("[sys]deep sleep\n");
    char buffer[16] = {0};
    while(read(powerd, buffer, 0x10u) > 0); // 清空电源键的缓冲区
    while(read(homed, buffer, 0x10u) > 0);  // 清空HOME键的缓冲区

    // 睡死过去，相当省电
    system("echo \"0\" >/sys/class/rtc/rtc0/wakealarm");
    system("echo \"0\" >/sys/class/rtc/rtc0/wakealarm");
    system("echo \"mem\" > /sys/power/state");

    // 按电源键会醒过来，继续执行下面的代码

    sys_wake(); // 那睡觉的起来了嗷（改到这里是为了防止其他醒来的情况，比如插拔usb）
    while(read(powerd, buffer, 0x10u) > 0); // 再次清空电源键的缓冲区，因为开机按的电源键也算数
}

/**
 * 检测是否超时熄屏
 */
void lcd_detect_timeout(void)
{
    if(dont_timeout_enabled){
        is_screen_timeout = false;
        evdev_refresh_press_ts();
        return;
    }

    uint64_t timeout_ms = ms_get() - evdev_get_press_ts();
    if(timeout_ms < SCREEN_TIMEOUT_MS){
        if(is_screen_timeout) {
            is_screen_timeout = false;
            lcd_set_brightness_inner(lcd_brightness);
            printf("[lcd-timeout]restore brightness\n");
        }
    }
    else if(!is_screen_timeout) {
        is_screen_timeout = true;
        lcd_set_brightness_inner(lcd_brightness / 5);    // 不保存亮度值
        printf("[lcd-timeout]screen timeout\n");
    } else if(timeout_ms > SCREEN_TIMEOUT_MS + 5000) {
        sys_sleep();
        printf("[lcd-timeout]go to sleep\n");
    }
}

/**
 * 不许睡！
 */
void sys_set_dont_deep_sleep(bool b)
{
    dont_deep_sleep_enabled += (b ? 1 : -1);
    printf("[sys]dont_deep_sleep=%d\n", dont_deep_sleep_enabled);
    // 初始值为0，大于1即判定为不允许睡眠
    // 页面每设置一次，该变量+1；每取消一次，该变量-1
    // 这样就可以处理多个页面同时请求不睡眠的情况了
}

/**
 * 不许熄屏！
 */
void sys_set_dont_timeout(bool b)
{
    dont_timeout_enabled += (b ? 1 : -1);
    printf("[sys]dont_timeout=%d\n", dont_timeout_enabled);
}

/**
 * 切换到robot程序
 */
void switch_robot(void)
{
    switch_background();

    // 现在不需要杀vsftpd了
    system("chmod 777 ./switch_robot");
    system("sh ./switch_robot");
}

/**
 * 进入后台
 */
void switch_background(void)
{
    if(ts_background != -1) return;
    ts_background = tick_get();
    ts_sleep      = -1;
    if(fbd) close(fbd);
    if(dispd) close(dispd);
    if(powerd) close(powerd);
    usleep(100000);
}

/**
 * 从robot切换回来
 */
void switch_foreground(void)
{
    if(ts_background == -1) return;

    chdir(homepath);
    system("chmod 777 switch_foreground");
    system("sh ./switch_foreground &");
    // 等待自己被脚本杀死，然后开始新的轮回
    // 因为这里确实处理不好设备占用问题，只能把两个全杀了再重启自己
    sleep(114514);
}

/**
 * 获取字体
 */
lv_font_t * font_get(uint16_t weight, uint16_t font_style)
{
    lv_style_t style;
    lv_style_init(&style);

    lv_ft_info_t ft_info;
    ft_info.name   = "./res/font.ttf";
    ft_info.weight = weight;
    ft_info.style  = font_style;
    ft_info.mem    = NULL;

    if(lv_ft_font_init(&ft_info)) {
        return ft_info.font;
    }

    return NULL;
}
