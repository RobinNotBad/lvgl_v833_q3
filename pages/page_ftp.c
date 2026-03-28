#include "page_ftp.h"

#include "main.h"

#define VSFTPD_EXE "../ftp/vsftpd"
#define VSFTPD_CONF "../ftp/vsftpd.conf"

static void back_click(lv_event_t * e);
static void btn_start_click(lv_event_t * e);
static void btn_stop_click(lv_event_t * e);
static void refresh_text(lv_obj_t * label);

lv_obj_t * page_ftp(void)
{
    lv_obj_t * screen = lv_obj_create(lv_scr_act());
    lv_obj_remove_style_all(screen);
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(screen, lv_pct(100), lv_pct(100));

    lv_obj_t * label_ip = lv_label_create(screen);
    lv_obj_align(label_ip, LV_ALIGN_TOP_MID, 0, lv_pct(80));
    refresh_text(label_ip);

    lv_obj_t * btn_start = lv_btn_create(screen);
    lv_obj_set_size(btn_start, lv_pct(60), lv_pct(25));
    lv_obj_align(btn_start, LV_ALIGN_TOP_MID, 0, lv_pct(20));
    lv_obj_t * btn_start_label = lv_label_create(btn_start);
    lv_label_set_text(btn_start_label, "start vsftpd");
    lv_obj_center(btn_start_label);
    lv_obj_add_event_cb(btn_start, btn_start_click, LV_EVENT_CLICKED, label_ip);

    lv_obj_t * btn_stop = lv_btn_create(screen);
    lv_obj_set_size(btn_stop, lv_pct(60), lv_pct(25));
    lv_obj_align(btn_stop, LV_ALIGN_TOP_MID, 0, lv_pct(50));
    lv_obj_t * btn_stop_label = lv_label_create(btn_stop);
    lv_label_set_text(btn_stop_label, "kill vsftpd");
    lv_obj_center(btn_stop_label);
    lv_obj_add_event_cb(btn_stop, btn_stop_click, LV_EVENT_CLICKED, label_ip);
    
    lv_obj_t * btn_back = lv_btn_create(screen);
    lv_obj_set_size(btn_back, lv_pct(25), lv_pct(12));
    lv_obj_align(btn_back, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_t * btn_back_label = lv_label_create(btn_back);
    lv_label_set_text(btn_back_label, CUSTOM_SYMBOL_BACK "");
    lv_obj_center(btn_back_label);
    lv_obj_add_event_cb(btn_back, back_click, LV_EVENT_CLICKED, NULL);

    return screen;
}

static void back_click(lv_event_t * e)
{
    page_back();
}

static void btn_start_click(lv_event_t * e)
{
    system("chmod 777 " VSFTPD_EXE);

    pid_t cpid = fork();

    if(cpid == 0) {
        //此处为子进程
        daemon(1, 0);
        close(fbd);
        close(dispd);
        close(powerd);
        close(homed);

        char * argv[]     = {VSFTPD_EXE, VSFTPD_CONF, NULL};
        execv(VSFTPD_EXE, argv);
        
        exit(127);  //防止意外执行失败
    }

    usleep(1000);
    refresh_text((lv_obj_t *)e->user_data);
}

static void btn_stop_click(lv_event_t * e)
{
    system("killall vsftpd &");
    usleep(1000);
    refresh_text((lv_obj_t *)e->user_data);
}

static void refresh_text(lv_obj_t * label)
{
    char buffer_pidof[32];
    FILE * fp_pidof = popen("pidof vsftpd", "r");
    int is_running  = (fgets(buffer_pidof, sizeof(buffer_pidof), fp_pidof) != NULL);
    pclose(fp_pidof);

    if(is_running) {
        FILE * fp_ifconfig;
        char buffer_ifconfig[512];
        char ip[20] = "";

        // 执行ifconfig命令
        fp_ifconfig = popen("ifconfig", "r");
        if(fp_ifconfig == NULL) {
            perror("popen failed");
        } else {
            // 解析输出，查找IP地址
            while(fgets(buffer_ifconfig, sizeof(buffer_ifconfig), fp_ifconfig) != NULL) {
                // 查找包含"inet"的行（IPv4地址）
                if(strstr(buffer_ifconfig, "inet addr:") != NULL && strstr(buffer_ifconfig, "127.0.0.1") == NULL) {
                    char * ip_start = strstr(buffer_ifconfig, "inet addr:") + 10;
                    sscanf(ip_start, "%s", ip);
                    break;
                }
            }
            pclose(fp_ifconfig);

            if(strlen(ip) == 0)
                lv_label_set_text(label, "No Connection");
            else
                lv_label_set_text(label, ip);
        }
    }
    else {
        lv_label_set_text(label, "Not Running");
    }

}
