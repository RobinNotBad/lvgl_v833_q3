#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>


#define CFG_SETUP "/dendro/setup"
#define CFG_BRIGHTNESS "/system/brightness"
#define CFG_VOLUME "/system/volume"


/**
 * @brief 从 JSON 配置文件中读取整数值
 * @param file_path   JSON 文件路径
 * @param json_path   JSON 路径（如 "/server/port"）
 * @param out_value   输出整数值
 * @return 成功返回 0，失败返回 -1（文件不存在、路径无效或类型不匹配）
 */
int read_config_int(const char* file_path, const char* json_path, int def, int* out_value);

/**
 * @brief 从 JSON 配置文件中读取浮点值
 * @param file_path   JSON 文件路径
 * @param json_path   JSON 路径
 * @param out_value   输出浮点值
 * @return 成功返回 0，失败返回 -1
 */
int read_config_double(const char* file_path, const char* json_path, double def, double* out_value);

/**
 * @brief 从 JSON 配置文件中读取字符串值
 * @param file_path   JSON 文件路径
 * @param json_path   JSON 路径
 * @param out_value   输出字符串指针（调用者需使用 free() 释放）
 * @return 成功返回 0，失败返回 -1
 */
int read_config_string(const char* file_path, const char* json_path, char* def, char** out_value);

/**
 * @brief 从 JSON 配置文件中读取布尔值
 * @param file_path   JSON 文件路径
 * @param json_path   JSON 路径
 * @param out_value   输出布尔值（true/false）
 * @return 成功返回 0，失败返回 -1
 */
int read_config_bool(const char* file_path, const char* json_path, bool def, bool* out_value);

/**
 * @brief 写入整数值到 JSON 配置文件（自动创建中间路径）
 * @param file_path   JSON 文件路径
 * @param json_path   JSON 路径
 * @param value       整数值
 * @return 成功返回 0，失败返回 -1
 */
int write_config_int(const char* file_path, const char* json_path, int value);

/**
 * @brief 写入浮点值到 JSON 配置文件
 * @param file_path   JSON 文件路径
 * @param json_path   JSON 路径
 * @param value       浮点值
 * @return 成功返回 0，失败返回 -1
 */
int write_config_double(const char* file_path, const char* json_path, double value);

/**
 * @brief 写入字符串到 JSON 配置文件
 * @param file_path   JSON 文件路径
 * @param json_path   JSON 路径
 * @param value       字符串（函数内部会拷贝）
 * @return 成功返回 0，失败返回 -1
 */
int write_config_string(const char* file_path, const char* json_path, const char* value);

/**
 * @brief 写入布尔值到 JSON 配置文件
 * @param file_path   JSON 文件路径
 * @param json_path   JSON 路径
 * @param value       布尔值
 * @return 成功返回 0，失败返回 -1
 */
int write_config_bool(const char* file_path, const char* json_path, bool value);

#endif // CONFIG_MANAGER_H