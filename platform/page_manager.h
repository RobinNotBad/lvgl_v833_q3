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

/*********************
 *      DEFINES
 *********************/
#define MAX_PAGE_STACK 32 // 最大页面堆栈深度

typedef enum { KEY_CODE_POWER, KEY_CODE_HOME, KEY_CODE_SCANNER } key_code_t;
typedef enum { KEY_ACTION_DOWN, KEY_ACTION_UP } key_action_t;

/**********************
 *      TYPEDEFS
 **********************/

typedef struct
{
    lv_obj_t * obj;   // 页面对象
    void * user_data; // 用户数据
    void (*on_create)(void *);  // 创建时自动触发
    void (*on_resume)(void *);  // 切换到前台时自动触发
    void (*on_pause)(void *);   // 切换到后台时自动触发
    void (*on_destroy)(void *); // 销毁时自动触发
    bool (*on_key)(void *, key_code_t, key_action_t); //接收到按键事件时自动触发

} BasePage;

typedef struct
{
    BasePage * stack[MAX_PAGE_STACK]; // 页面堆栈
    int top;                        // 栈顶指针
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
 * 用于不需要回调的页面，直接传入一个obj即可
 */
void page_open_obj(lv_obj_t * obj);

/**
 * 传入一个BasePage指针并显示它作为页面，之前的页面放入堆栈中（并进行on_pause回调）
 * 如何创建自己的页面类型？见base_page_create()
 * 或者obj那个用于简单页面也是可以的
 */
void page_open(BasePage * new_page);

/**
 * 销毁当前页面并返回上一页
 */
void page_back(void);

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
