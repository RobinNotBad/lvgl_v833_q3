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

#include "platform/hw_keys.h"
#include "platform/hw_screen.h"
#include "platform/sys_robot.h"
#include "platform/audio_ctrl.h"
#include "platform/battery_manager.h"
#include "platform/config_manager.h"
#include "platform/page_manager.h"
#include "platform/lv_utils.h"

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

char homepath[PATH_MAX_LENGTH];

uint32_t ts_sleep = -1;
uint32_t ts_background = -1;

bool is_screen_timeout = false;
uint8_t dont_deep_sleep_enabled = 0;
uint8_t dont_timeout_enabled = 0;

void lcd_detect_timeout(void);

lv_style_t * style_default;

int main(int argc, char * argv[])
{
    printf("ciallo lvgl\n");

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

    key_init_home();
    key_init_power();

    #if CPU_POWER_CTRL_ENABLED == 1
        system("echo interactive > /sys/devices/system/cpu/cpufreq/policy0/scaling_governor");
    #endif

    bool is_daemon_mode = true;

    for(uint32_t i = 0; i < argc; i++) {
        char * arg = argv[i];
        printf("argv[%d] = %s\n", i, arg);
        if(strcmp(arg, "-d") == 0) {
            is_daemon_mode = false;
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

    if(is_daemon_mode) daemon(1, 0);
    // daemon函数将本程序置于后台，脱离终端
    // 若要进行调试，请使用-d参数

    kill_robot();

    // 设置时区
    setenv("TZ", "CST-8", 1);
    tzset();

    lv_init();

    // 屏幕（含lcd和触摸）
    lcd_init();
    
    // ffmpeg
    lv_ffmpeg_init();

    // 音频
    audio_init();

    // 字体
    lv_freetype_init(128, 4, 0);

    lv_font_t * font = font_get(16, FT_FONT_STYLE_NORMAL);

    if(font) {
        lv_theme_t * theme =
            lv_theme_default_init(lv_disp_get_default(), lv_color_hex(THEME_COLOR), lv_color_hex(THEME_COLOR), true, font);
        theme->font_normal = font;
        theme->font_large  = font;
        theme->font_small  = font; // 为啥子设置不上？
        lv_disp_set_theme(lv_disp_get_default(), theme);

        style_default = malloc(sizeof(lv_style_t));
        lv_style_init(style_default);
        lv_style_set_text_font(style_default, font);
        lv_obj_add_style(lv_scr_act(), style_default, 0);
    }

    // 配置文件
    bool setup;
    if(config_read_bool(CFG_FILE_MAIN, CFG_SETUP, false, &setup) == -1 || !setup) {
        config_write_bool(CFG_FILE_MAIN, CFG_SETUP, true);
        
        config_write_string(CFG_FILE_MAIN, CFG_TIMIDITY_CFG, TIMIDITY_CFG_DEFAULT);
        config_write_bool(CFG_FILE_MAIN, CFG_REVERSE_X, TOUCH_REVERSE_X_DEFAULT);
        config_write_bool(CFG_FILE_MAIN, CFG_REVERSE_Y, TOUCH_REVERSE_Y_DEFAULT);
    }

    bool reverse_x, reverse_y;
    config_read_bool(CFG_FILE_MAIN, CFG_REVERSE_X, TOUCH_REVERSE_X_DEFAULT, &reverse_x);
    config_read_bool(CFG_FILE_MAIN, CFG_REVERSE_Y, TOUCH_REVERSE_Y_DEFAULT, &reverse_y);
    evdev_reverse(reverse_x, reverse_y);

    int volume;
    config_read_int(CFG_FILE_MAIN, CFG_VOLUME, 0, &volume);
    audio_volume_set(volume);
    config_read_int(CFG_FILE_MAIN, CFG_BRIGHTNESS, SCREEN_BRIGHTNESS_DEFAULT, &lcd_brightness);
    lcd_set_brightness_inner(lcd_brightness);
    

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

    if(style_default) free(style_default);
    lcd_close();
    key_close_home();
    key_close_power();
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
        #if CPU_POWER_CTRL_ENABLED == 1
            system("echo interactive > /sys/devices/system/cpu/cpufreq/policy0/scaling_governor");
        #endif
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
        #if CPU_POWER_CTRL_ENABLED == 1
            if(!dont_deep_sleep_enabled) system("echo powersave > /sys/devices/system/cpu/cpufreq/policy0/scaling_governor");
        #endif
    }
}

/**
 * 睡死
 */
void sys_deep_sleep(void)
{
    printf("[sys]deep sleep\n");
    key_clear_power();
    key_clear_home();

    // 睡死过去，相当省电
    system("echo \"0\" >/sys/class/rtc/rtc0/wakealarm");
    system("echo \"0\" >/sys/class/rtc/rtc0/wakealarm");
    system("echo \"mem\" > /sys/power/state");

    // 按电源键会醒过来，继续执行下面的代码

    sys_wake(); // 那睡觉的起来了嗷（改到这里是为了防止其他醒来的情况，比如插拔usb）
    key_clear_power();
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
