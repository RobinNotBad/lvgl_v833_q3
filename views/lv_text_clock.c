/**
 * @file lv_text_clock.c
 * 文字时钟控件实现
 */

#include "lv_text_clock.h"

static void lv_text_clock_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj);
static void lv_text_clock_destructor(const lv_obj_class_t * class_p, lv_obj_t * obj);
static void lv_text_clock_timer_cb(lv_timer_t * timer);
static void lv_text_clock_update_time(lv_text_clock_t * clock);

static const lv_obj_class_t lv_text_clock_class = {
    .constructor_cb = lv_text_clock_constructor,
    .destructor_cb = lv_text_clock_destructor,
    .instance_size = sizeof(lv_text_clock_t),
    .base_class = &lv_label_class
};

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

lv_obj_t * lv_text_clock_create(lv_obj_t * parent)
{
    LV_LOG_INFO("Start to create text clock\n");

    /* 创建对象 */
    lv_obj_t * obj = lv_obj_class_create_obj(&lv_text_clock_class, parent);
    lv_obj_class_init_obj(obj);
    
    return obj;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

/**
 * 构造函数
 */
static void lv_text_clock_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj)
{
    lv_text_clock_t * clock = (lv_text_clock_t *)obj;
    
    /* 初始化时钟结构 */
    clock->timer = NULL;
    memset(clock->time_str, 0, sizeof(clock->time_str));

    /* 设置初始文本 */
    lv_label_set_text(&clock->label, "00:00:00");
    
    // 立即更新时间 
    lv_text_clock_update_time(clock);
    
    // 创建更新定时器（每秒更新一次
    clock->timer = lv_timer_create(lv_text_clock_timer_cb, 250, obj);
    if(clock->timer == NULL) {
        LV_LOG_ERROR("Failed to create timer for text clock\n");
        return;
    }
    
    
    LV_LOG_INFO("Text clock created\n");
    
    
}

/**
 * 析构函数
 */
static void lv_text_clock_destructor(const lv_obj_class_t * class_p, lv_obj_t * obj)
{
    lv_text_clock_t * clock = (lv_text_clock_t *)obj;
    
    /* 删除定时器 */
    if(clock->timer) {
        lv_timer_del(clock->timer);
        clock->timer = NULL;
    }
    
    LV_LOG_INFO("Text clock deleted\n");
}

/**
 * 定时器回调函数
 */
static void lv_text_clock_timer_cb(lv_timer_t * timer)
{
    lv_text_clock_t * clock = (lv_text_clock_t *)timer->user_data;
    
    /* 更新当前时间 */
    lv_text_clock_update_time(clock);
}

/**
 * 更新时间显示
 */
static void lv_text_clock_update_time(lv_text_clock_t * clock)
{
    /* 获取当前时间 */
    time_t rawtime;
    struct tm * timeinfo;
    
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    
    if(timeinfo == NULL) {
        lv_label_set_text(&clock->label, "Error");
        return;
    }
    
    /* 格式化为"HH:MM:SS"格式 */
    snprintf(clock->time_str, sizeof(clock->time_str), 
             "%02d:%02d:%02d",
             timeinfo->tm_hour,
             timeinfo->tm_min,
             timeinfo->tm_sec);
    
    /* 更新标签文本 */
    lv_label_set_text(&clock->label, clock->time_str);
}
