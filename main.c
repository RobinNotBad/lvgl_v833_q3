#include "lvgl/lvgl.h"
#include "lvgl/demos/lv_demos.h"
#include "lv_drivers/display/fbdev.h"
#include "lv_drivers/indev/evdev.h"
#include "lv_lib_100ask/lv_lib_100ask.h"
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <string.h>
#include "platform/audio_alsa.h"

//请教DeepSeek实现了简易页面管理器，100ask那个实际上不太好用……
#include "pages/page_manager.h"
#include "pages/page_main.h"
#include "pages/page_demo.h"
#include "pages/page_file_manager.h"
#include "pages/page_calculator.h"
#include "pages/page_apple.h"

#define DISP_BUF_SIZE (240 * 240)

#define PATH_MAX_LENGTH 256
extern char homepath[PATH_MAX_LENGTH] = {0};

extern int dispd  = 0;     // 背光
extern int fbd    = 0;     // 帧缓冲设备
extern int powerd = 0;     //电源按钮
extern int homed  = 0;     // 主页按钮

extern uint32_t sleepTs     = -1;
extern uint32_t homeClickTs = -1;
extern uint32_t backgroundTs = -1;

extern bool deepsleep  = false;

extern void lcdBrightness(int brightness);
extern void sysSleep(void);
extern void sysWake(void);
extern void sysDeepSleep(void);
extern void switchRobot(void);
extern void switchBackground(void);
extern void switchForeground(void);

uint32_t tick_get(void);

int main(int argc, char *argv[])
{
	printf("kill robot\n");
	system("killall robotd");
	system("killall robot_run");
	usleep(100000);
	
	printf("ciallo lvgl\n");
	#if LV_USE_PERF_MONITOR
	printf("monitor on\n");
	#endif

    bool isDaemonMode = true;

    for (uint32_t i = 0; i < argc; i++)
    {
        char * arg = argv[i];
        printf("argv[%d] = %s\n", i, arg);
        if(strcmp(arg, "-d") == 0) {
            isDaemonMode = false;
        }
    }

    getcwd(homepath, PATH_MAX_LENGTH);

    if(isDaemonMode) daemon(1,0);
	//daemon函数将本程序置于后台，脱离终端
	//若要进行调试，请使用-d参数
    
    fbd = open("/dev/fb0", O_RDWR);
    dispd = open("/dev/disp", O_RDWR);
    lcdOpen();
    touchOpen();
    lcdBrightness(25);

    lv_init();
    fbdev_init();
    
    static lv_color_t buf[DISP_BUF_SIZE];
    
    static lv_disp_draw_buf_t disp_buf;
    lv_disp_draw_buf_init(&disp_buf, buf, NULL, DISP_BUF_SIZE);
    
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.draw_buf   = &disp_buf;
    disp_drv.flush_cb   = fbdev_flush;
    disp_drv.hor_res    = 240;
    disp_drv.ver_res    = 240;
    lv_disp_drv_register(&disp_drv);

    evdev_init();
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = evdev_read;
    lv_indev_t *indev = lv_indev_drv_register(&indev_drv);
    
    powerd = open("/dev/input/event1", O_RDWR);
    fcntl(powerd, 4,2048);
    homed = open("/dev/input/event2", O_RDWR);
    fcntl(homed, 4,2048);

	lv_ffmpeg_init();

    page_manager_init();
    page_open(page_main(), NULL);

    while(1) {
        readKeyHome();
        if(backgroundTs == -1){
            readKeyPower();
         	if(sleepTs == -1) {
            	lv_timer_handler();
        	    lcdRefresh();
	            usleep(5000);
            }
            else {
        	    if (!deepsleep && tick_get() - sleepTs >= 60000) {
        		    sysDeepSleep();
        	    }
            	usleep(25000);
            }
        }
        else {
            if(tick_get() - backgroundTs >= 300000) {
                switchForeground();
            }
            usleep(25000);
        }
    }

    close(fbd);
    close(dispd);
    close(powerd);
    close(homed);
    return 0;
}

/*Set in lv_conf.h as `LV_TICK_CUSTOM_SYS_TIME_EXPR`*/
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

void lcdOpen(void) {
    int buffer[8] = {0};
    buffer[1] = 1;
    ioctl(dispd, 0xFu, buffer);
    printf("[lcd]opened\n");
}

void lcdClose(void) {
    char buffer[24] = {0};
    ioctl(dispd, 0xFu, buffer);
    printf("[lcd]closed\n");
}

void touchOpen(void) {
	int tpd = open("/proc/sprocomm_tpInfo", 526338);
    write(tpd, "1", 1u);
    close(tpd);
    printf("[tp]opened\n");
}

void touchClose(void) {
    int tpd = open("/proc/sprocomm_tpInfo", 526338);
    write(tpd, "0", 1u);
    close(tpd);
    printf("[tp]closed\n");
}

void lcdRefresh(void) {
    int buffer[8] = {0};
	ioctl(fbd, 0x4606u, buffer);
}

void lcdBrightness(int brightness) {
	int buffer[8] = {0};
    buffer[1] = brightness;
	ioctl(dispd, 0x102u, buffer);
}

void readKeyPower(void) {
    char buffer[16] = {0};
    while (read(powerd, buffer, 0x10u) > 0) {
		if(buffer[10] != 0x74) return;

		if(buffer[12] == 0x00) {
			//printf("[key]power_up\n");
			if(sleepTs == -1) {
				sysSleep();
			}
			else {
				sysWake();
			}
		}
		else {
			//printf("[key]power_down\n");
		}
	}
}

void readKeyHome(void) {
	char buffer[16] = {0};
	while (read(homed, buffer, 0x10u) > 0) {
		if(buffer[10] != 0x73) return;

		if(buffer[12] == 0x00) {
			//printf("[key]home_up\n");
            uint32_t ts = tick_get();
            if(homeClickTs != -1 && ts - homeClickTs <= 300){
                switchForeground();
                homeClickTs = -1;
            } else {
                homeClickTs = ts;
            }
        } else {
            //printf("[key]home_down\n");
        }
    }
}

void sysWake(void){
	deepsleep = false;
	sleepTs = -1;
	touchOpen();
    lcdOpen();
}

void sysSleep(void){
	deepsleep = false;
	sleepTs = tick_get();
	touchClose();
	lcdClose();
}

void sysDeepSleep(void){
	deepsleep = true;
    sleepTs   = -1;
    // 这是真睡死过去，相当省电，按电源键还能回来
    system("echo \"0\" >/sys/class/rtc/rtc0/wakealarm");
    system("echo \"mem\" > /sys/power/state");
}

void switchRobot(){
    switchBackground();

    chdir("/mnt/app");
    system("./robot_run &");
}

void switchBackground(void){
    if(backgroundTs != -1) return;
    backgroundTs = tick_get();
    sleepTs    = -1;
}

void switchForeground(void)
{
    if(backgroundTs == -1) return;

    chdir(homepath);
    system("sh ./switch_foreground &");
    sleep(114514);
}
