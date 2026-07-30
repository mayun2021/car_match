/**
 * @file main.c
 * @brief 程序入口文件。
 *
 * 上电后先初始化硬件抽象层和应用层，然后在无限循环中持续调用机器人主任务。
 */

#include "hal.h"
#include "robot_app.h"

/**
 * @brief 主函数。
 *
 * @return 嵌入式程序正常不会返回。
 */
int main(void)
{
    hal_init();
    robot_app_init();

    while (1)
    {
        robot_app_tick(hal_millis());
    }
}
