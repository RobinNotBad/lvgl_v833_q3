#ifndef HW_KEYS_H
#define HW_KEYS_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>

/*********************
 *      DEFINES
 *********************/

#define KEY_DEVICE_POWER "/dev/input/event1"
#define KEY_DEVICE_HOME "/dev/input/event2"

typedef enum { KEY_CODE_POWER, KEY_CODE_HOME, KEY_CODE_SCANNER } key_code_t;
typedef enum { KEY_ACTION_DOWN, KEY_ACTION_UP } key_action_t;

extern int powerd; // 电源按钮
extern int homed;  // 主页按钮

/**********************
 *      TYPEDEFS
 **********************/

/**
 * 初始化电源键
 */
void key_init_power(void);

/**
 * 释放电源键的文件描述符
 */
void key_close_power(void);

/**
 * 清除电源键的缓冲区
 */
void key_clear_power(void);

/**
 * 读取电源键并执行相关事件
 */
void key_read_power(void);


/**
 * 初始化home键
 */
void key_init_home(void);

/**
 * 释放home键的文件描述符
 */
void key_close_home(void);

/**
 * 清除home键的缓冲区
 */
void key_clear_home(void);

/**
 * 读取home键并执行相关事件
 */
void key_read_home(void);


/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
