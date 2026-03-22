#include "page_bird.h"

typedef struct
{
    BasePage base;
    lv_timer_t * timer_move;
    lv_obj_t * bird;
    lv_obj_t * tubeA;
    lv_obj_t * tubeB;
    lv_obj_t * touch_area;
    lv_obj_t * btn_restart;
    lv_obj_t * img_logo;
    lv_obj_t * img_over;
    lv_obj_t * img_tap;
    lv_obj_t * label_score;

    int birdY;
    double birdSpeed;
    int tubeAX;
    int tubeAY;
    int tubeBX;
    int tubeBY;
    bool tubeAScored;
    bool tubeBScored;
    bool started;
    bool over;
    int score;
} BirdPage;

#define BIRD_WIDTH 36
#define BIRD_WIDTH_DELTA 6
#define BIRD_HEIGHT 24
#define TUBE_WIDTH 56
#define TUBE_HEIGHT 480
#define GAP_HEIGHT 100
#define BIRD_X 48
#define BIRD_Y_INIT 135
#define TUBE_AX_INIT LV_SCR_WIDTH
#define TUBE_BX_INIT LV_SCR_WIDTH * 1.5 + TUBE_WIDTH * 0.5
#define TUBE_RAND_MIN (LV_SCR_HEIGHT - GAP_HEIGHT) * 0.25
#define TUBE_RAND_MAX (LV_SCR_HEIGHT - GAP_HEIGHT) * 0.75

static lv_obj_t * page_bird_obj(BirdPage * page);
static void page_bird_destroy(void * p);

static void back_click(lv_event_t * e);
static void touch_pressed(lv_event_t * e);
static void restart_click(lv_event_t * e);
static void timer_move_tick(lv_timer_t * e);
static void game_init(BirdPage * page);
static void game_score(BirdPage * page);
static void game_stop(BirdPage * page);
static bool page_bird_on_key(void * p, key_code_t key_code, key_action_t key_action);

static int rand_between(int start, int end);

BasePage * page_bird_create(void)
{
    BirdPage * page = malloc(sizeof(BirdPage));
    if(!page) return NULL;
    memset(page, 0, sizeof(BirdPage));

    page->base.obj        = page_bird_obj(page);
    page->base.on_destroy = page_bird_destroy;
    page->base.on_key     = page_bird_on_key;
    return (BasePage *)page;
}

static lv_obj_t * page_bird_obj(BirdPage * page)
{
    lv_obj_t * screen = lv_obj_create(lv_scr_act());
    lv_obj_remove_style_all(screen);
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(screen, lv_pct(100), lv_pct(100));

    lv_obj_t * background = lv_img_create(screen);
    lv_obj_set_size(background, LV_SCR_WIDTH, LV_SCR_HEIGHT);
    lv_obj_set_pos(background, 0, 0);
    lv_img_set_src(background, "./res/bird/background.png");

    lv_obj_t * tubeA = lv_img_create(screen);
    lv_obj_set_size(tubeA, TUBE_WIDTH, TUBE_HEIGHT);
    lv_img_set_src(tubeA, "./res/bird/tube.png");
    page->tubeA = tubeA;

    lv_obj_t * tubeB = lv_img_create(screen);
    lv_obj_set_size(tubeB, TUBE_WIDTH, TUBE_HEIGHT);
    lv_img_set_src(tubeB, "./res/bird/tube.png");
    page->tubeB = tubeB;

    lv_obj_t * bird = lv_img_create(screen);
    lv_obj_set_size(bird, BIRD_WIDTH, BIRD_HEIGHT);
    lv_img_set_src(bird, "./res/bird/bird2.png");
    page->bird = bird;

    lv_obj_t * img_tap = lv_img_create(screen);
    lv_obj_set_size(img_tap, 120, 98);
    lv_obj_align(img_tap, LV_ALIGN_TOP_MID, 0, lv_pct(42));
    lv_img_set_src(img_tap, "./res/bird/tap.png");
    page->img_tap = img_tap;

    lv_obj_t * img_over = lv_img_create(screen);
    lv_obj_set_size(img_over, 208, 60);
    lv_obj_align(img_over, LV_ALIGN_TOP_MID, 0, lv_pct(20));
    lv_obj_add_flag(img_over, LV_OBJ_FLAG_HIDDEN);
    lv_img_set_src(img_over, "./res/bird/over.png");
    page->img_over = img_over;

    lv_obj_t * img_logo = lv_img_create(screen);
    lv_obj_set_size(img_logo, 178, 48);
    lv_obj_align(img_logo, LV_ALIGN_TOP_MID, 0, lv_pct(16));
    lv_img_set_src(img_logo, "./res/bird/logo.png");
    page->img_logo = img_logo;

    lv_obj_t * label_score = lv_label_create(screen);
    lv_obj_align(label_score, LV_ALIGN_TOP_MID, 0, lv_pct(5));
    lv_label_set_text(label_score, "0");
    page->label_score = label_score;

    lv_obj_t * touch_area = lv_obj_create(screen);
    lv_obj_set_size(touch_area, LV_SCR_WIDTH, LV_SCR_HEIGHT);
    lv_obj_set_pos(touch_area, 0, 0);
    lv_obj_set_style_bg_opa(touch_area, LV_OPA_0, 0);
    lv_obj_set_style_border_opa(touch_area, LV_OPA_0, 0);
    lv_obj_set_style_outline_opa(touch_area, LV_OPA_0, 0);
    lv_obj_set_style_shadow_opa(touch_area, LV_OPA_0, 0);
    lv_obj_add_event_cb(touch_area, touch_pressed, LV_EVENT_PRESSED, page);
    page->touch_area = touch_area;

    lv_obj_t * btn_restart = lv_img_create(screen);
    lv_obj_set_size(btn_restart, 120, 72);
    lv_obj_align(btn_restart, LV_ALIGN_TOP_MID, 0, lv_pct(55));
    lv_obj_add_flag(btn_restart, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(btn_restart, LV_OBJ_FLAG_CLICKABLE);
    lv_img_set_src(btn_restart, "./res/bird/start.png");
    lv_obj_add_event_cb(btn_restart, restart_click, LV_EVENT_CLICKED, page);
    page->btn_restart = btn_restart;

    lv_obj_t * btn_back = lv_obj_create(screen);
    lv_obj_set_size(btn_back, lv_pct(100), lv_pct(12));
    lv_obj_align(btn_back, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_style_bg_opa(btn_back, LV_OPA_0, 0);
    lv_obj_set_style_border_opa(btn_back, LV_OPA_0, 0);
    lv_obj_set_style_outline_opa(btn_back, LV_OPA_0, 0);
    lv_obj_set_style_shadow_opa(btn_back, LV_OPA_0, 0);
    lv_obj_add_event_cb(btn_back, back_click, LV_EVENT_CLICKED, page);

    game_init(page);
    page->timer_move = lv_timer_create(timer_move_tick, 25, page);
    lv_timer_pause(page->timer_move);

    return screen;
}

static void timer_move_tick(lv_timer_t * t)
{
    BirdPage * page = (BirdPage *)t->user_data;
    if(!page) return;

    // 下方Y坐标均以屏幕底部为零点
    // 先将变量取出进行运算，再将修改过的值存回去
    int tubeAX       = page->tubeAX;
    int tubeAY       = page->tubeAY;
    int tubeBX       = page->tubeBX;
    int tubeBY       = page->tubeBY;
    lv_obj_t * bird  = page->bird;
    lv_obj_t * tubeA = page->tubeA;
    lv_obj_t * tubeB = page->tubeB;
    double birdSpeed = page->birdSpeed;
    int birdY        = page->birdY;

    birdSpeed -= 1;
    birdY += birdSpeed;

    if(birdSpeed < -15) birdSpeed = -15;
    if(birdY < BIRD_HEIGHT) birdY = BIRD_HEIGHT;
    if(birdY > LV_SCR_HEIGHT) birdY = LV_SCR_HEIGHT;

    if(!page->over) {
        tubeAX -= 3;
        tubeBX -= 3;
    }

    if(tubeAX < -TUBE_WIDTH) {
        tubeAX            = LV_SCR_WIDTH;
        tubeAY            = rand_between(TUBE_RAND_MIN, TUBE_RAND_MAX);
        page->tubeAScored = false;
    }
    if(tubeBX < -TUBE_WIDTH) {
        tubeBX            = LV_SCR_WIDTH;
        tubeBY            = rand_between(TUBE_RAND_MIN, TUBE_RAND_MAX);
        page->tubeBScored = false;
    }

    // 运算已经结束了，把改过的变量存回去
    page->birdSpeed = birdSpeed;
    page->birdY     = birdY;
    page->tubeAX    = tubeAX;
    page->tubeAY    = tubeAY;
    page->tubeBX    = tubeBX;
    page->tubeBY    = tubeBY;

    lv_obj_set_pos(bird, BIRD_X, LV_SCR_HEIGHT - birdY);
    lv_obj_set_pos(tubeA, tubeAX, -TUBE_HEIGHT + LV_SCR_HEIGHT + (TUBE_HEIGHT - GAP_HEIGHT) / 2 - tubeAY);
    lv_obj_set_pos(tubeB, tubeBX, -TUBE_HEIGHT + LV_SCR_HEIGHT + (TUBE_HEIGHT - GAP_HEIGHT) / 2 - tubeBY);

    if(!page->over) {
        if(BIRD_X - TUBE_WIDTH + BIRD_WIDTH_DELTA < tubeAX && tubeAX < BIRD_X + BIRD_WIDTH - BIRD_WIDTH_DELTA) {
            if(!(tubeAY < birdY - BIRD_HEIGHT && tubeAY + GAP_HEIGHT > birdY)) {
                page->over = true;
            }
        }
        if(!page->tubeAScored && tubeAX < BIRD_X + BIRD_WIDTH / 2) {
            page->tubeAScored = true;
            game_score(page);
        }

        if(BIRD_X - TUBE_WIDTH + BIRD_WIDTH_DELTA < tubeBX && tubeBX < BIRD_X + BIRD_WIDTH - BIRD_WIDTH_DELTA) {
            if(!(tubeBY < birdY - BIRD_HEIGHT && tubeBY + GAP_HEIGHT > birdY)) {
                page->over = true;
            }
        }
        if(!page->tubeBScored && tubeBX < BIRD_X + BIRD_WIDTH / 2) {
            page->tubeBScored = true;
            game_score(page);
        }
    }

    if(birdY <= BIRD_HEIGHT) {
        page->over = true;
        game_stop(page);
    }

}

static void game_init(BirdPage * page)
{
    srand((unsigned)time(NULL));

    page->score = 0;
    lv_label_set_text(page->label_score, "0");

    page->birdSpeed = 10;
    page->birdY     = BIRD_Y_INIT;

    page->tubeAX      = TUBE_AX_INIT;
    page->tubeBX      = TUBE_BX_INIT;
    page->tubeAY      = rand_between(TUBE_RAND_MIN, TUBE_RAND_MAX);
    page->tubeBY      = rand_between(TUBE_RAND_MIN, TUBE_RAND_MAX);
    page->tubeAScored = false;
    page->tubeBScored = false;

    lv_obj_set_pos(page->bird, BIRD_X, LV_SCR_HEIGHT - page->birdY);
    lv_obj_set_pos(page->tubeA, page->tubeAX, page->tubeAY - (TUBE_HEIGHT / 2));
    lv_obj_set_pos(page->tubeB, page->tubeBX, page->tubeBY - (TUBE_HEIGHT / 2));

    page->started = false;
    page->over    = false;
}

static void game_score(BirdPage * page)
{
    page->score += 1;
    lv_label_set_text_fmt(page->label_score, "%d", page->score);
}

static void game_stop(BirdPage * page)
{
    page->started = false;
    lv_timer_pause(page->timer_move);
    lv_obj_clear_flag(page->img_over, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(page->btn_restart, LV_OBJ_FLAG_HIDDEN);
}

static void restart_click(lv_event_t * e)
{
    BirdPage * page = (BirdPage *)e->user_data;
    if(!page) return;

    lv_obj_add_flag(page->img_over, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(page->btn_restart, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(page->img_tap, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(page->img_logo, LV_OBJ_FLAG_HIDDEN);

    game_init(page);
}

static void touch_pressed(lv_event_t * e)
{
    BirdPage * page = (BirdPage *)e->user_data;
    if(!page) return;

    if(!page->over) {
        if(!page->started) {
            lv_obj_add_flag(page->img_tap, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(page->img_logo, LV_OBJ_FLAG_HIDDEN);
            page->started = true;
            lv_timer_resume(page->timer_move);
        }
        page->birdSpeed = 10;
    }
}

static bool page_bird_on_key(void * p, key_code_t key_code, key_action_t key_action)
{
    if(!p) return false;
    if(key_code != KEY_CODE_HOME) return false;
    if(key_action != KEY_ACTION_DOWN) return true;

    BirdPage * page = (BirdPage *)p;

    if(!page->over) {
        if(!page->started) {
            lv_obj_add_flag(page->img_tap, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(page->img_logo, LV_OBJ_FLAG_HIDDEN);
            page->started = true;
            lv_timer_resume(page->timer_move);
        }
        page->birdSpeed = 10;
    } 
    else if(!page->started) {
        lv_obj_add_flag(page->img_over, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(page->btn_restart, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(page->img_tap, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(page->img_logo, LV_OBJ_FLAG_HIDDEN);

        game_init(page);
    }
    return true;
}

static void back_click(lv_event_t * e)
{
    page_back();
}

static void page_bird_destroy(void * p)
{
    BirdPage * page = (BirdPage *)p;
    if(!page) return;

    lv_timer_pause(page->timer_move);
    lv_timer_del(page->timer_move);
    lv_img_cache_invalidate_src(NULL);
}

static int rand_between(int start, int end)
{
    return rand() % (end - start + 1) + start;
}