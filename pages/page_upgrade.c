#include "page_upgrade.h"

typedef struct {
    BasePage base;
    char json_url[256];
    char remote_version[16];
    char file_url[256];
    char file_name[128];
    char file_sha1[128];
    char file_size[32];
    char upgrade_info_url[256];
    bool has_new_version;
    bool updating;

    lv_obj_t * label_status;
    lv_obj_t * label_version;
    lv_obj_t * btn_check;
    lv_obj_t * btn_back;
    lv_obj_t * btn_info;
    lv_obj_t * btn_update;
    lv_obj_t * btn_detail;

    char local_version[16];
    char result_msg[512]; 
    char script_output[4096];

    /* 线程相关 */
    pthread_t check_thread;
    pthread_t upgrade_thread;
    pthread_mutex_t lock;
    bool thread_running;
} UpgradePage;

struct status_msg {
    UpgradePage *page;
    char text[64];
};

struct curl_data {
    char * buf;
    size_t size;
};

static lv_obj_t * upgrade_ui_create(UpgradePage * page);
static void upgrade_on_create(void * p);
static void upgrade_on_destroy(void * p);
static bool upgrade_on_key(void * p, key_code_t key_code, key_action_t key_action);
static void btn_back_cb(lv_event_t * e);
static void btn_info_cb(lv_event_t * e);
static void btn_update_cb(lv_event_t * e);
static void btn_detail_cb(lv_event_t * e);
static void btn_check_cb(lv_event_t * e);
static void update_status(UpgradePage * page, const char * text);
static void lock_ui(UpgradePage * page, bool lock);
static void set_thread_running(UpgradePage * page, bool running);
static bool is_thread_running(UpgradePage * page);

/* 后台线程 */
static void * check_update_thread(void * arg);
static void * upgrade_thread(void * arg);

/* UI 更新回调（主线程） */
static void ui_show_check_result(void * p);
static void ui_show_download_fail(void * p);
static void ui_show_sha1_fail(void * p);
static void ui_show_extract_fail(void * p);
static void ui_show_install_fail(void * p);
static void ui_show_install_success(void * p);
static void ui_update_status_cb(void * p);   /* 接收 status_msg 并更新屏幕状态 */

/* 工具函数 */
static int read_local_version(char * buf, size_t size);
static int download_file(const char * url, const char * save_path);
static size_t curl_write_memory(void * ptr, size_t size, size_t nmemb, void * data);
static int sha1_verify(const char * file_path, const char * expected_sha1);
static int run_install_script(UpgradePage * page);
static void show_success_dialog(UpgradePage * page);
static void dialog_restart_cb(lv_event_t * e);
static void dialog_later_cb(lv_event_t * e);
static void cleanup_tmp(void);
extern void sys_set_dont_deep_sleep(bool b);
extern void sys_set_dont_timeout(bool b);

/* 异步更新状态的辅助函数 */
static void update_status_async(UpgradePage *page, const char *text);

BasePage * page_upgrade_create(const char * json_url)
{
    UPGRADE_DEBUG("page_upgrade_create called, json_url=%s", json_url ? json_url : "NULL");
    UpgradePage * page = malloc(sizeof(UpgradePage));
    if (!page) {
        UPGRADE_DEBUG("Failed to allocate memory for UpgradePage");
        return NULL;
    }
    memset(page, 0, sizeof(UpgradePage));

    strncpy(page->json_url, json_url, sizeof(page->json_url) - 1);
    page->base.obj = upgrade_ui_create(page);
    page->base.on_create = upgrade_on_create;
    page->base.on_destroy = upgrade_on_destroy;
    page->base.on_key = upgrade_on_key;

    UPGRADE_DEBUG("UpgradePage created successfully");
    return (BasePage *)page;
}

static lv_obj_t * upgrade_ui_create(UpgradePage * page)
{
    UPGRADE_DEBUG("Creating upgrade UI");
    lv_obj_t * scr = lv_obj_create(lv_scr_act());
    lv_obj_remove_style_all(scr);
    lv_obj_set_size(scr, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_color(scr, lv_color_white(), 0);

    /* 当前版本号（居中向上7像素） */
    page->label_version = lv_label_create(scr);
    lv_obj_set_width(page->label_version, lv_pct(90));
    lv_obj_align(page->label_version, LV_ALIGN_CENTER, 0, -7);
    lv_obj_set_style_text_align(page->label_version, LV_TEXT_ALIGN_CENTER, 0);
    char ver_text[64];
    if (read_local_version(page->local_version, sizeof(page->local_version)) == 0) {
        snprintf(ver_text, sizeof(ver_text), "当前版本: %s", page->local_version);
        UPGRADE_DEBUG("Local version: %s", page->local_version);
    } else {
        snprintf(ver_text, sizeof(ver_text), "当前版本: 未知");
        UPGRADE_DEBUG("Local version unknown");
    }
    lv_label_set_text(page->label_version, ver_text);

    /* 检查更新按钮 */
    page->btn_check = lv_btn_create(scr);
    lv_obj_set_size(page->btn_check, 80, 35);
    lv_obj_align_to(page->btn_check, page->label_version, LV_ALIGN_OUT_BOTTOM_MID, 0, 7);
    lv_obj_t * lbl_check = lv_label_create(page->btn_check);
    lv_label_set_text(lbl_check, "检查更新");
    lv_obj_center(lbl_check);
    lv_obj_add_event_cb(page->btn_check, btn_check_cb, LV_EVENT_CLICKED, page);

    /* 状态标签（初始隐藏） */
    page->label_status = lv_label_create(scr);
    lv_obj_set_width(page->label_status, lv_pct(90));
    lv_obj_align(page->label_status, LV_ALIGN_CENTER, 0, -7);
    lv_obj_set_style_text_align(page->label_status, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_add_flag(page->label_status, LV_OBJ_FLAG_HIDDEN);

    /* 更新按钮（初始隐藏） */
    page->btn_update = lv_btn_create(scr);
    lv_obj_set_size(page->btn_update, 80, 35);
    lv_obj_add_flag(page->btn_update, LV_OBJ_FLAG_HIDDEN);
    lv_obj_t * lbl_upd = lv_label_create(page->btn_update);
    lv_label_set_text(lbl_upd, "更新");
    lv_obj_center(lbl_upd);
    lv_obj_add_event_cb(page->btn_update, btn_update_cb, LV_EVENT_CLICKED, page);

    /* 详情按钮（初始隐藏） */
    page->btn_detail = lv_btn_create(scr);
    lv_obj_set_size(page->btn_detail, 90, 35);
    lv_obj_add_flag(page->btn_detail, LV_OBJ_FLAG_HIDDEN);
    lv_obj_t * lbl_det = lv_label_create(page->btn_detail);
    lv_label_set_text(lbl_det, "详情");
    lv_obj_center(lbl_det);
    lv_obj_add_event_cb(page->btn_detail, btn_detail_cb, LV_EVENT_CLICKED, page);

    /* 左下返回按钮 */
    page->btn_back = lv_btn_create(scr);
    lv_obj_set_size(page->btn_back, lv_pct(25), lv_pct(12));
    lv_obj_align(page->btn_back, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    lv_obj_t * lbl_back = lv_label_create(page->btn_back);
    lv_label_set_text(lbl_back, CUSTOM_SYMBOL_BACK "");
    lv_obj_center(lbl_back);
    lv_obj_add_event_cb(page->btn_back, btn_back_cb, LV_EVENT_CLICKED, page);

    /* 更新内容按钮（初始隐藏，检查更新后显示） */
    page->btn_info = lv_btn_create(scr);
    lv_obj_set_size(page->btn_info, lv_pct(30), 28);
    lv_obj_align(page->btn_info, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
    lv_obj_add_flag(page->btn_info, LV_OBJ_FLAG_HIDDEN);
    lv_obj_t * lbl_info = lv_label_create(page->btn_info);
    lv_label_set_text(lbl_info, "更新内容");
    lv_obj_center(lbl_info);
    lv_obj_add_event_cb(page->btn_info, btn_info_cb, LV_EVENT_CLICKED, page);

    UPGRADE_DEBUG("UI created successfully");
    return scr;
}

static void upgrade_on_create(void * p)
{
    UpgradePage * page = (UpgradePage *)p;
    if (!page) return;
    sys_set_dont_deep_sleep(true);
    sys_set_dont_timeout(true);
    pthread_mutex_init(&page->lock, NULL);
    page->thread_running = false;
    UPGRADE_DEBUG("Mutex initialized, screen sleep/timeout disabled");
}

static void upgrade_on_destroy(void * p)
{
    UpgradePage * page = (UpgradePage *)p;
    UPGRADE_DEBUG("Page on_destroy");
    if (!page) return;

    if (page->thread_running) {
        pthread_mutex_lock(&page->lock);
        page->thread_running = false;
        pthread_mutex_unlock(&page->lock);
        if (page->check_thread) pthread_cancel(page->check_thread);
        if (page->upgrade_thread) pthread_cancel(page->upgrade_thread);
        if (page->check_thread) pthread_join(page->check_thread, NULL);
        if (page->upgrade_thread) pthread_join(page->upgrade_thread, NULL);
        UPGRADE_DEBUG("Threads cancelled and joined");
    }

    cleanup_tmp();
    sys_set_dont_deep_sleep(false);
    sys_set_dont_timeout(false);
    pthread_mutex_destroy(&page->lock);
    curl_global_cleanup();
    UPGRADE_DEBUG("Resources released");
}

static bool upgrade_on_key(void * p, key_code_t key_code, key_action_t key_action)
{
    if (key_code == KEY_CODE_HOME) {
        UPGRADE_DEBUG("HOME key intercepted");
        return true;
    }
    return false;
}

static void btn_back_cb(lv_event_t * e)
{
    UpgradePage * page = (UpgradePage *)lv_event_get_user_data(e);
    if (!page) return;
    bool locked;
    pthread_mutex_lock(&page->lock);
    locked = page->updating;
    pthread_mutex_unlock(&page->lock);
    if (locked) return;
    UPGRADE_DEBUG("Back button clicked");
    page_back();
}

static void btn_check_cb(lv_event_t * e)
{
    UpgradePage * page = (UpgradePage *)lv_event_get_user_data(e);
    if (!page) return;
    lv_obj_add_flag(page->label_version, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(page->btn_check, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(page->label_status, LV_OBJ_FLAG_HIDDEN);
    update_status(page, "正在检查更新...");
    UPGRADE_DEBUG("Check update button clicked");

    pthread_mutex_lock(&page->lock);
    page->thread_running = true;
    pthread_mutex_unlock(&page->lock);
    if (pthread_create(&page->check_thread, NULL, check_update_thread, page) != 0) {
        UPGRADE_DEBUG("Failed to create check thread");
        update_status(page, "网络初始化失败");
        pthread_mutex_lock(&page->lock);
        page->thread_running = false;
        pthread_mutex_unlock(&page->lock);
    }
}

static void btn_info_cb(lv_event_t * e)
{
    UpgradePage * page = (UpgradePage *)lv_event_get_user_data(e);
    if (!page) return;
    bool locked;
    pthread_mutex_lock(&page->lock);
    locked = page->updating;
    pthread_mutex_unlock(&page->lock);
    if (locked) return;

    UPGRADE_DEBUG("Info button clicked, downloading upgrade info from %s", page->upgrade_info_url);
    if (download_file(page->upgrade_info_url, TMP_UPGRADE_INFO) == 0) {
        UPGRADE_DEBUG("Opening upgrade info text page");
        page_open(page_txt_create(TMP_UPGRADE_INFO));
    } else {
        update_status(page, "更新内容下载失败");
        UPGRADE_DEBUG("Failed to download upgrade info");
    }
}

static void btn_update_cb(lv_event_t * e)
{
    UpgradePage * page = (UpgradePage *)lv_event_get_user_data(e);
    if (!page) return;
    bool locked;
    pthread_mutex_lock(&page->lock);
    locked = page->updating;
    pthread_mutex_unlock(&page->lock);
    if (locked) return;

    UPGRADE_DEBUG("Update button clicked");
    pthread_mutex_lock(&page->lock);
    page->thread_running = true;
    page->updating = true;
    pthread_mutex_unlock(&page->lock);
    lock_ui(page, true);

    lv_obj_add_flag(page->btn_update, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(page->btn_detail, LV_OBJ_FLAG_HIDDEN);

    if (pthread_create(&page->upgrade_thread, NULL, upgrade_thread, page) != 0) {
        UPGRADE_DEBUG("Failed to create upgrade thread");
        update_status(page, "启动失败");
        lock_ui(page, false);
        pthread_mutex_lock(&page->lock);
        page->thread_running = false;
        page->updating = false;
        pthread_mutex_unlock(&page->lock);
    }
}

static void btn_detail_cb(lv_event_t * e)
{
    UpgradePage * page = (UpgradePage *)lv_event_get_user_data(e);
    if (!page) return;
    UPGRADE_DEBUG("Detail button clicked, opening error log");
    page_open(page_txt_create(TMP_ERROR_LOG));
}

static void update_status(UpgradePage * page, const char * text)
{
    UPGRADE_DEBUG("Status: %s", text);
    lv_label_set_text(page->label_status, text);
    lv_timer_handler();
}

static void lock_ui(UpgradePage * page, bool lock)
{
    page->updating = lock;
    if (lock) {
        lv_obj_add_state(page->btn_back, LV_STATE_DISABLED);
        lv_obj_add_state(page->btn_info, LV_STATE_DISABLED);
        lv_obj_add_state(page->btn_update, LV_STATE_DISABLED);
        lv_obj_add_state(page->btn_detail, LV_STATE_DISABLED);
    } else {
        lv_obj_clear_state(page->btn_back, LV_STATE_DISABLED);
        lv_obj_clear_state(page->btn_info, LV_STATE_DISABLED);
        lv_obj_clear_state(page->btn_update, LV_STATE_DISABLED);   // 添加
        lv_obj_clear_state(page->btn_detail, LV_STATE_DISABLED);   // 添加
    }
}

static void set_thread_running(UpgradePage * page, bool running)
{
    pthread_mutex_lock(&page->lock);
    page->thread_running = running;
    pthread_mutex_unlock(&page->lock);
}

static bool is_thread_running(UpgradePage * page)
{
    bool ret;
    pthread_mutex_lock(&page->lock);
    ret = page->thread_running;
    pthread_mutex_unlock(&page->lock);
    return ret;
}

/* 辅助函数：异步更新状态文本 */
static void update_status_async(UpgradePage *page, const char *text)
{
    struct status_msg *msg = malloc(sizeof(struct status_msg));
    if (!msg) return;
    msg->page = page;
    snprintf(msg->text, sizeof(msg->text), "%s", text);
    lv_async_call(ui_update_status_cb, msg);
}

/* ================== 检查更新线程 ================== */
static void * check_update_thread(void * arg)
{
    UpgradePage * page = (UpgradePage *)arg;
    UPGRADE_DEBUG("Check update thread started, fetching %s", page->json_url);

    struct curl_data json_data = {0};
    CURL * curl = curl_easy_init();
    if (!curl) {
        UPGRADE_DEBUG("curl_easy_init() failed");
        lv_async_call(ui_show_check_result, page);
        set_thread_running(page, false);
        return NULL;
    }
    curl_easy_setopt(curl, CURLOPT_URL, page->json_url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_memory);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &json_data);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_CAINFO, CA_BUNDLE_PATH);
    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        UPGRADE_DEBUG("curl perform failed: %d (%s)", res, curl_easy_strerror(res));
        lv_async_call(ui_show_check_result, page);
        free(json_data.buf);
        set_thread_running(page, false);
        return NULL;
    }

    if (!json_data.buf) {
        UPGRADE_DEBUG("curl received empty data");
        lv_async_call(ui_show_check_result, page);
        set_thread_running(page, false);
        return NULL;
    }
    UPGRADE_DEBUG("JSON downloaded, size=%zu", json_data.size);

    cJSON * root = cJSON_Parse(json_data.buf);
    free(json_data.buf);
    if (!root) {
        UPGRADE_DEBUG("JSON parse error");
        lv_async_call(ui_show_check_result, page);
        set_thread_running(page, false);
        return NULL;
    }

    cJSON * ver = cJSON_GetObjectItem(root, "version");
    cJSON * furl = cJSON_GetObjectItem(root, "file_url");
    cJSON * fname = cJSON_GetObjectItem(root, "file_name");
    cJSON * sha1 = cJSON_GetObjectItem(root, "file_sha1");
    cJSON * info_url = cJSON_GetObjectItem(root, "upgrade_info_url");
    cJSON * fsize = cJSON_GetObjectItem(root, "file_size");

    if (!ver || !furl || !fname || !sha1 || !info_url) {
        UPGRADE_DEBUG("Incomplete JSON fields");
        cJSON_Delete(root);
        lv_async_call(ui_show_check_result, page);
        set_thread_running(page, false);
        return NULL;
    }

    snprintf(page->remote_version, sizeof(page->remote_version), "%s", ver->valuestring ? ver->valuestring : "");
    snprintf(page->file_url, sizeof(page->file_url), "%s", furl->valuestring);
    snprintf(page->file_name, sizeof(page->file_name), "%s", fname->valuestring);
    snprintf(page->file_sha1, sizeof(page->file_sha1), "%s", sha1->valuestring);
    snprintf(page->upgrade_info_url, sizeof(page->upgrade_info_url), "%s", info_url->valuestring);
    if (fsize && fsize->valuestring) {
        snprintf(page->file_size, sizeof(page->file_size), "%s", fsize->valuestring);
        UPGRADE_DEBUG("Remote file size: %s", page->file_size);
    } else {
        page->file_size[0] = '\0';
    }
    cJSON_Delete(root);

    UPGRADE_DEBUG("Remote version=%s, file=%s, sha1=%s", page->remote_version, page->file_name, page->file_sha1);

    read_local_version(page->local_version, sizeof(page->local_version));
    page->has_new_version = (strcmp(page->remote_version, page->local_version) != 0);
    UPGRADE_DEBUG("Version compare: remote=%s local=%s different=%d", page->remote_version, page->local_version, page->has_new_version);

    lv_async_call(ui_show_check_result, page);
    set_thread_running(page, false);
    return NULL;
}

static void ui_show_check_result(void * p)
{
    UpgradePage * page = (UpgradePage *)p;
    if (!page) return;

    if (page->remote_version[0] == '\0') {
        update_status(page, "检查更新失败");
        return;
    }

    lv_obj_clear_flag(page->btn_info, LV_OBJ_FLAG_HIDDEN);

    char status_text[128];
    if (page->has_new_version) {
        snprintf(status_text, sizeof(status_text), "发现新版本 %s", page->remote_version);
        lv_obj_set_style_text_color(page->label_status, lv_color_hex(0xD4A017), 0);
        lv_obj_align(page->label_status, LV_ALIGN_CENTER, 0, -7);
        lv_obj_clear_flag(page->btn_update, LV_OBJ_FLAG_HIDDEN);
        lv_obj_align_to(page->btn_update, page->label_status, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
    } else {
        snprintf(status_text, sizeof(status_text), "已是最新版本");
        lv_obj_set_style_text_color(page->label_status, lv_color_hex(0x888888), 0);
        lv_obj_align(page->label_status, LV_ALIGN_CENTER, 0, -7);
        lv_obj_add_flag(page->btn_update, LV_OBJ_FLAG_HIDDEN);
    }
    lv_label_set_text(page->label_status, status_text);
    lv_timer_handler();
}

/* ================== 升级线程 ================== */
static void * upgrade_thread(void * arg)
{
    UpgradePage * page = (UpgradePage *)arg;
    static char save_path[256];
    static char cmd[768];
    int ret;

    UPGRADE_DEBUG("Upgrade thread started");

    /* 1. 下载 */
    update_status_async(page, "正在下载更新...");
    snprintf(save_path, sizeof(save_path), "%s/%s", TMP_DIR, page->file_name);
    mkdir(TMP_DIR, 0755);
    UPGRADE_DEBUG("Downloading %s to %s", page->file_url, save_path);
    if (download_file(page->file_url, save_path) != 0) {
        lv_async_call(ui_show_download_fail, page);
        set_thread_running(page, false);
        pthread_mutex_lock(&page->lock);
        page->updating = false;
        pthread_mutex_unlock(&page->lock);
        return NULL;
    }
    UPGRADE_DEBUG("Download completed");

    /* 2. SHA1 校验 */
    update_status_async(page, "正在校验文件...");
    UPGRADE_DEBUG("Verifying SHA1: %s", save_path);
    if (sha1_verify(save_path, page->file_sha1) != 0) {
        unlink(save_path);
        lv_async_call(ui_show_sha1_fail, page);
        set_thread_running(page, false);
        pthread_mutex_lock(&page->lock);
        page->updating = false;
        pthread_mutex_unlock(&page->lock);
        return NULL;
    }
    UPGRADE_DEBUG("SHA1 verification passed");

    /* 3. 解压 */
    update_status_async(page, "正在安装更新...");
    snprintf(cmd, sizeof(cmd), "./tools/busybox_1 tar -xzvf %s -C %s", save_path, TMP_DIR);
    UPGRADE_DEBUG("Extracting: %s", cmd);
    ret = system(cmd);
    if (ret != 0) {
        UPGRADE_DEBUG("Extraction failed, ret=%d", ret);
        unlink(save_path);
        lv_async_call(ui_show_extract_fail, page);
        set_thread_running(page, false);
        pthread_mutex_lock(&page->lock);
        page->updating = false;
        pthread_mutex_unlock(&page->lock);
        return NULL;
    }
    UPGRADE_DEBUG("Extraction succeeded");

    /* 4. 安装脚本 */
    update_status_async(page, "正在安装...");
    system("chmod +x /tmp/dendro/install.sh");
    ret = run_install_script(page);
    if (ret == 0) {
        UPGRADE_DEBUG("Install script finished successfully");
        lv_async_call(ui_show_install_success, page);
    } else {
        UPGRADE_DEBUG("Install script failed, result_msg=%s", page->result_msg);
        FILE * fp = fopen(TMP_ERROR_LOG, "w");
        if (fp) {
            fputs(page->script_output, fp);
            fclose(fp);
        }
        lv_async_call(ui_show_install_fail, page);
    }

    set_thread_running(page, false);
    pthread_mutex_lock(&page->lock);
    page->updating = false;
    pthread_mutex_unlock(&page->lock);
    return NULL;
}

/* ================== 异步 UI 回调 ================== */
static void ui_update_status_cb(void * p)
{
    struct status_msg * msg = (struct status_msg *)p;
    if (!msg || !msg->page) return;
    update_status(msg->page, msg->text);
    free(msg);
}

static void ui_show_download_fail(void * p)
{
    UpgradePage * page = (UpgradePage *)p;
    if (!page) return;
    update_status(page, "下载失败");
    lock_ui(page, false);
}

static void ui_show_sha1_fail(void * p)
{
    UpgradePage * page = (UpgradePage *)p;
    if (!page) return;
    update_status(page, "文件校验失败");
    lock_ui(page, false);
}

static void ui_show_extract_fail(void * p)
{
    UpgradePage * page = (UpgradePage *)p;
    if (!page) return;
    update_status(page, "解压失败");
    lock_ui(page, false);
}

static void ui_show_install_fail(void * p)
{
    UpgradePage * page = (UpgradePage *)p;
    if (!page) return;
    update_status(page, "安装失败");
    lv_obj_align_to(page->btn_detail, page->label_status, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
    lv_obj_clear_flag(page->btn_detail, LV_OBJ_FLAG_HIDDEN);
    lock_ui(page, false);
}

static void ui_show_install_success(void * p)
{
    UpgradePage * page = (UpgradePage *)p;
    if (!page) return;
    page->has_new_version = false;
    show_success_dialog(page);
}

/* ========== 底层工具函数 ========== */
static int read_local_version(char * buf, size_t size)
{
    FILE * fp = fopen(LOCAL_VERSION_PATH, "r");
    if (!fp) return -1;
    if (!fgets(buf, size, fp)) {
        fclose(fp);
        return -1;
    }
    buf[strcspn(buf, "\r\n")] = '\0';
    fclose(fp);
    return 0;
}

static size_t curl_write_memory(void * ptr, size_t size, size_t nmemb, void * data)
{
    struct curl_data * cd = (struct curl_data *)data;
    size_t total = size * nmemb;
    cd->buf = realloc(cd->buf, cd->size + total + 1);
    if (!cd->buf) return 0;
    memcpy(cd->buf + cd->size, ptr, total);
    cd->size += total;
    cd->buf[cd->size] = '\0';
    return total;
}

static int download_file(const char * url, const char * save_path)
{
    UPGRADE_DEBUG("Downloading %s -> %s", url, save_path);
    CURL * curl = curl_easy_init();
    if (!curl) return -1;
    FILE * fp = fopen(save_path, "wb");
    if (!fp) {
        curl_easy_cleanup(curl);
        return -1;
    }
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_CAINFO, CA_BUNDLE_PATH);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    CURLcode res = curl_easy_perform(curl);
    fclose(fp);
    curl_easy_cleanup(curl);
    if (res != CURLE_OK) {
        UPGRADE_DEBUG("Download failed: curl error %d", res);
        unlink(save_path);
        return -1;
    }
    UPGRADE_DEBUG("Download succeeded");
    return 0;
}

static int sha1_verify(const char * file_path, const char * expected_sha1)
{
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "./tools/busybox_1 sha1sum %s", file_path);
    UPGRADE_DEBUG("Running SHA1 command: %s", cmd);
    FILE * fp = popen(cmd, "r");
    if (!fp) return -1;
    char sha1_buf[128] = {0};
    if (!fgets(sha1_buf, sizeof(sha1_buf), fp)) {
        pclose(fp);
        return -1;
    }
    pclose(fp);
    char * space = strchr(sha1_buf, ' ');
    if (space) *space = '\0';
    UPGRADE_DEBUG("Computed SHA1: %s, expected: %s", sha1_buf, expected_sha1);
    return strcasecmp(sha1_buf, expected_sha1) == 0 ? 0 : -1;
}

static int run_install_script(UpgradePage * page)
{
    FILE * fp = popen("/tmp/dendro/install.sh 2>&1", "r");
    if (!fp) return -1;

    static char line[512];         // 改为 static，减少栈占用
    static char prev_line[512];    // 改为 static
    static char output_buf[4096];  // 改为 static
    int is_success = 0;

    // 每次调用必须手动清空缓冲区，因为 static 变量会保留上次的内容
    prev_line[0] = '\0';
    output_buf[0] = '\0';

    while (fgets(line, sizeof(line), fp)) {
        printf("[install.sh] %s", line);
        strcat(output_buf, line);
        size_t len = strlen(line);
        while (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r'))
            line[--len] = '\0';
        if (strcmp(line, "End Of File") == 0) {
            char * last_line = prev_line;
            while (*last_line == ' ' || *last_line == '\t') last_line++;
            UPGRADE_DEBUG("Last line before EOF: '%s'", last_line);
            if (strcmp(last_line, "All Done!") == 0) {
                is_success = 1;
                strncpy(page->result_msg, "更新成功", sizeof(page->result_msg));
            } else {
                snprintf(page->result_msg, sizeof(page->result_msg), "%s", last_line[0] ? last_line : "未知错误");
            }
            break;
        }
        strncpy(prev_line, line, sizeof(prev_line));
    }
    pclose(fp);
    strncpy(page->script_output, output_buf, sizeof(page->script_output));
    page->script_output[sizeof(page->script_output)-1] = '\0';
    UPGRADE_DEBUG("Install script finished, success=%d", is_success);
    return is_success ? 0 : -1;
}

static void show_success_dialog(UpgradePage * page)
{
    lv_obj_t * scr = page->base.obj;

    lv_obj_t * bg = lv_obj_create(scr);
    lv_obj_set_size(bg, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_color(bg, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(bg, LV_OPA_50, 0);
    lv_obj_clear_flag(bg, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_move_foreground(bg);

    lv_obj_t * box = lv_obj_create(bg);
    lv_obj_set_size(box, 220, 140);
    lv_obj_align(box, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(box, lv_color_white(), 0);
    lv_obj_set_style_radius(box, 10, 0);
    lv_obj_clear_flag(box, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t * label = lv_label_create(box);
    lv_label_set_text(label, " 更新成功！\n 是否立刻重启？");
    lv_obj_set_width(label, lv_pct(90));
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 15);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);

    lv_obj_t * btn_restart = lv_btn_create(box);
    lv_obj_set_size(btn_restart, 80, 35);
    lv_obj_align(btn_restart, LV_ALIGN_BOTTOM_LEFT, 10, -10);
    lv_obj_t * lbl_restart = lv_label_create(btn_restart);
    lv_label_set_text(lbl_restart, "重启");
    lv_obj_center(lbl_restart);
    lv_obj_add_event_cb(btn_restart, dialog_restart_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t * btn_later = lv_btn_create(box);
    lv_obj_set_size(btn_later, 80, 35);
    lv_obj_align(btn_later, LV_ALIGN_BOTTOM_RIGHT, -10, -10);
    lv_obj_t * lbl_later = lv_label_create(btn_later);
    lv_label_set_text(lbl_later, "稍后");
    lv_obj_center(lbl_later);
    lv_obj_add_event_cb(btn_later, dialog_later_cb, LV_EVENT_CLICKED, page);
}

static void dialog_restart_cb(lv_event_t * e)
{
    (void)e;
    system("reboot");
}

static void dialog_later_cb(lv_event_t * e)
{
    UpgradePage * page = (UpgradePage *)lv_event_get_user_data(e);
    if (!page) return;

    lv_obj_t * btn = lv_event_get_target(e);
    lv_obj_t * box = lv_obj_get_parent(btn);
    lv_obj_t * bg = lv_obj_get_parent(box);
    if (bg) lv_obj_del(bg);

    page->has_new_version = false;
    update_status(page, "更新已完成，请重启设备\n(≧▽≦)");
    lv_obj_set_style_text_color(page->label_status, lv_color_hex(0x888888), 0);
    lv_obj_add_flag(page->btn_update, LV_OBJ_FLAG_HIDDEN);
    lv_obj_align(page->label_status, LV_ALIGN_CENTER, 0, -7);
    lock_ui(page, false);
}

static void cleanup_tmp(void)
{
    system("rm -rf " TMP_DIR "/*");
}