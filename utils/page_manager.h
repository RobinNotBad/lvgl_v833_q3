#ifndef PROJ_PAGE_MANAGER_H
#define PROJ_PAGE_MANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "../lvgl/lvgl.h"
#include "../lv_lib_100ask/lv_lib_100ask.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include "hw_keys.h"

/*********************
 *      DEFINES
 *********************/
#define PAGE_MAX_STACK 32 // 最大页面堆栈深度
#define PAGE_ID_MAX_LEN 32  // 最大页面id长度

/**********************
 *      TYPEDEFS
 **********************/
typedef enum { PAGE_OK = 0, PAGE_ERR_NEW_NULL, PAGE_ERR_CURRENT_NULL, PAGE_ERR_STACK_EMPTY, PAGE_ERR_NO_EXISTING } page_ret_code_t;
typedef enum { PAGE_TYPE_DEFAULT = 0, PAGE_TYPE_DIALOG = 1 } page_type_t;

typedef struct
{
    lv_obj_t * obj;   // 页面对象
    void * user_data; // 用户数据
    char page_id[PAGE_ID_MAX_LEN];  // 页面id
    page_type_t page_type;          // 页面类型
    void (*on_create)(void *);  // 创建时自动触发
    void (*on_resume)(void *);  // 切换到前台时自动触发
    void (*on_pause)(void *);   // 切换到后台时自动触发
    void (*on_destroy)(void *); // 销毁时自动触发
    bool (*on_key)(void *, key_code_t, key_action_t); //接收到按键事件时自动触发

} BasePage;

typedef struct
{
    BasePage * stack[PAGE_MAX_STACK]; // 页面堆栈
    int top;                        // 栈顶序号
} PageManager;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * 创建一个基础页面
 */
BasePage * base_page_create(lv_obj_t * obj);

/**
 * 初始化，没什么好说的，初次使用时调用即可
 */
void page_manager_init(void);

/**
 * 启动一个纯obj页面，建议用于不需要回调的情况
 */
void page_open_obj(lv_obj_t * obj);

/**
 * 启动一个纯obj页面，同时为该页面设置一个id
 */
void page_open_obj_add_id(lv_obj_t * obj, char * id);

/**
 * 传入一个BasePage指针并显示它作为页面，之前的页面放入堆栈中（并进行on_pause回调）
 * 如何创建自己的页面类型？见base_page_create()
 * 或者obj那个用于简单页面也是可以的
 */
void page_open(BasePage * new_page);

/**
 * 将已有的一个页面置于顶部，其他页面前移填补空位
 */
page_ret_code_t page_open_existing(char * page_id);

/**
 * 将已有的一个页面关闭，其他页面前移填补空位
 */
page_ret_code_t page_close_existing(char * page_id);

/**
 * 销毁当前页面并返回上一页
 */
void page_back(void);

/**
 * 返回的回调函数，等价于page_back()
 * 因为返回按钮非常常用，所以统一放在这里
 */
void back_cb(lv_event_t * e);

/**
 * 接收按键事件并传给页面
 */
bool page_on_key(key_code_t key_code, key_action_t key_action);

/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
