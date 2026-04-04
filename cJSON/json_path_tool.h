/**
 * Created By DeepSeek
 */
#ifndef JSON_PATH_TOOL_H
#define JSON_PATH_TOOL_H

#include "cJSON.h"

/**
 * 根据路径获取 JSON 节点（只读，不会自动创建缺失节点）
 * @param root  根节点（通常是 cJSON_Object 或 cJSON_Array）
 * @param path  路径字符串，如 "/data/person/0/name"
 * @return      找到的 cJSON*，失败返回 NULL
 */
cJSON* cJSON_GetObjectPath(const cJSON* root, const char* path);

/**
 * 根据路径设置 JSON 节点（自动创建缺失的中间节点）
 * @param root  根节点（必须是 cJSON_Object 或 cJSON_Array）
 * @param path  路径字符串，如 "/data/person/0/name"
 * @param value 要设置的值（注意：函数将接管 value 的所有权，调用者不应再手动释放）
 * @return      成功返回 0，失败返回 -1
 */
int cJSON_SetObjectPath(cJSON* root, const char* path, cJSON* value);

/**
 * 根据路径删除 JSON 节点（父节点必须存在且类型正确）
 * @param root  根节点
 * @param path  路径字符串
 * @return      成功返回 0，失败返回 -1
 */
int cJSON_DeleteObjectPath(cJSON* root, const char* path);

#endif // JSON_PATH_TOOL_H