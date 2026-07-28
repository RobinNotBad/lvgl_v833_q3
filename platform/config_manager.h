#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>


#define CFG_FILE_MAIN "/mnt/UDISK/lvgl/config.json"

#define CFG_SETUP "/dendro/setup"
#define CFG_TIMIDITY_CFG "/midi/timidity_cfg"
#define CFG_REVERSE_X "/evdev/reverse_x"
#define CFG_REVERSE_Y "/evdev/reverse_y"
#define CFG_BRIGHTNESS "/system/brightness"
#define CFG_VOLUME "/system/volume"

/**
 * @brief 从 JSON 配置文件中读取整数值
 * @param file_path   JSON 文件路径
 * @param json_path   JSON 内部路径（如 "/server/port"）
 * @param def_value   默认值
 * @param out_value   整数变量的指针
 * @return 成功返回 0，失败返回 -1
 */
int config_read_int(const char* file_path, const char* json_path, int def_value, int* out_value);

/**
 * @brief 从 JSON 配置文件中读取浮点值
 * @param file_path   JSON 文件路径
 * @param json_path   JSON 内部路径
 * @param def_value   默认值
 * @param out_value   浮点变量的指针
 * @return 成功返回 0，失败返回 -1
 */
int config_read_double(const char* file_path, const char* json_path, double def_value, double* out_value);

/**
 * @brief 从 JSON 配置文件中读取字符串值
 * @param file_path   JSON 文件路径
 * @param json_path   JSON 内部路径
 * @param def_value   默认值
 * @param out_value   字符串的指针（请传入一个空指针，后续需使用 free() 释放）
 * @return 成功返回 0，失败返回 -1
 */
int config_read_string(const char* file_path, const char* json_path, char* def_value, char** out_value);

/**
 * @brief 从 JSON 配置文件中读取布尔值
 * @param file_path   JSON 文件路径
 * @param json_path   JSON 内部路径
 * @param def_value   默认值
 * @param out_value   布尔变量的指针
 * @return 成功返回 0，失败返回 -1
 */
int config_read_bool(const char* file_path, const char* json_path, bool def_value, bool* out_value);

/**
 * @brief 写入整数值到 JSON 配置文件（自动创建中间路径）
 * @param file_path   JSON 文件路径
 * @param json_path   JSON 内部路径
 * @param value       整数值
 * @return 成功返回 0，失败返回 -1
 */
int config_write_int(const char* file_path, const char* json_path, int value);

/**
 * @brief 写入浮点值到 JSON 配置文件
 * @param file_path   JSON 文件路径
 * @param json_path   JSON 内部路径
 * @param value       浮点值
 * @return 成功返回 0，失败返回 -1
 */
int config_write_double(const char* file_path, const char* json_path, double value);

/**
 * @brief 写入字符串到 JSON 配置文件
 * @param file_path   JSON 文件路径
 * @param json_path   JSON 内部路径
 * @param value       字符串（函数内部会拷贝）
 * @return 成功返回 0，失败返回 -1
 */
int config_write_string(const char* file_path, const char* json_path, const char* value);

/**
 * @brief 写入布尔值到 JSON 配置文件
 * @param file_path   JSON 文件路径
 * @param json_path   JSON 内部路径
 * @param value       布尔值
 * @return 成功返回 0，失败返回 -1
 */
int config_write_bool(const char* file_path, const char* json_path, bool value);

#endif // CONFIG_MANAGER_H