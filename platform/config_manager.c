#include "config_manager.h"
#include "cJSON/json_path_tool.h"
#include "cJSON/cJSON.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

/* ---------- 内部辅助函数 ---------- */

/* 读取整个文件内容到字符串（调用者需 free） */
static char* read_file_content(const char* file_path) {
    FILE* fp = fopen(file_path, "rb");
    if (!fp) {
        return NULL;
    }
    fseek(fp, 0, SEEK_END);
    long len = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    char* buf = (char*)malloc(len + 1);
    if (!buf) {
        fclose(fp);
        return NULL;
    }
    size_t read_len = fread(buf, 1, len, fp);
    if (read_len != (size_t)len) {
        free(buf);
        fclose(fp);
        return NULL;
    }
    buf[len] = '\0';
    fclose(fp);
    return buf;
}

/* 将 cJSON 对象写入文件（若文件不存在则创建） */
static int write_json_to_file(const char* file_path, const cJSON* root) {
    char* json_str = cJSON_Print(root);
    if (!json_str) {
        return -1;
    }
    FILE* fp = fopen(file_path, "w");
    if (!fp) {
        free(json_str);
        return -1;
    }
    fprintf(fp, "%s", json_str);
    fclose(fp);
    free(json_str);
    return 0;
}

/* 读取 JSON 文件，返回 cJSON 根对象（若文件不存在或为空则创建一个对象） */
static cJSON* load_json_root(const char* file_path) {
    char* content = read_file_content(file_path);
    if (!content) {
        // 文件不存在或读失败，创建空对象
        return cJSON_CreateObject();
    }
    cJSON* root = cJSON_Parse(content);
    free(content);
    if (!root) {
        // 解析失败，创建空对象
        return cJSON_CreateObject();
    }
    // 若根不是对象或数组，则替换为对象（保留内容？简单起见丢弃并新建对象）
    if (!cJSON_IsObject(root) && !cJSON_IsArray(root)) {
        cJSON_Delete(root);
        root = cJSON_CreateObject();
    }
    return root;
}

/* 读取通用函数：读取指定类型的值 */
static int config_read_generic(const char* file_path, const char* json_path,
                               void* out_value, int expected_type) {
    if (!file_path || !json_path || !out_value) return -1;

    cJSON* root = load_json_root(file_path);
    if (!root) return -1;

    cJSON* node = cJSON_GetObjectPath(root, json_path);
    int ret = -1;
    if (node) {
        switch (expected_type) {
            case cJSON_Number: {
                if (cJSON_IsNumber(node)) {
                    double d = node->valuedouble;
                    *(double*)out_value = d; // 此处需要区分，但调用者传指针类型未知，我们用额外参数处理
                    ret = 0;
                }
                break;
            }
            case cJSON_String:
                if (cJSON_IsString(node)) {
                    char* str = strdup(node->valuestring);
                    if (str) {
                        *(char**)out_value = str;
                        ret = 0;
                    }
                }
                break;
            case cJSON_True:
            case cJSON_False:
                if (cJSON_IsBool(node)) {
                    *(bool*)out_value = cJSON_IsTrue(node);
                    ret = 0;
                }
                break;
            default:
                break;
        }
    }
    cJSON_Delete(root);
    return ret;
}

/* 写入通用函数：设置指定路径的值，并写回文件 */
static int config_write_generic(const char* file_path, const char* json_path,
                                cJSON* value) {
    if (!file_path || !json_path || !value) return -1;

    cJSON* root = load_json_root(file_path);
    if (!root) {
        cJSON_Delete(value);
        return -1;
    }

    int ret = cJSON_SetObjectPath(root, json_path, value);
    if (ret == 0) {
        ret = write_json_to_file(file_path, root);
    }
    cJSON_Delete(root);
    return ret;
}

/* ---------- 公开函数实现 ---------- */

int config_read_int(const char* file_path, const char* json_path, int def_value, int* out_value) {
    double d;
    int ret = config_read_generic(file_path, json_path, &d, cJSON_Number);
    if (ret == 0) {
        *out_value = (int)d; // 若实际为浮点则截断，但类型不匹配会返回-1
    }
    else {
        *out_value = def_value;
    }
    return ret;
}

int config_read_double(const char* file_path, const char* json_path, double def_value, double* out_value) {
    double d;
    int ret = config_read_generic(file_path, json_path, &d, cJSON_Number);
    if (ret == 0) {
        *out_value = d;
    }
    else *out_value = def_value;
    return ret;
}

int config_read_string(const char* file_path, const char* json_path, char* def_value, char** out_value) {
    char* str;
    int ret = config_read_generic(file_path, json_path, &str, cJSON_String);
    if (ret == 0) {
        *out_value = str;
    }
    else *out_value = def_value;
    return ret;
}

int config_read_bool(const char* file_path, const char* json_path, bool def_value, bool* out_value) {
    // 布尔类型使用 cJSON_True 或 cJSON_False
    int ret = config_read_generic(file_path, json_path, out_value, cJSON_True);
    if (ret != 0) {
        // 尝试读取为 False 类型
        ret = config_read_generic(file_path, json_path, out_value, cJSON_False);
        if(ret != 0){
            *out_value = def_value;
        }
    }
    return ret;
}

int config_write_int(const char* file_path, const char* json_path, int value) {
    printf("[config_manager] write int: %s=%d\n", json_path, value);
    cJSON* val = cJSON_CreateNumber(value);
    if (!val) return -1;
    return config_write_generic(file_path, json_path, val);
}

int config_write_double(const char* file_path, const char* json_path, double value) {
    printf("[config_manager] write double: %s=%.7f\n", json_path, value);
    cJSON* val = cJSON_CreateNumber(value);
    if (!val) return -1;
    return config_write_generic(file_path, json_path, val);
}

int config_write_string(const char* file_path, const char* json_path, const char* value) {
    printf("[config_manager] write string: %s=%s\n", json_path, value);
    cJSON* val = cJSON_CreateString(value);
    if (!val) return -1;
    return config_write_generic(file_path, json_path, val);
}

int config_write_bool(const char* file_path, const char* json_path, bool value) {
    printf("[config_manager] write int: %s=%d\n", json_path, value);
    cJSON* val = cJSON_CreateBool(value);
    if (!val) return -1;
    return config_write_generic(file_path, json_path, val);
}