/**
 * From DeepSeek
 * 一个页面管理器
 */

#include "page_manager.h"

#include <string.h>
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

/**
 * 旧版本，用于兼容旧版本的页面
 * 也可以用于极简页面
 */
void page_open_obj_add_id(lv_obj_t * obj, char * page_id)
{
    BasePage * p = base_page_create(obj);
    strcpy(p->page_id, page_id);
    page_open(p);
}

// 创建新页面并压入堆栈
void page_open(BasePage * new_page)
{
    if(!new_page || !new_page->obj) {
        printf("[page_manager] new page is null!\n");
    }

    if(page_manager.top >= PAGE_MAX_STACK - 1) {
        printf("[page_manager] stack overflow!\n");
        lv_obj_del_async(new_page->obj);
        free(new_page);
        return;
    }

    // 隐藏当前页面（如果有）
    if(page_manager.top >= 0) {
        // 普通页面要隐藏，但对话框不要
        BasePage * current_page = page_manager.stack[page_manager.top];
        if(new_page->page_type == PAGE_TYPE_DEFAULT) {
            lv_obj_add_flag(current_page->obj, LV_OBJ_FLAG_HIDDEN);
        }
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
    
    printf("[page_manager] top=%d\n", page_manager.top);
}

// 将已有的一个页面置于顶部，其他页面前移填补空位
page_ret_code_t page_open_existing(char * page_id)
{    
    printf("[page_manager] page_id=%s\n", page_id);
    // 没有页面你移动个啥
    if(!page_id) return PAGE_ERR_NEW_NULL;
    if(page_manager.top < 0) return PAGE_ERR_STACK_EMPTY;
    
    BasePage * current_page = page_manager.stack[page_manager.top];
    if(!current_page) return PAGE_ERR_CURRENT_NULL;

    // 如果就是当前页，不做操作也不进行回调
    if(strcmp(current_page->page_id, page_id) == 0) return PAGE_OK;

    // 遍历列表，查找那个页面的位置（当前位置已经检查过，减一跳过）
    int index;
    BasePage * existing_page;
    for (index = page_manager.top - 1; index >= 0; index--)
    {
        existing_page = page_manager.stack[index];
        if(!existing_page) continue;
        if(strcmp(existing_page->page_id, page_id) == 0) break;
    }
    printf("[page_manager] index=%d\n", index);
    if(index < 0) return PAGE_ERR_NO_EXISTING;

    // 隐藏当前页面
    if(current_page->page_type == PAGE_TYPE_DEFAULT) {
        lv_obj_add_flag(current_page->obj, LV_OBJ_FLAG_HIDDEN);
    }
    // 当前页面on_pause回调
    if(current_page->on_pause) (*current_page->on_pause)(current_page);
    
    // 将这个位置后的所有页面都前移一位
    for (int i = index; i < page_manager.top; i++)
    {
        page_manager.stack[i] = page_manager.stack[i+1];
    }
    
    page_manager.stack[page_manager.top] = existing_page;

    // 设置新页面为当前显示
    lv_obj_clear_flag(existing_page->obj, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(existing_page->obj);

    // on_resume回调
    if(existing_page->on_resume) (*existing_page->on_resume)(existing_page);

    printf("[page_manager] top=%d\n", page_manager.top);
    return PAGE_OK;
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
        lv_obj_move_foreground(new_page->obj);

        // on_resume回调
        if(new_page->on_resume) (*new_page->on_resume)(new_page);
    }

    // 延迟删除当前页面
    lv_obj_del_async(current_page->obj);
    free(current_page);

    // 如果需要，可以在这里触发页面切换动画
    printf("[page_manager] top=%d\n", page_manager.top);
}

// 将已有的一个页面关闭，其他页面前移填补空位
page_ret_code_t page_close_existing(char * page_id)
{
    printf("[page_manager] page_id=%s\n", page_id);
    // 没有页面你关个啥
    if(!page_id) return PAGE_ERR_NEW_NULL;
    if(page_manager.top < 0) return PAGE_ERR_STACK_EMPTY;
    
    BasePage * current_page = page_manager.stack[page_manager.top];
    if(!current_page) return PAGE_ERR_CURRENT_NULL;

    // 如果就是当前页，直接page_back()
    if(strcmp(current_page->page_id, page_id) == 0) {
        page_back();
        return PAGE_OK;
    }

    // 遍历列表，查找那个页面的位置（当前位置已经检查过，减一跳过）
    int index;
    BasePage * existing_page;
    for (index = page_manager.top - 1; index >= 0; index--)
    {
        existing_page = page_manager.stack[index];
        if(!existing_page) continue;
        if(strcmp(existing_page->page_id, page_id) == 0) break;
    }
    printf("[page_manager] index=%d\n", index);
    if(index < 0) return PAGE_ERR_NO_EXISTING;
    
    // 将这个位置后的所有页面都前移一位
    for (int i = index; i < page_manager.top; i++)
    {
        page_manager.stack[i] = page_manager.stack[i+1];
    }
    
    lv_obj_add_flag(existing_page->obj, LV_OBJ_FLAG_HIDDEN);

    // on_destroy回调
    if(existing_page->on_destroy) (*existing_page->on_destroy)(existing_page);

    // 显示上一页面
    page_manager.top--;
    if(page_manager.top >= 0) {
        BasePage * new_page = page_manager.stack[page_manager.top];
        lv_obj_clear_flag(new_page->obj, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(new_page->obj);

        // on_resume回调
        if(new_page->on_resume) (*new_page->on_resume)(new_page);
    }

    // 延迟删除当前页面
    lv_obj_del_async(existing_page->obj);
    free(existing_page);

    printf("[page_manager] top=%d\n", page_manager.top);
    return PAGE_OK;
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
        
        if(current_page->page_type == PAGE_TYPE_DIALOG && key_code != KEY_CODE_POWER) ret = true;

        return ret;
    }
    return false;
}

void back_cb(lv_event_t * e)
{
    page_back();
}