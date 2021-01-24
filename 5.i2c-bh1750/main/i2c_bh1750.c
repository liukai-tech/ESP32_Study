/*
* @file         i2c_bh1750.c 
* @brief        i2c驱动光照传感器BH1750
* @details      用户应用程序的入口文件,用户所有要实现的功能逻辑均是从该文件开始或者处理
* @author       Caesar
* @par Copyright (c):  
*               Caesar,Email:792910363@qq.com
* @par History:          
*               Ver0.0.1:
                     Caesar, 2019/10/18, 初始化版本\n  
*/

/* 
=============
头文件包含
=============
*/
#include <stdio.h>
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "driver/i2c.h"

/*
===========================
宏定义
=========================== 
*/
#define LEDC_MAX_DUTY         	(8191)	//2的13次方-1(13位ledc)
#define LEDC_FADE_TIME    		(1000)	//渐变时间(ms)

#define LED_R_IO    2
#define LED_G_IO    18
#define LED_B_IO    19

//I2C 
#define I2C_SCL_IO          		33                  //SCL->IO33
#define I2C_SDA_IO          		32                  //SDA->IO32
#define I2C_MASTER_NUM      		I2C_NUM_1           //I2C_1
#define I2C_MASTER_FREQ_HZ  		100000           	//I2C主机时钟频率
#define I2C_MASTER_TX_BUF_DISABLE  	0            		//I2C主机不需要tx buffer
#define I2C_MASTER_RX_BUF_DISABLE  	0            		//I2C主机不需要rx buffer
#define WRITE_BIT           		I2C_MASTER_WRITE    //写:0
#define READ_BIT            		I2C_MASTER_READ     //读:1
#define ACK_CHECK_EN        		0x1                 //主机检查从机的ACK
#define ACK_CHECK_DIS       		0x0                 //主机不检查从机的ACK
#define ACK_VAL             		0x0                 //应答
#define NACK_VAL            		0x1                 //不应答

//BH1750
#define BH1750_SENSOR_ADDR          0x23             	//BH1750 sensor从机地址
#define BH1750_CMD_START            0x23             	//BH1750设置测量模式指令

/*
===========================
全局变量定义
=========================== 
*/
//ledc配置结构体
ledc_channel_config_t 	g_ledc_ch_R,g_ledc_ch_G,g_ledc_ch_B;

/*
===========================
任务句柄
=========================== 
*/
TaskHandle_t led_breathe_task_handle;
TaskHandle_t i2c_sensor_task_handle;

/*
===========================
函数声明
=========================== 
*/
void led_breathe_task();
void i2c_sensor_task();

/*
===========================
函数定义
=========================== 
*/

/*
* void ledc_init(void):定时器0用在PWM模式，输出3通道的LEDC信号
* @param[in]   无
* @retval      无
* @note        修改日志 
*               Ver0.0.1:
                    Caesar, 2019/10/18, 初始化版本\n   
*/
void ledc_init(void)
{
	//定时器配置结构体
	ledc_timer_config_t 	ledc_timer;
	//定时器配置->timer0
	ledc_timer.duty_resolution = LEDC_TIMER_13_BIT; //PWM分辨率
	ledc_timer.freq_hz = 5000;                      //频率
	ledc_timer.speed_mode = LEDC_HIGH_SPEED_MODE;  	//速度
	ledc_timer.timer_num = LEDC_TIMER_0;           	// 选择定时器
	ledc_timer_config(&ledc_timer);					//设置定时器PWM模式

	//PWM通道0配置->IO2->红色灯
	g_ledc_ch_R.channel    = LEDC_CHANNEL_0;		//PWM通道
	g_ledc_ch_R.duty       = 0;						//占空比
	g_ledc_ch_R.gpio_num   = LED_R_IO;			    //IO映射
	g_ledc_ch_R.speed_mode = LEDC_HIGH_SPEED_MODE;	//速度
	g_ledc_ch_R.timer_sel  = LEDC_TIMER_0;			//选择定时器
	ledc_channel_config(&g_ledc_ch_R);				//配置PWM
	
	//PWM通道1配置->IO18->绿色灯
	g_ledc_ch_G.channel    = LEDC_CHANNEL_1;		//PWM通道
	g_ledc_ch_G.duty       = 0;						//占空比
	g_ledc_ch_G.gpio_num   = LED_G_IO;				//IO映射
	g_ledc_ch_G.speed_mode = LEDC_HIGH_SPEED_MODE;	//速度
	g_ledc_ch_G.timer_sel  = LEDC_TIMER_0;			//选择定时器
	ledc_channel_config(&g_ledc_ch_G);				//配置PWM
	
	//PWM通道2配置->IO19->蓝色灯
	g_ledc_ch_B.channel    = LEDC_CHANNEL_2;		//PWM通道
	g_ledc_ch_B.duty       = 0;						//占空比
	g_ledc_ch_B.gpio_num   = LED_B_IO;				//IO映射
	g_ledc_ch_B.speed_mode = LEDC_HIGH_SPEED_MODE;	//速度
	g_ledc_ch_B.timer_sel  = LEDC_TIMER_0;			//选择定时器
	ledc_channel_config(&g_ledc_ch_B);				//配置PWM
	
	//注册LEDC服务:相当于使能
	ledc_fade_func_install(0);
}

/**
 * @brief test code to write esp-i2c-slave
 *
 * 1. set mode
 * _________________________________________________________________
 * | start | slave_addr + wr_bit + ack | write 1 byte + ack  | stop |
 * --------|---------------------------|---------------------|------|
 * 2. wait more than 24 ms
 * 3. read data
 * ______________________________________________________________________________________
 * | start | slave_addr + rd_bit + ack | read 1 byte + ack  | read 1 byte + nack | stop |
 * --------|---------------------------|--------------------|--------------------|------|
 */
static esp_err_t i2c_master_sensor_test(i2c_port_t i2c_num, uint8_t* data_h, uint8_t* data_l)
{
    int ret;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, BH1750_SENSOR_ADDR << 1 | WRITE_BIT, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, BH1750_CMD_START, ACK_CHECK_EN);
    i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    if (ret != ESP_OK) {
        return ret;
    }
    vTaskDelay(30 / portTICK_RATE_MS);
    cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, BH1750_SENSOR_ADDR << 1 | READ_BIT, ACK_CHECK_EN);
    i2c_master_read_byte(cmd, data_h, ACK_VAL);
    i2c_master_read_byte(cmd, data_l, NACK_VAL);
    i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    return ret;
}

/**
 * @brief i2c master initialization
 */
static void i2c_master_init()
{
    int i2c_master_port = I2C_MASTER_NUM;
    i2c_config_t conf;
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = I2C_SDA_IO;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_io_num = I2C_SCL_IO;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = I2C_MASTER_FREQ_HZ;
    i2c_param_config(i2c_master_port, &conf);
    i2c_driver_install(i2c_master_port, conf.mode,
                       I2C_MASTER_RX_BUF_DISABLE,
                       I2C_MASTER_TX_BUF_DISABLE, 0);
}




/*
 * 应用程序的函数入口
 * @param[in]   无
 * @retval      无              
 * @par         修改日志 
 *               Ver0.0.1:
                     Caesar, 2019/10/18, 初始化版本\n   
*/
void app_main()
{    
	//创建led呼吸灯任务
	xTaskCreate(led_breathe_task, "led_breathe_task", 1024*2, NULL, configMAX_PRIORITIES, led_breathe_task_handle);
	//创建i2c驱动BH1750任务
	xTaskCreate(i2c_sensor_task, "i2c_sensor_task", 1024*2, NULL, configMAX_PRIORITIES-1, i2c_sensor_task_handle);
}

/*
* led呼吸灯任务
* @param[in]   无
* @retval      无
* @note        修改日志 
*               Ver0.0.1:
                    Caesar, 2019/10/18, 初始化版本\n    
*/
void led_breathe_task()
{
	ledc_init();
	while(1){
		//ledc 红灯渐变至100%，时间LEDC_FADE_TIME
		ledc_set_fade_with_time(g_ledc_ch_R.speed_mode,
                    g_ledc_ch_R.channel, 
					LEDC_MAX_DUTY,
                    LEDC_FADE_TIME);
		//渐变开始
		ledc_fade_start(g_ledc_ch_R.speed_mode,
				g_ledc_ch_R.channel, 
				LEDC_FADE_NO_WAIT);
		//ledc 绿灯渐变至100%，时间LEDC_FADE_TIME
		ledc_set_fade_with_time(g_ledc_ch_G.speed_mode,
                    g_ledc_ch_G.channel, 
					LEDC_MAX_DUTY,
                    LEDC_FADE_TIME);
		//渐变开始
		ledc_fade_start(g_ledc_ch_G.speed_mode,
				g_ledc_ch_G.channel, 
				LEDC_FADE_NO_WAIT);
		//ledc 蓝灯渐变至100%，时间LEDC_FADE_TIME
		ledc_set_fade_with_time(g_ledc_ch_B.speed_mode,
                    g_ledc_ch_B.channel, 
					LEDC_MAX_DUTY,
                    LEDC_FADE_TIME);
		//渐变开始
		ledc_fade_start(g_ledc_ch_B.speed_mode,
				g_ledc_ch_B.channel, 
				LEDC_FADE_NO_WAIT);
		//延时LEDC_FADE_TIME，给LEDC控制时间
        vTaskDelay(LEDC_FADE_TIME / portTICK_PERIOD_MS);
		

		//ledc 红灯 渐变至0%，时间LEDC_FADE_TIME
		ledc_set_fade_with_time(g_ledc_ch_R.speed_mode,
                    g_ledc_ch_R.channel, 
					0,
                    LEDC_FADE_TIME);
		//渐变开始
		ledc_fade_start(g_ledc_ch_R.speed_mode,
				g_ledc_ch_R.channel, 
				LEDC_FADE_NO_WAIT);
		//ledc 绿灯渐变至0%，时间LEDC_FADE_TIME
		ledc_set_fade_with_time(g_ledc_ch_G.speed_mode,
                    g_ledc_ch_G.channel, 
					0,
                    LEDC_FADE_TIME);
		//渐变开始
		ledc_fade_start(g_ledc_ch_G.speed_mode,
				g_ledc_ch_G.channel, 
				LEDC_FADE_NO_WAIT);
		//ledc 蓝灯渐变至0%，时间LEDC_FADE_TIME
		ledc_set_fade_with_time(g_ledc_ch_B.speed_mode,
                    g_ledc_ch_B.channel, 
					0,
                    LEDC_FADE_TIME);
		//渐变开始
		ledc_fade_start(g_ledc_ch_B.speed_mode,
				g_ledc_ch_B.channel, 
				LEDC_FADE_NO_WAIT);
		//延时LEDC_FADE_TIME，给LEDC控制时间
        vTaskDelay(LEDC_FADE_TIME / portTICK_PERIOD_MS);
	}
}

/*
* i2c驱动bh1750任务
* @param[in]   无
* @retval      无
* @note        修改日志 
*               Ver0.0.1:
                    Caesar, 2019/10/18, 初始化版本\n  
*/
void i2c_sensor_task()
{
	int ret;
    uint8_t sensor_data_h, sensor_data_l;
	i2c_master_init();
	while(1){
		ret = i2c_master_sensor_test( I2C_MASTER_NUM, &sensor_data_h, &sensor_data_l);
        if(ret == ESP_ERR_TIMEOUT) {
            printf("I2C timeout\n");
        } else if(ret == ESP_OK) {
            printf("*******************\n");
            printf("MASTER READ SENSOR( BH1750 )\n");
            printf("*******************\n");
            printf("data_h: %02x\n", sensor_data_h);
            printf("data_l: %02x\n", sensor_data_l);
            printf("sensor val: %.2f\n", (sensor_data_h << 8 | sensor_data_l) / 1.2);
        } else {
            printf("No ack, sensor not connected...skip...\n");
        }
		vTaskDelay(500 / portTICK_RATE_MS);
	}
}
