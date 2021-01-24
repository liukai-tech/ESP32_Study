/*
* @file         led.c 
* @brief        LED流水灯
* @details      用户应用程序的入口文件,用户所有要实现的功能逻辑均是从该文件开始或者处理
* @author       Caesar 
* @par Copyright (c):  
*               Caesar,Email:792910363@qq.com
* @par History:          
*               Ver0.0.1:
*                     Caesar, 2019/10/17, 初始化版本\n 
*/

/* 
=============
头文件包含
=============
*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "sdkconfig.h"

/*
===========================
宏定义(LED连接为共阳极，低电平点亮)
=========================== 
*/
#define LED_R_IO 		2   
#define LED_G_IO 		18
#define LED_B_IO 		19
#define LED_USER_IO     5

/*
===========================
任务句柄
=========================== 
*/
TaskHandle_t led_flow_task_handle;
TaskHandle_t led_toggle_task_handle;

/*
===========================
函数声明
=========================== 
*/
void led_flow_task();
void led_toggle_task();

/*
* esp32 led配置
* @param[in]   无
* @retval      无
* @note        修改日志 
*               Ver0.0.1:
                    Caesar, 2019/10/17, 初始化版本\n
*/
void led_init(void)
{
	//选择IO
    gpio_pad_select_gpio(LED_R_IO);
    gpio_pad_select_gpio(LED_G_IO);
    gpio_pad_select_gpio(LED_B_IO);
    gpio_pad_select_gpio(LED_USER_IO);
    //设置IO为输出
    gpio_set_direction(LED_R_IO, GPIO_MODE_OUTPUT);
    gpio_set_direction(LED_G_IO, GPIO_MODE_OUTPUT);
    gpio_set_direction(LED_B_IO, GPIO_MODE_OUTPUT);
    gpio_set_direction(LED_USER_IO, GPIO_MODE_OUTPUT);
}



/*
 * 应用程序的函数入口
 * @param[in]   无
 * @retval      无              
 * @par         修改日志 
 *               Ver0.0.1:
                     Caesar, 2019/10/17, 初始化版本\n
*/
void app_main()
{
    //led初始化
	led_init();
	//创建led流水灯任务
	xTaskCreate(led_flow_task, "led_flow_task", 1024*2, NULL, configMAX_PRIORITIES, led_flow_task_handle);
	//创建led翻转任务
	xTaskCreate(led_toggle_task, "led_toggle_task", 1024*2, NULL, configMAX_PRIORITIES-1, led_toggle_task_handle);
}


/*
* led流水灯任务
* @param[in]   无
* @retval      无
* @note        修改日志 
*               Ver0.0.1:
                    Caesar, 2019/10/17, 初始化版本\n 
*/
void led_flow_task()
{
    static uint8_t led_state = 1;
    while (1) {
        switch(led_state)
        {
            case 1:
                //只点亮红灯
                gpio_set_level(LED_R_IO, 0);
                gpio_set_level(LED_G_IO, 1);
                gpio_set_level(LED_B_IO, 1);
                break;
            case 2:
                //只点亮绿灯
                gpio_set_level(LED_R_IO, 1);
                gpio_set_level(LED_G_IO, 0);
                gpio_set_level(LED_B_IO, 1);
                break;
            case 3:
                //只点亮蓝灯
                gpio_set_level(LED_R_IO, 1);
                gpio_set_level(LED_G_IO, 1);
                gpio_set_level(LED_B_IO, 0);
                break;
            default:
                break;
        }
        if(led_state < 3){
            led_state ++;
        }
        else{
            led_state = 1;
        }
        vTaskDelay(100 / portTICK_PERIOD_MS);
    } 
}

/*
* led翻转任务
* @param[in]   无
* @retval      无
* @note        修改日志 
*               Ver0.0.1:
                    Caesar, 2019/10/17, 初始化版本\n  
*/
void led_toggle_task()
{
    while (1) {
        gpio_set_level(LED_USER_IO, 1);
        vTaskDelay(500 / portTICK_PERIOD_MS);
        gpio_set_level(LED_USER_IO, 0);
        vTaskDelay(500 / portTICK_PERIOD_MS);
    } 
}