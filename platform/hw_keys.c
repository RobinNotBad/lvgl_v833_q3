
#include "hw_keys.h"

#include <fcntl.h>
#include <sys/ioctl.h>
#include "main.h"
#include "sys_robot.h"
#include "lv_drivers/indev/evdev.h"
#include "page_manager.h"


/**
 * 电源按钮
 */
int powerd;

void key_init_power(void)
{
    powerd = open(KEY_DEVICE_POWER, O_RDWR);
    fcntl(powerd, 4, 2048);
}

void key_close_power(void)
{
    close(powerd);
}

void key_clear_power(void)
{
    char buffer[16] = {0};
    while(read(powerd, buffer, sizeof(buffer)) > 0);
}

void key_read_power(void)
{
    char buffer[16] = {0};
    while(read(powerd, buffer, sizeof(buffer)) > 0) {
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
 * HOME按钮
 */

int homed;
static uint32_t ts_home_click = -1;

void key_init_home(void)
{
    homed = open(KEY_DEVICE_HOME, O_RDWR);
    fcntl(homed, 4, 2048);
}

void key_close_home(void)
{
    close(homed);
}

void key_clear_home(void)
{
    char buffer[16] = {0};
    while(read(homed, buffer, sizeof(buffer)) > 0);
}

void key_read_home(void)
{
    char buffer[16] = {0};
    while(read(homed, buffer, sizeof(buffer)) > 0) {
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