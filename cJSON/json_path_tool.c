/**
 * Created By DeepSeek
 */
#include "json_path_tool.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

/* 判断字符串是否表示非负整数（纯数字） */
static int is_number_string(const char* str) {
    if (!str || *str == '\0') return 0;
    while (*str) {
        if (!isdigit((unsigned char)*str)) return 0;
        str++;
    }
    return 1;
}

/* 按 '/' 分割路径，忽略空段，返回动态分配的字符串数组，*count 为段数 */
static char** split_path(const char* path, int* count) {
    if (!path) {
        *count = 0;
        return NULL;
    }
    // 跳过开头的 '/'
    while (*path == '/') path++;
    if (*path == '\0') {
        *count = 0;
        return NULL;
    }
    // 先统计段数
    int n = 1;
    const char* p = path;
    while (*p) {
        if (*p == '/') {
            // 连续 '/' 视为一个分隔符，跳过
            while (*(p+1) == '/') p++;
            n++;
        }
        p++;
    }
    char** segs = (char**)malloc(sizeof(char*) * n);
    if (!segs) {
        *count = 0;
        return NULL;
    }
    int idx = 0;
    const char* start = path;
    const char* end = path;
    while (1) {
        if (*end == '/' || *end == '\0') {
            // 提取段
            int len = end - start;
            if (len > 0) {
                segs[idx] = (char*)malloc(len + 1);
                if (!segs[idx]) {
                    for (int i = 0; i < idx; i++) free(segs[i]);
                    free(segs);
                    *count = 0;
                    return NULL;
                }
                strncpy(segs[idx], start, len);
                segs[idx][len] = '\0';
                idx++;
            }
            // 跳过连续 '/'
            while (*end == '/') end++;
            start = end;
            if (*end == '\0') break;
        } else {
            end++;
        }
    }
    *count = idx;
    return segs;
}

/* 释放分割得到的路径数组 */
static void free_segments(char** segs, int count) {
    if (segs) {
        for (int i = 0; i < count; i++) free(segs[i]);
        free(segs);
    }
}

/* ---------- 公开函数实现 ---------- */

cJSON* cJSON_GetObjectPath(const cJSON* root, const char* path) {
    if (!root || !path) return NULL;
    int seg_cnt;
    char** segs = split_path(path, &seg_cnt);
    if (seg_cnt == 0) {
        free_segments(segs, seg_cnt);
        return NULL;
    }
    const cJSON* cur = root;
    for (int i = 0; i < seg_cnt; i++) {
        if (!cur) {
            cur = NULL;
            break;
        }
        if (cur->type == cJSON_Object) {
            cur = cJSON_GetObjectItem(cur, segs[i]);
        } else if (cur->type == cJSON_Array) {
            if (!is_number_string(segs[i])) {
                cur = NULL;
                break;
            }
            int idx = atoi(segs[i]);
            cur = cJSON_GetArrayItem(cur, idx);
        } else {
            cur = NULL;
            break;
        }
    }
    free_segments(segs, seg_cnt);
    return (cJSON*)cur; // 丢弃 const，调用者不应修改只读结果
}

int cJSON_SetObjectPath(cJSON* root, const char* path, cJSON* value) {
    if (!root || !path || !value) return -1;
    if (root->type != cJSON_Object && root->type != cJSON_Array) return -1;

    int seg_cnt;
    char** segs = split_path(path, &seg_cnt);
    if (seg_cnt == 0) {
        free_segments(segs, seg_cnt);
        return -1; // 空路径不能设置根节点本身
    }

    cJSON* cur = root;
    int i;
    // 处理前 seg_cnt-1 段，确保路径存在（自动创建）
    for (i = 0; i < seg_cnt - 1; i++) {
        const char* seg = segs[i];
        const char* next_seg = segs[i+1]; // 用于判断应创建对象还是数组
        int next_is_num = is_number_string(next_seg);

        if (cur->type == cJSON_Object) {
            cJSON* child = cJSON_GetObjectItem(cur, seg);
            if (!child) {
                // 不存在，创建新节点
                child = next_is_num ? cJSON_CreateArray() : cJSON_CreateObject();
                if (!child) goto fail;
                cJSON_AddItemToObject(cur, seg, child);
            } else {
                // 存在，检查类型是否与下一步匹配
                if (next_is_num && child->type != cJSON_Array) goto fail;
                if (!next_is_num && child->type != cJSON_Object) goto fail;
            }
            cur = child;
        } else if (cur->type == cJSON_Array) {
            if (!is_number_string(seg)) goto fail;
            int idx = atoi(seg);
            // 扩展数组长度至至少 idx+1
            while (cJSON_GetArraySize(cur) <= idx) {
                cJSON_AddItemToArray(cur, cJSON_CreateNull());
            }
            cJSON* child = cJSON_GetArrayItem(cur, idx);
            if (!child || child->type == cJSON_NULL) {
                // 需要将 null 替换为合适的容器
                child = next_is_num ? cJSON_CreateArray() : cJSON_CreateObject();
                if (!child) goto fail;
                cJSON_ReplaceItemInArray(cur, idx, child);
            } else {
                // 检查类型
                if (next_is_num && child->type != cJSON_Array) goto fail;
                if (!next_is_num && child->type != cJSON_Object) goto fail;
            }
            cur = child;
        } else {
            goto fail; // 中间节点不是对象或数组，无法继续
        }
    }

    // 处理最后一段：设置值
    const char* last_seg = segs[seg_cnt-1];
    if (cur->type == cJSON_Object) {
        // 删除已有同名项（如果有），再添加新值
        cJSON_DeleteItemFromObject(cur, last_seg);
        cJSON_AddItemToObject(cur, last_seg, value);
    } else if (cur->type == cJSON_Array) {
        if (!is_number_string(last_seg)) goto fail;
        int idx = atoi(last_seg);
        // 确保数组足够长
        while (cJSON_GetArraySize(cur) <= idx) {
            cJSON_AddItemToArray(cur, cJSON_CreateNull());
        }
        cJSON_ReplaceItemInArray(cur, idx, value);
    } else {
        goto fail;
    }

    free_segments(segs, seg_cnt);
    return 0;

fail:
    free_segments(segs, seg_cnt);
    return -1;
}

int cJSON_DeleteObjectPath(cJSON* root, const char* path) {
    if (!root || !path) return -1;
    int seg_cnt;
    char** segs = split_path(path, &seg_cnt);
    if (seg_cnt == 0) {
        free_segments(segs, seg_cnt);
        return -1;
    }

    cJSON* cur = root;
    int i;
    // 定位到父节点（前 seg_cnt-1 段）
    for (i = 0; i < seg_cnt - 1; i++) {
        if (!cur) {
            free_segments(segs, seg_cnt);
            return -1;
        }
        if (cur->type == cJSON_Object) {
            cur = cJSON_GetObjectItem(cur, segs[i]);
        } else if (cur->type == cJSON_Array) {
            if (!is_number_string(segs[i])) {
                free_segments(segs, seg_cnt);
                return -1;
            }
            int idx = atoi(segs[i]);
            cur = cJSON_GetArrayItem(cur, idx);
        } else {
            free_segments(segs, seg_cnt);
            return -1;
        }
    }
    if (!cur) {
        free_segments(segs, seg_cnt);
        return -1;
    }

    const char* last_seg = segs[seg_cnt-1];
    if (cur->type == cJSON_Object) {
        cJSON_DeleteItemFromObject(cur, last_seg);
    } else if (cur->type == cJSON_Array) {
        if (!is_number_string(last_seg)) {
            free_segments(segs, seg_cnt);
            return -1;
        }
        int idx = atoi(last_seg);
        cJSON_DeleteItemFromArray(cur, idx);
    } else {
        free_segments(segs, seg_cnt);
        return -1;
    }

    free_segments(segs, seg_cnt);
    return 0;
}