/*
* @file         i2c_ssd1306.c 
* @brief        ESP32操作OLED-I2C
* @details      用户应用程序的入口文件,用户所有要实现的功能逻辑均是从该文件开始或者处理
* @author       Caesar 
* @par Copyright (c):  
*               Caesar,Email:792910363@qq.com
* @par History:          
*               Ver0.0.1:
*                     Caesar, 2019/10/18, 初始化版本\n  
*/
#include <stdio.h>
#include "esp_system.h"
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include "freertos/task.h"
#include "ssd1306.h"
#include "fonts.h"

void app_main()
{
	char pbuf[20];
	uint8_t len = 0;
    unsigned int cnt=0;
    SSD1306_Init();
    SSD1306_DrawStr(0,0,  "ESP32 I2C Demo", &Font_7x10, 1);
    SSD1306_DrawStr(0,15, "ssd1306 example", &Font_7x10, 1);
    SSD1306_DrawStr(0,30, "Hello World!", &Font_7x10, 1);
    SSD1306_DrawStr(0,45, "Powered by Caesar.",&Font_7x10,1);
    vTaskDelay(10000 / portTICK_PERIOD_MS);
	SSD1306_Clear();
	SSD1306_DrawLine(0,0,100,60,1);
	SSD1306_DrawRectangle(5,5,60,45,1);
	SSD1306_DrawTriangle(20,5, 7,34, 40,40,1);
	SSD1306_DrawCircle(50,30,20,1);
	vTaskDelay(10000 / portTICK_PERIOD_MS);
	SSD1306_Clear();
    while(1)
    {   
        len = sprintf(pbuf,"%04d",cnt % 10000);
		pbuf[len] = '\0';
		SSD1306_DrawStr(20,0,pbuf,&Font_7x10,1);//显示0000-9999(3种字体大小)
		SSD1306_DrawStr(20,15,pbuf,&Font_11x18,1);
		SSD1306_DrawStr(20,34,pbuf,&Font_16x26,1);
		cnt++;
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        ESP_LOGI("OLED", "cnt = %d \r\n", cnt);
    }
}