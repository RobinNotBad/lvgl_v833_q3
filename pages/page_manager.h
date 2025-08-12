#ifndef PROJ_PAGE_MAIN_H
#define PROJ_PAGE_MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "../lvgl/lvgl.h"
#include "../lv_lib_100ask/lv_lib_100ask.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * 初始化，没什么好说的，初次使用时调用即可
 */
void page_manager_init(void);

/**
 * 传入一个lvgl控件并显示它作为页面，之前的页面放入堆栈中（其中的内容保留且继续运行）
 * data是任意类型的指针，可以传入你想传入的任何东西
 */
void page_open(lv_obj_t * new_page, void * user_data);

/**
 * 销毁当前页面并返回上一页
 */
void page_back(void);

/**
 * 获得当前页面传入的数据
 */
void * page_get_current_user_data(void);

/**
 * 获得当前的页面
 */
lv_obj_t * page_get_current(void);


/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
