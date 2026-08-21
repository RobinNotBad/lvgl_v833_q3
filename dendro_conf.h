#ifndef DENDRO_CONF_H
#define DENDRO_CONF_H

/**
 * 这里是Dendro的配置文件
 * 屏幕、触摸驱动配置请移步lv_drv_conf.h
 * LVGL配置请移步lv_shared_linux/lv_conf.h，修改编译完成后别忘记同步到这里的lv_conf.h
 * （Dendro为上层应用pages提供包装好的底层api，算是一个中间层？）
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * 默认反转EVDEV的XY轴（也可以修改配置文件）
 */
#define TOUCH_REVERSE_X_DEFAULT 0
#define TOUCH_REVERSE_Y_DEFAULT 0

/**
 * 允许屏幕超时熄灭
 */
#define SCREEN_TIMEOUT_ENABLED 1

/**
 * 允许熄屏后睡眠
 * 部分设备不支持，会导致崩溃
 */
#define DEEP_SLEEP_ENABLED 1

/**
 * 屏幕超时时间
 * 超过该时间，屏幕变暗，然后自动熄灭
 */
#define SCREEN_TIMEOUT_MS 30000

/**
 * 允许熄屏时使用scaling_governor调控cpu频率
 * 部分设备不支持，会导致崩溃
 */
#define CPU_POWER_CTRL_ENABLED 1

/**
 * 主题颜色（按钮等）
 */
#define THEME_COLOR 0xff78c05d

/**
 * MIDI配置文件
 */
#define TIMIDITY_CFG_DEFAULT "/mnt/UDISK/dendro/midi/timidity.cfg"

/**
 * 路径最大长度
 */
#define PATH_MAX_LENGTH 1024



#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
