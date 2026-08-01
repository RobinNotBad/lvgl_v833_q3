
#include "sys_robot.h"

#include "main.h"
#include "hw_keys.h"
#include "hw_screen.h"

/**
 * 干掉robot
 */
void kill_robot(void)
{
    printf("kill robot\n");
    system("killall robotd");
    system("killall robot_run");
    system("killall robot_run_1");
    usleep(100000);
}

/**
 * 切换到robot程序
 */
void switch_robot(void)
{
    switch_background();

    // 现在不需要杀vsftpd了
    system("chmod 777 ./switch_robot");
    system("sh ./switch_robot");
}

/**
 * 进入后台
 */
void switch_background(void)
{
    if(ts_background != -1) return;
    ts_background = tick_get();
    ts_sleep      = -1;
    
    lcd_close();
    key_close_power();
    usleep(100000);
}

/**
 * 从robot切换回来
 */
void switch_foreground(void)
{
    if(ts_background == -1) return;

    chdir(homepath);
    system("chmod 777 switch_foreground");
    system("sh ./switch_foreground &");
    // 等待自己被脚本杀死，然后开始新的轮回
    // 因为这里确实处理不好设备占用问题，只能把两个全杀了再重启自己
    sleep(114514);
}
