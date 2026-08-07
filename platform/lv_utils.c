#include "lv_utils.h"

/**
 * 获取字体
 */
lv_font_t * font_get(uint16_t weight, uint16_t font_style)
{
    lv_style_t style;
    lv_style_init(&style);

    lv_ft_info_t ft_info;
    ft_info.name   = "./res/font.ttf";
    ft_info.weight = weight;
    ft_info.style  = font_style;
    ft_info.mem    = NULL;

    if(lv_ft_font_init(&ft_info)) {
        return ft_info.font;
    }

    return NULL;
}

/**
 * 更新布局并获取该控件的百分比宽度
 */
lv_coord_t lv_obj_get_width_pct(lv_obj_t * obj, float pct)
{
    lv_obj_update_layout(obj);
    return (lv_coord_t)(lv_obj_get_width(obj) * pct / 100);
}

/**
 * 更新布局并获取该控件的百分比高度
 */
lv_coord_t lv_obj_get_height_pct(lv_obj_t * obj, float pct)
{
    lv_obj_update_layout(obj);
    return (lv_coord_t)(lv_obj_get_height(obj) * pct / 100);
}