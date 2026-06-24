/**
 * From DeepSeek
 */

#include "page_manager.h"

#include "main.h"

static PageManager page_manager;

/**
 * 一个创建页面的简单示例，也可以对简单页面直接使用
 * 需要自己分配一下内存，页面管理器会在销毁页面时自动释放
 */
BasePage * base_page_create(lv_obj_t * obj) 
{
    BasePage * page = malloc(sizeof(BasePage));
    if(!page) return NULL;
    memset(page, 0, sizeof(BasePage));

    page->obj = obj;
    return page;
}

// 初始化页面管理器
void page_manager_init(void)
{
    page_manager.top = -1; // 初始化为空栈
}

/**
 * 旧版本，用于兼容旧版本的页面
 * 也可以用于极简页面
 */
void page_open_obj(lv_obj_t * obj)
{
    page_open(base_page_create(obj));
}

// 创建新页面并压入堆栈
void page_open(BasePage * new_page)
{
    if(!new_page || !new_page->obj) {
        LV_LOG_ERROR("[pm]new page is null!\n");
    }

    if(page_manager.top >= MAX_PAGE_STACK - 1) {
        LV_LOG_ERROR("[pm]stack overflow!\n");
        return;
    }

    // 隐藏当前页面（如果有）
    if(page_manager.top >= 0) {
        BasePage * current_page = page_manager.stack[page_manager.top];
        lv_obj_add_flag(current_page->obj, LV_OBJ_FLAG_HIDDEN);

        // on_pause回调
        if(current_page->on_pause) (*current_page->on_pause)(current_page);
    }

    // 压入新页面
    page_manager.top++;
    page_manager.stack[page_manager.top] = new_page;

    // 设置新页面为当前显示
    lv_obj_clear_flag(new_page->obj, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(new_page->obj);

    // on_create回调
    if(new_page->on_create) (*new_page->on_create)(new_page);
}

// 返回上一页并销毁当前页
void page_back(void)
{
    if(page_manager.top <= 0) return;

    // 获取、隐藏当前页面
    BasePage * current_page = page_manager.stack[page_manager.top];
    lv_obj_add_flag(current_page->obj, LV_OBJ_FLAG_HIDDEN);

    // on_destroy回调
    if(current_page->on_destroy) (*current_page->on_destroy)(current_page);

    // 显示上一页面
    page_manager.top--;
    if(page_manager.top >= 0) {
        BasePage * new_page = page_manager.stack[page_manager.top];
        lv_obj_clear_flag(new_page->obj, LV_OBJ_FLAG_HIDDEN);

        // on_resume回调
        if(new_page->on_resume) (*new_page->on_resume)(new_page);
    }

    // 延迟删除当前页面
    lv_obj_del_async(current_page->obj);
    free(current_page);

    // 如果需要，可以在这里触发页面切换动画
}

bool page_on_key(key_code_t key_code, key_action_t key_action)
{
    // 获取当前页面
    BasePage * current_page = page_manager.stack[page_manager.top];
    
    if(current_page) {
        bool ret;
        if(current_page->on_key) 
            ret = (*current_page->on_key)(current_page, key_code, key_action);
        else
            ret = false;

        return ret;
    }
    return false;
}