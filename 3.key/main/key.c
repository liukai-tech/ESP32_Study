/*
* @file         key.c 
* @brief        ESP32 GPIO输入检测按键
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

#include "esp_log.h"
#include "soc/timer_group_struct.h"
#include "driver/periph_ctrl.h"
#include "driver/timer.h"
#include "esp_timer.h"

/*
===========================
宏定义(LED连接为共阳极，低电平点亮)
=========================== 
*/
#define LED_R_IO 		2   
#define LED_G_IO 		18
#define LED_B_IO 		19
#define LED_USER_IO     5
#define LED_USER2_IO    4

#define KEY_IO          34

/*
===========================
任务句柄
=========================== 
*/
TaskHandle_t led_flow_task_handle;
TaskHandle_t led_toggle_task_handle;

//定时器句柄
esp_timer_handle_t fw_timer_handle = 0;

/*
===========================
函数声明
=========================== 
*/
void led_flow_task();
void led_toggle_task();

/*
===========================
全局变量声明
=========================== 
*/
unsigned char led_user_status = 0;
unsigned char key_status[2];

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
    gpio_pad_select_gpio(LED_USER2_IO);
    //设置IO为输出
    gpio_set_direction(LED_R_IO, GPIO_MODE_OUTPUT);
    gpio_set_direction(LED_G_IO, GPIO_MODE_OUTPUT);
    gpio_set_direction(LED_B_IO, GPIO_MODE_OUTPUT);
    gpio_set_direction(LED_USER_IO, GPIO_MODE_OUTPUT);
    gpio_set_direction(LED_USER2_IO, GPIO_MODE_OUTPUT);
}

/*
* esp32 key配置
* @param[in]   无
* @retval      无
* @note        修改日志 
*               Ver0.0.1:
                    Caesar, 2019/10/17, 初始化版本\n 
*/
void key_init(void)
{
	//选择IO
    gpio_pad_select_gpio(KEY_IO);
    //设置IO为输入
    gpio_set_direction(KEY_IO, GPIO_MODE_INPUT);
}

void key_read(void)
{
    if(gpio_get_level(KEY_IO)==0)//按键按下
    {
        //等待松手，最傻的办法
        while(gpio_get_level(KEY_IO)==0);
        if (led_user_status==1)
        {
            led_user_status = 0;
            gpio_set_level(LED_USER_IO, 0);//不亮
        }
        else
        {
            led_user_status = 1;
            gpio_set_level(LED_USER_IO, 1);//亮
        }

    }
}

void key_read_filter(void)
{
    //按键识别
    if(gpio_get_level(KEY_IO)==0){
        key_status[0] = 0;
    }
    else{
       key_status[0] = 1; 
    }
    if(key_status[0]!=key_status[1]) {
        key_status[1] = key_status[0];
        if(key_status[1]==0){//按键按下
/*            if (led_user_status==1){
                led_user_status = 0;
                gpio_set_level(LED_USER_IO, 0);//不亮
            }else{
                led_user_status = 1;
                gpio_set_level(LED_USER_IO, 1);//亮
            }
*/
        }else{//按键松手
            if (led_user_status==1){
                led_user_status = 0;
                gpio_set_level(LED_USER_IO, 0);//不亮
            }else{
                led_user_status = 1;
                gpio_set_level(LED_USER_IO, 1);//亮
            }
        }
    }
}

void fw_timer_cb(void *arg) 
{
	//获取时间戳
	int64_t tick = esp_timer_get_time();
	printf("timer cnt = %lld \r\n", tick);
/*
	if (tick > 50000000) //50秒结束
	{
		//定时器暂停、删除
		esp_timer_stop(fw_timer_handle);
		esp_timer_delete(fw_timer_handle);
		printf("timer stop and delete!!! \r\n");
		//重启
		esp_restart();
	}
*/
	gpio_set_level(LED_USER2_IO, 0);
	vTaskDelay(300 / portTICK_PERIOD_MS);
	gpio_set_level(LED_USER2_IO, 1);
	vTaskDelay(300 / portTICK_PERIOD_MS);
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
    //key初始化
    key_init();

    //定时器结构体初始化
	esp_timer_create_args_t fw_timer = 
	{ 
		.callback = &fw_timer_cb, 	//回调函数
		.arg = NULL, 				//参数
		.name = "fw_timer" 			//定时器名称
	};

	//定时器创建、启动
	esp_err_t err = esp_timer_create(&fw_timer, &fw_timer_handle);
	err = esp_timer_start_periodic(fw_timer_handle, 1000 * 1000);//1秒回调
	if(err == ESP_OK)
	{
		printf("fw timer create and start ok!\r\n");
	}

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
 /*       gpio_set_level(LED_USER_IO, 1);
        vTaskDelay(500 / portTICK_PERIOD_MS);
        gpio_set_level(LED_USER_IO, 0);
        vTaskDelay(500 / portTICK_PERIOD_MS);
        */
        key_read_filter();
        vTaskDelay(10 / portTICK_PERIOD_MS);
    } 
}