#include "page_apple.h"

static void back_click(lv_event_t * e);

lv_obj_t * page_apple()
{
    lv_obj_t * screen = lv_obj_create(lv_scr_act());
    lv_obj_remove_style_all(screen);
    lv_obj_set_size(screen, lv_pct(100), lv_pct(100));

    lv_obj_t * player = lv_ffmpeg_player_create(screen);
    lv_obj_set_size(player, lv_pct(100), lv_pct(75));
    lv_ffmpeg_player_set_src(player, "./res/BadApple.mp4");
    lv_ffmpeg_player_set_auto_restart(player, false);
    //player->ffmpeg_ctx->skip_frame = AVDISCARD_NONKEY;
    //lv_ffmpeg_player_set_volume(player, 100);
    //lv_ffmpeg_player_set_mute(player, false);
    //lv_ffmpeg_player_set_zoom_mode(player, LV_FFMPEG_PLAYER_ZOOM_MODE_FIT);
    lv_ffmpeg_player_set_cmd(player, LV_FFMPEG_PLAYER_CMD_START);
    lv_obj_center(player);

    lv_obj_t * btn_back = lv_btn_create(screen);
    lv_obj_set_size(btn_back, lv_pct(25), lv_pct(12));
    lv_obj_align(btn_back, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    lv_obj_t * btn_back_label = lv_label_create(btn_back);
    lv_label_set_text(btn_back_label, "back");
    lv_obj_center(btn_back_label);
    lv_obj_add_event_cb(btn_back, back_click, LV_EVENT_CLICKED, NULL);

    return screen;
}

static void back_click(lv_event_t * e)
{
    page_back();
}