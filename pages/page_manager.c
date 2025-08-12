/**
 * From DeepSeek
 */

#include "page_manager.h"

#define MAX_PAGE_STACK 32 // 最大页面堆栈深度

typedef struct
{
    lv_obj_t * page;  // 页面对象
    void * user_data; // 用户数据
} PageInfo;

typedef struct
{
    PageInfo stack[MAX_PAGE_STACK]; // 页面堆栈
    int top;                        // 栈顶指针
} PageManager;

static PageManager page_manager = {0};

// 初始化页面管理器
void page_manager_init(void)
{
    page_manager.top = -1; // 初始化为空栈
}

// 创建新页面并压入堆栈
void page_open(lv_obj_t * new_page, void * user_data)
{
    if(!new_page) {
        printf("[pm]new page is null!");
    }

    if(page_manager.top >= MAX_PAGE_STACK - 1) {
        printf("[pm]stack overflow!");
        return;
    }

    // 隐藏当前页面（如果有）
    if(page_manager.top >= 0) {
        lv_obj_add_flag(page_manager.stack[page_manager.top].page, LV_OBJ_FLAG_HIDDEN);
    }

    // 压入新页面
    page_manager.top++;
    page_manager.stack[page_manager.top].page      = new_page;
    page_manager.stack[page_manager.top].user_data = user_data;

    // 设置新页面为当前显示
    lv_obj_clear_flag(new_page, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(new_page);
}

// 返回上一页并销毁当前页
void page_back(void)
{
    if(page_manager.top < 0) return;

    // 1. 获取当前页面
    lv_obj_t * current_page = page_manager.stack[page_manager.top].page;

    // 2. 先隐藏当前页面
    lv_obj_add_flag(current_page, LV_OBJ_FLAG_HIDDEN);

    // 3. 显示上一页面
    page_manager.top--;
    if(page_manager.top >= 0) {
        lv_obj_clear_flag(page_manager.stack[page_manager.top].page, LV_OBJ_FLAG_HIDDEN);
    }

    // 4. 延迟删除当前页面
    lv_obj_del_async(current_page);

    // 5. 如果需要，可以在这里触发页面切换动画
}

// 获取当前页面的用户数据
void * page_get_current_user_data(void)
{
    if(page_manager.top < 0) {
        return NULL;
    }
    return page_manager.stack[page_manager.top].user_data;
}

// 获取当前页面对象
lv_obj_t * page_get_current(void)
{
    if(page_manager.top < 0) {
        return NULL;
    }
    return page_manager.stack[page_manager.top].page;
}