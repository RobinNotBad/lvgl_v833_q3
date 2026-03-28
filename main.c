#include "main.h"

#include "lvgl/demos/lv_demos.h"
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

// 请教DeepSeek实现了简易页面管理器，100ask那个实际上不太好用……
#include "pages/page_manager.h"
#include "pages/page_main.h"

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

uint32_t sleepTs;
uint32_t homeClickTs;
uint32_t backgroundTs;
uint32_t lcdBrightness;

bool screenTimeout;
bool dontDeepSleep;

void readKeyPower(void);
void readKeyHome(void);
void lcdInit(void);
void lcdOpen(void);
void lcdClose(void);
void lcdRefresh(void);
void touchOpen(void);
void touchClose(void);
void detectTimeout(void);

static lv_style_t style_default;

int main(int argc, char * argv[])
{
    // 初始化变量
    sleepTs       = -1;
    homeClickTs   = -1;
    backgroundTs  = -1;
    lcdBrightness = SCREEN_BRIGHTNESS_DEFAULT;
    screenTimeout = false;
    dontDeepSleep = false;

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
            switchBackground();
            while(1) {
                usleep(25000);
                readKeyHome();
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
    lcdInit();
    lcdClose();
    lcdOpen();
    lcdSetBrightness(SCREEN_BRIGHTNESS_DEFAULT);
    touchOpen();

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

    lv_obj_t * screen = lv_obj_create(NULL);
    lv_scr_load(screen);

    lv_freetype_init(128, 4, 0);

    lv_ft_info_t ft_info;
    ft_info.name   = "./res/font.ttf";
    ft_info.weight = 16;
    ft_info.style  = FT_FONT_STYLE_NORMAL;
    ft_info.mem    = NULL;

    if(lv_ft_font_init(&ft_info)) {
        lv_theme_t * theme = lv_theme_default_init(disp, lv_palette_main(LV_PALETTE_LIGHT_GREEN),
                                                   lv_palette_main(LV_PALETTE_GREEN), true, ft_info.font);
        theme->font_normal = ft_info.font;
        theme->font_large  = ft_info.font;
        theme->font_small  = ft_info.font; // 为啥子设置不上？
        lv_disp_set_theme(disp, theme);

        lv_style_init(&style_default);
        lv_style_set_text_font(&style_default, ft_info.font);
        lv_obj_add_style(lv_scr_act(), &style_default, 0);
    }

    page_manager_init();
    page_open_obj(page_main());

    while(1) {
        readKeyHome();
        if(backgroundTs == -1) {
            readKeyPower();
            if(sleepTs == -1) {
                // 亮
                lv_timer_handler();
                lcdRefresh(); // 放在fbdev里不合适，反而会增大cpu占用且变卡，神金啊
                detectTimeout();
                usleep(5000);

            } else {
                // 灭
                if(dontDeepSleep)
                    sleepTs = tick_get();

                else if(tick_get() - sleepTs >= 60000)
                    sysDeepSleep();

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
void lcdInit(void)
{
    vinfo         = fbdev_get_vinfo();
    vinfo->rotate = 3;
    ioctl(fbd, 0x4601u, vinfo);
}

/**
 * 点亮LCD
 */
void lcdOpen(void)
{
    int buffer[8] = {0};
    buffer[1]     = 1;
    ioctl(dispd, 0xFu, buffer);
    printf("[lcd]opened\n");
}

/**
 * 熄灭LCD
 */
void lcdClose(void)
{
    int buffer[8] = {0};
    ioctl(dispd, 0xFu, buffer);
    printf("[lcd]closed\n");
}

/**
 * 启用触摸
 */
void touchOpen(void)
{
    int tpd = open("/proc/sprocomm_tpInfo", 526338);
    write(tpd, "1", 1u);
    close(tpd);
    printf("[tp]opened\n");
}

/**
 * 关闭触摸
 */
void touchClose(void)
{
    int tpd = open("/proc/sprocomm_tpInfo", 526338);
    write(tpd, "0", 1u);
    close(tpd);
    printf("[tp]closed\n");
}

/**
 * LCD刷屏
 */
void lcdRefresh(void)
{
    ioctl(fbd, 0x4606u, vinfo);
}

/**
 * 设置LCD背光亮度
 */
void lcdSetBrightness(int brightness)
{
    int buffer[8] = {0};
    buffer[1]     = brightness;
    ioctl(dispd, 0x102u, buffer);
}

/**
 * 读取电源按钮
 */
void readKeyPower(void)
{
    char buffer[16] = {0};
    while(read(powerd, buffer, 0x10u) > 0) {
        if(buffer[10] != 0x74) return;

        if(buffer[12] == 0x00) {
            printf("[key]power_up\n");
            if(sleepTs == -1)
                if(page_on_key(KEY_CODE_POWER, KEY_ACTION_UP)) continue;
            // 如果页面处理了按键事件，就不继续执行了

            if(sleepTs == -1)
                sysSleep(); // 没睡的给我睡
            else
                sysWake(); // 睡着的起来

        } else if(buffer[12] == 0x01) {
            printf("[key]power_down\n");

            if(sleepTs == -1)
                if(page_on_key(KEY_CODE_POWER, KEY_ACTION_DOWN)) continue;
        }
    }
}

/**
 * 读取圆形HOME按钮
 */
void readKeyHome(void)
{
    char buffer[16] = {0};
    while(read(homed, buffer, 0x10u) > 0) {
        if(buffer[10] != 0x73) return;

        if(buffer[12] == 0x00) {
            printf("[key]home_up\n");
            if(sleepTs == -1)
                if(page_on_key(KEY_CODE_HOME, KEY_ACTION_UP)) continue;
            // 如果页面处理了按键事件，就不继续执行了

            uint32_t ts = tick_get();
            if(homeClickTs != -1 && ts - homeClickTs <= 300) {
                switchForeground();
                homeClickTs = -1;
            } else {
                homeClickTs = ts;
                if(sleepTs == -1)
                    page_back(); // 没睡的返回
                else
                    sysWake(); // 睡着的起来
            }
        } else if(buffer[12] == 0x01) {
            printf("[key]home_down\n");
            if(sleepTs == -1)
                if(page_on_key(KEY_CODE_HOME, KEY_ACTION_DOWN)) continue;
        }
    }
}

/**
 * 亮屏
 */
void sysWake(void)
{
    if(sleepTs != -1) {
        screenTimeout = false;
        sleepTs       = -1;
        touchOpen();
        lcdOpen();
        lcdSetBrightness(lcdBrightness);
    }
}

/**
 * 熄屏
 */
void sysSleep(void)
{
    if(sleepTs == -1) {
        sleepTs = tick_get();
        touchClose();
        lcdClose();
    }
}

/**
 * 睡死
 */
void sysDeepSleep(void)
{
    char buffer[16] = {0};
    while(read(powerd, buffer, 0x10u) > 0); // 清空电源键的缓冲区
    while(read(homed, buffer, 0x10u) > 0);  // 清空HOME键的缓冲区

    // 睡死过去，相当省电
    system("echo \"0\" >/sys/class/rtc/rtc0/wakealarm");
    system("echo \"0\" >/sys/class/rtc/rtc0/wakealarm");
    system("echo \"mem\" > /sys/power/state");

    // 按电源键会醒过来，继续执行下面的代码

    sysWake(); // 那睡觉的起来了嗷（改到这里是为了防止其他醒来的情况，比如插拔usb）
    while(read(powerd, buffer, 0x10u) > 0); // 再次清空电源键的缓冲区，因为开机按的电源键也算数
}

/**
 * 检测是否超时熄屏
 */
void detectTimeout(void)
{
    uint64_t timeout_ms = ms_get() - evdev_get_release_ts();
    if(timeout_ms < SCREEN_TIMEOUT_MS){
        if(screenTimeout) {
            screenTimeout = false;
            lcdSetBrightness(lcdBrightness);
        }
    }
    else if(!screenTimeout) {
        screenTimeout = true;
        lcdSetBrightness(5);
    } else if(timeout_ms > SCREEN_TIMEOUT_MS + 4500) {
        sysSleep();
    }
}

/**
 * 不许睡！
 */
void setDontDeepSleep(bool b)
{
    dontDeepSleep = b;
}

/**
 * 切换到robot程序
 */
void switchRobot(void)
{
    switchBackground();

    // 现在不需要杀vsftpd了
    system("chmod 777 ./switch_robot");
    system("sh ./switch_robot");
}

/**
 * 进入后台
 */
void switchBackground(void)
{
    if(backgroundTs != -1) return;
    backgroundTs = tick_get();
    sleepTs      = -1;
    if(fbd) close(fbd);
    if(dispd) close(dispd);
    if(powerd) close(powerd);
    usleep(100000);
}

/**
 * 从robot切换回来
 */
void switchForeground(void)
{
    if(backgroundTs == -1) return;

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
lv_style_t getFontStyle(const char * filename, uint16_t weight, uint16_t font_style)
{
    lv_style_t style;
    lv_style_init(&style);

    lv_ft_info_t ft_info;
    ft_info.name   = filename;
    ft_info.weight = weight;
    ft_info.style  = font_style;
    ft_info.mem    = NULL;

    if(lv_ft_font_init(&ft_info)) {
        lv_style_set_text_font(&style, ft_info.font);
    }

    return style;
}
