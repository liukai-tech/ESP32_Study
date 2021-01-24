/*
* @file         ssd1306.c 
* @brief        ESP32操作OLED-I2C
* @details      用户应用程序的入口文件,用户所有要实现的功能逻辑均是从该文件开始或者处理
* @author       Caesar, 2019/10/18, 初始化版本\n  
* @par Copyright (c):  
*               Caesar,Email:792910363@qq.com
*/
/* 
=============
头文件包含
=============
*/
#include "ssd1306.h"
#include "string.h"
#include "stdlib.h"
#include "fonts.h"
/*
===========================
全局变量定义
=========================== 
*/
//OLED缓存128*64bit
static uint8_t g_oled_buffer[SSD1306_WIDTH * SSD1306_HEIGHT / 8];
//OLED实时信息
static SSD1306_t oled;
//OLED是否正在显示，1显示，0等待
static bool is_show_str =0;

/* Absolute value */
#define ABS(x)   ((x) > 0 ? (x) : -(x))

/*
===========================
函数定义
=========================== 
*/

/** 
 * oled_i2c 初始化
 * @param[in]   无
 * @retval      
 *              无                              
 * @par         修改日志 
 *               Ver0.0.1:
                     Caesar, 2019/10/18, 初始化版本\n 
 */
static void i2c_init(void)
{
    //注释参考sht30之i2c教程
    i2c_config_t conf;
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = I2C_OLED_MASTER_SDA_IO;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_io_num = I2C_OLED_MASTER_SCL_IO;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = 400000;
    i2c_param_config(I2C_OLED_MASTER_NUM, &conf);
    i2c_driver_install(I2C_OLED_MASTER_NUM, conf.mode,0, 0, 0);
}

/** 
 * 向oled写命令
 * @param[in]   command
 * @retval      
 *              - ESP_OK                              
 * @par         修改日志 
 *               Ver0.0.1:
                     Caesar, 2019/10/18, 初始化版本\n 
 */

static int oled_write_cmd(uint8_t command)
{
    //注释参考sht30之i2c教程
    int ret;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    ret = i2c_master_start(cmd);
    ret = i2c_master_write_byte(cmd, OLED_WRITE_ADDR |WRITE_BIT , ACK_CHECK_EN); 
    ret = i2c_master_write_byte(cmd, WRITE_CMD, ACK_CHECK_EN);
    ret = i2c_master_write_byte(cmd,command, ACK_CHECK_EN);
    ret = i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin(I2C_OLED_MASTER_NUM, cmd, 100 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    if (ret != ESP_OK) 
    {
        return ret;
    }
    return ret;
}

/** 
 * 向oled写数据
 * @param[in]   data
 * @retval      
 *              - ESP_OK                              
 * @par         修改日志 
 *               Ver0.0.1:
                     Caesar, 2019/10/18, 初始化版本\n  
 */
static int oled_write_data(uint8_t data)
{
    //注释参考sht30之i2c教程
    int ret;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    ret = i2c_master_start(cmd);
    ret = i2c_master_write_byte(cmd, OLED_WRITE_ADDR | WRITE_BIT, ACK_CHECK_EN);
    ret = i2c_master_write_byte(cmd, WRITE_DATA, ACK_CHECK_EN);
    ret = i2c_master_write_byte(cmd, data, ACK_CHECK_EN);
    ret = i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin(I2C_OLED_MASTER_NUM, cmd, 100 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    if (ret != ESP_OK) 
    {
        return ret;
    }
    return ret;
}
/** 
 * 向oled写长数据
 * @param[in]   data   要写入的数据
 * @param[in]   len     数据长度
 * @retval      
 *              - ESP_OK                              
 * @par         修改日志 
 *               Ver0.0.1:
                     Caesar, 2019/10/18, 初始化版本\n  
 */
static int oled_write_long_data(uint8_t *data,uint16_t len)
{
    //注释参考sht30之i2c教程
    int ret;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    ret = i2c_master_start(cmd);
    ret = i2c_master_write_byte(cmd, OLED_WRITE_ADDR | WRITE_BIT, ACK_CHECK_EN);
    ret = i2c_master_write_byte(cmd, WRITE_DATA, ACK_CHECK_EN);
    ret = i2c_master_write(cmd, data, len,ACK_CHECK_EN);
    ret = i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin(I2C_OLED_MASTER_NUM, cmd, 10000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    if (ret != ESP_OK) 
    {
        return ret;
    }
    return ret;    
}

/** 
 * 初始化 oled
 * @param[in]   NULL
 * @retval      
 *              NULL                            
 * @par         修改日志 
 *               Ver0.0.1:
                     Caesar, 2019/10/18, 初始化版本\n 
 */
void SSD1306_Init(void)
{
    //i2c初始化
    i2c_init();
    //oled配置
    oled_write_cmd(TURN_OFF_CMD);
    oled_write_cmd(0xAE);//关显示
    oled_write_cmd(0X20);//低列地址
    oled_write_cmd(0X10);//高列地址
    oled_write_cmd(0XB0);//
    oled_write_cmd(0XC8);
    oled_write_cmd(0X00);
    oled_write_cmd(0X10);
     //设置行显示的开始地址(0-63)  
    //40-47: (01xxxxx)  
    oled_write_cmd(0X40);
     //设置对比度  
    oled_write_cmd(0X81);
    oled_write_cmd(0XFF);//这个值越大，屏幕越亮(和上条指令一起使用)(0x00-0xff) 

    oled_write_cmd(0XA1);//0xA1: 左右反置，  0xA0: 正常显示（默认0xA0）
   //0xA6: 表示正常显示（在面板上1表示点亮，0表示不亮）  
    //0xA7: 表示逆显示（在面板上0表示点亮，1表示不亮）
    oled_write_cmd(0XA6); 

    oled_write_cmd(0XA8);//设置多路复用率（1-64） 
    oled_write_cmd(0X3F);//（0x01-0x3f）(默认为3f)
    oled_write_cmd(0XA4);
    //设置显示抵消移位映射内存计数器  
    oled_write_cmd(0XD3);
    oled_write_cmd(0X00);
    //设置显示时钟分频因子/振荡器频率 
    oled_write_cmd(0XD5);
    //低4位定义显示时钟(屏幕的刷新时间)（默认：0000）分频因子= [3:0]+1  
    //高4位定义振荡器频率（默认：1000） 
    oled_write_cmd(0XF0);
    //时钟预充电周期  
    oled_write_cmd(0XD9);
    oled_write_cmd(0X22);
    //设置COM硬件应脚配置  
    oled_write_cmd(0XDA);
    oled_write_cmd(0X12);
    oled_write_cmd(0XDB);
    oled_write_cmd(0X20);
    //电荷泵设置（初始化时必须打开，否则看不到显示）
    oled_write_cmd(0X8D);
    oled_write_cmd(0X14);
    //开显示
    oled_write_cmd(0XAF);
    //清屏
    SSD1306_Clear();
}

/** 
 * 将显存内容刷新到oled显示区
 * @param[in]   NULL
 * @retval      
 *              NULL                           
 * @par         修改日志 
 *               Ver0.0.1:
                     Caesar, 2019/10/18, 初始化版本\n 
 */
void SSD1306_UpdateScreen(void)
{
    uint8_t line_index;
    for(line_index = 0; line_index < 8; line_index ++)
    {
        oled_write_cmd(0xb0+line_index);
        oled_write_cmd(0x00);
        oled_write_cmd(0x10);
        
        oled_write_long_data(&g_oled_buffer[SSD1306_WIDTH * line_index],SSD1306_WIDTH);
    }
}

/** 
 * 清屏
 * @param[in]   NULL
 * @retval      
 *              NULL                            
 * @par         修改日志 
 *               Ver0.0.1:
                     Caesar, 2019/10/18, 初始化版本\n  
 */
void SSD1306_Clear(void)
{
    //清0缓存
    memset(g_oled_buffer,SSD1306_COLOR_BLACK,sizeof(g_oled_buffer));
    SSD1306_UpdateScreen();
}
/** 
 * 填屏
 * @param[in]   NULL
 * @retval      
 *              NULL                            
 * @par         修改日志 
 *               Ver0.0.1:
                     Caesar, 2019/10/18, 初始化版本\n 
 */
void SSD1306_All_On(void)
{
    //置ff缓存
    memset(g_oled_buffer,0xff,sizeof(g_oled_buffer));
    SSD1306_UpdateScreen();
}
/** 
 * 移动坐标
 * @param[in]   x   显示区坐标 x
 * @param[in]   y   显示去坐标 y
 * @retval      
 *              其它                         
 * @par         修改日志 
 *               Ver0.0.1:
                     Caesar, 2019/10/18, 初始化版本\n 
 */
void SSD1306_GotoXY(uint16_t x, uint16_t y) 
{
	oled.CurrentX = x;
	oled.CurrentY = y;
}
/** 
 * 向显存写入
 * @param[in]   x   坐标
 * @param[in]   y   坐标
 * @param[in]   color   色值0/1
 * @retval      
 *              - ESP_OK                              
 * @par         修改日志 
 *               Ver0.0.1:
                     Caesar, 2019/10/18, 初始化版本\n 
 */
void SSD1306_DrawPixel(uint16_t x, uint16_t y, SSD1306_COLOR_t color) 
{
	if (
		x >= SSD1306_WIDTH ||
		y >= SSD1306_HEIGHT
	) 
    {
		return;
	}
	if (color == SSD1306_COLOR_WHITE) 
	{
		g_oled_buffer[x + (y / 8) * SSD1306_WIDTH] |= 1 << (y % 8);
	} 
    else
    {
		g_oled_buffer[x + (y / 8) * SSD1306_WIDTH] &= ~(1 << (y % 8));
	}
}
/** 
 * 在x，y位置显示字符
 * @param[in]   x    显示坐标x 
 * @param[in]   y    显示坐标y 
 * @param[in]   ch   要显示的字符
 * @param[in]   font 显示的字形
 * @param[in]   color 颜色  1显示 0不显示
 * @retval      
 *              其它                        
 * @par         修改日志 
 *               Ver0.0.1:
                     XinC_Guo, 2018/07/18, 初始化版本\n 
 */
char SSD1306_DrawChar(uint16_t x,uint16_t y,char ch, FontDef_t* Font, SSD1306_COLOR_t color) 
{
	uint32_t i, b, j;
	if ( SSD1306_WIDTH <= (oled.CurrentX + Font->FontWidth) || SSD1306_HEIGHT <= (oled.CurrentY + Font->FontHeight) ) 
    {
		return 0;
	}
	if(0 == is_show_str)
    {
        SSD1306_GotoXY(x,y);
    }

	for (i = 0; i < Font->FontHeight; i++) 
    {
		b = Font->data[(ch - 32) * Font->FontHeight + i];
		for (j = 0; j < Font->FontWidth; j++)
        {
			if ((b << j) & 0x8000) 
            {
				SSD1306_DrawPixel(oled.CurrentX + j, (oled.CurrentY + i), (SSD1306_COLOR_t) color);
			} 
            else 
            {
				SSD1306_DrawPixel(oled.CurrentX + j, (oled.CurrentY + i), (SSD1306_COLOR_t)!color);
			}
		}
	}
	oled.CurrentX += Font->FontWidth;
	if(0 == is_show_str)
    {
       SSD1306_UpdateScreen(); 
    }
	return ch;
}
/** 
 * 在x，y位置显示字符串 
 * @param[in]   x    显示坐标x 
 * @param[in]   y    显示坐标y 
 * @param[in]   str   要显示的字符串
 * @param[in]   font 显示的字形
 * @param[in]   color 颜色  1显示 0不显示
 * @retval      
 *              其它                        
 * @par         修改日志 
 *               Ver0.0.1:
                     XinC_Guo, 2018/07/18, 初始化版本\n 
 */
char SSD1306_DrawStr(uint16_t x,uint16_t y, char* str, FontDef_t* Font, SSD1306_COLOR_t color) 
{
    is_show_str=1;
    SSD1306_GotoXY(x,y);
	while (*str) 
    {
		if (SSD1306_DrawChar(x,y,*str, Font, color) != *str) 
        {
            is_show_str=0;
			return *str;
		}
		str++;
	}
    is_show_str=0;
    SSD1306_UpdateScreen();
	return *str;
}

void SSD1306_DrawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, SSD1306_COLOR_t c) {
	int16_t dx, dy, sx, sy, err, e2, i, tmp; 
	
	/* Check for overflow */
	if (x0 >= SSD1306_WIDTH) {
		x0 = SSD1306_WIDTH - 1;
	}
	if (x1 >= SSD1306_WIDTH) {
		x1 = SSD1306_WIDTH - 1;
	}
	if (y0 >= SSD1306_HEIGHT) {
		y0 = SSD1306_HEIGHT - 1;
	}
	if (y1 >= SSD1306_HEIGHT) {
		y1 = SSD1306_HEIGHT - 1;
	}
	
	dx = (x0 < x1) ? (x1 - x0) : (x0 - x1); 
	dy = (y0 < y1) ? (y1 - y0) : (y0 - y1); 
	sx = (x0 < x1) ? 1 : -1; 
	sy = (y0 < y1) ? 1 : -1; 
	err = ((dx > dy) ? dx : -dy) / 2; 

	if (dx == 0) {
		if (y1 < y0) {
			tmp = y1;
			y1 = y0;
			y0 = tmp;
		}
		
		if (x1 < x0) {
			tmp = x1;
			x1 = x0;
			x0 = tmp;
		}
		
		/* Vertical line */
		for (i = y0; i <= y1; i++) {
			SSD1306_DrawPixel(x0, i, c);
		}
		
		/* Return from function */
		return;
	}
	
	if (dy == 0) {
		if (y1 < y0) {
			tmp = y1;
			y1 = y0;
			y0 = tmp;
		}
		
		if (x1 < x0) {
			tmp = x1;
			x1 = x0;
			x0 = tmp;
		}
		
		/* Horizontal line */
		for (i = x0; i <= x1; i++) {
			SSD1306_DrawPixel(i, y0, c);
		}
		
		/* Return from function */
		return;
	}
	
	while (1) {
		SSD1306_DrawPixel(x0, y0, c); 
		if (x0 == x1 && y0 == y1) {
			break;
		}
		e2 = err; 
		if (e2 > -dx) {
			err -= dy;
			x0 += sx;
		} 
		if (e2 < dy) {
			err += dx;
			y0 += sy;
		} 
	}
    if(0 == is_show_str)
    {
       SSD1306_UpdateScreen(); 
    }
}

void SSD1306_DrawRectangle(uint16_t x, uint16_t y, uint16_t w, uint16_t h, SSD1306_COLOR_t c) {
	/* Check input parameters */
	if (
		x >= SSD1306_WIDTH ||
		y >= SSD1306_HEIGHT
	) {
		/* Return error */
		return;
	}
	
	/* Check width and height */
	if ((x + w) >= SSD1306_WIDTH) {
		w = SSD1306_WIDTH - x;
	}
	if ((y + h) >= SSD1306_HEIGHT) {
		h = SSD1306_HEIGHT - y;
	}
	
	/* Draw 4 lines */
	SSD1306_DrawLine(x, y, x + w, y, c);         /* Top line */
	SSD1306_DrawLine(x, y + h, x + w, y + h, c); /* Bottom line */
	SSD1306_DrawLine(x, y, x, y + h, c);         /* Left line */
	SSD1306_DrawLine(x + w, y, x + w, y + h, c); /* Right line */

    if(0 == is_show_str)
    {
       SSD1306_UpdateScreen(); 
    }
}

void SSD1306_DrawFilledRectangle(uint16_t x, uint16_t y, uint16_t w, uint16_t h, SSD1306_COLOR_t c) {
	uint8_t i;
	
	/* Check input parameters */
	if (
		x >= SSD1306_WIDTH ||
		y >= SSD1306_HEIGHT
	) {
		/* Return error */
		return;
	}
	
	/* Check width and height */
	if ((x + w) >= SSD1306_WIDTH) {
		w = SSD1306_WIDTH - x;
	}
	if ((y + h) >= SSD1306_HEIGHT) {
		h = SSD1306_HEIGHT - y;
	}
	
	/* Draw lines */
	for (i = 0; i <= h; i++) {
		/* Draw lines */
		SSD1306_DrawLine(x, y + i, x + w, y + i, c);
	}

    if(0 == is_show_str)
    {
       SSD1306_UpdateScreen(); 
    }
}

void SSD1306_DrawTriangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3, SSD1306_COLOR_t color) {
	/* Draw lines */
	SSD1306_DrawLine(x1, y1, x2, y2, color);
	SSD1306_DrawLine(x2, y2, x3, y3, color);
	SSD1306_DrawLine(x3, y3, x1, y1, color);

    if(0 == is_show_str)
    {
       SSD1306_UpdateScreen(); 
    }
}


void SSD1306_DrawFilledTriangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3, SSD1306_COLOR_t color) {
	int16_t deltax = 0, deltay = 0, x = 0, y = 0, xinc1 = 0, xinc2 = 0, 
	yinc1 = 0, yinc2 = 0, den = 0, num = 0, numadd = 0, numpixels = 0, 
	curpixel = 0;
	
	deltax = ABS(x2 - x1);
	deltay = ABS(y2 - y1);
	x = x1;
	y = y1;

	if (x2 >= x1) {
		xinc1 = 1;
		xinc2 = 1;
	} else {
		xinc1 = -1;
		xinc2 = -1;
	}

	if (y2 >= y1) {
		yinc1 = 1;
		yinc2 = 1;
	} else {
		yinc1 = -1;
		yinc2 = -1;
	}

	if (deltax >= deltay){
		xinc1 = 0;
		yinc2 = 0;
		den = deltax;
		num = deltax / 2;
		numadd = deltay;
		numpixels = deltax;
	} else {
		xinc2 = 0;
		yinc1 = 0;
		den = deltay;
		num = deltay / 2;
		numadd = deltax;
		numpixels = deltay;
	}

	for (curpixel = 0; curpixel <= numpixels; curpixel++) {
		SSD1306_DrawLine(x, y, x3, y3, color);

		num += numadd;
		if (num >= den) {
			num -= den;
			x += xinc1;
			y += yinc1;
		}
		x += xinc2;
		y += yinc2;
	}

    if(0 == is_show_str)
    {
       SSD1306_UpdateScreen(); 
    }
}

void SSD1306_DrawCircle(int16_t x0, int16_t y0, int16_t r, SSD1306_COLOR_t c) {
	int16_t f = 1 - r;
	int16_t ddF_x = 1;
	int16_t ddF_y = -2 * r;
	int16_t x = 0;
	int16_t y = r;

    SSD1306_DrawPixel(x0, y0 + r, c);
    SSD1306_DrawPixel(x0, y0 - r, c);
    SSD1306_DrawPixel(x0 + r, y0, c);
    SSD1306_DrawPixel(x0 - r, y0, c);

    while (x < y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;

        SSD1306_DrawPixel(x0 + x, y0 + y, c);
        SSD1306_DrawPixel(x0 - x, y0 + y, c);
        SSD1306_DrawPixel(x0 + x, y0 - y, c);
        SSD1306_DrawPixel(x0 - x, y0 - y, c);

        SSD1306_DrawPixel(x0 + y, y0 + x, c);
        SSD1306_DrawPixel(x0 - y, y0 + x, c);
        SSD1306_DrawPixel(x0 + y, y0 - x, c);
        SSD1306_DrawPixel(x0 - y, y0 - x, c);
    }

    if(0 == is_show_str)
    {
       SSD1306_UpdateScreen(); 
    }
}

void SSD1306_DrawFilledCircle(int16_t x0, int16_t y0, int16_t r, SSD1306_COLOR_t c) {
	int16_t f = 1 - r;
	int16_t ddF_x = 1;
	int16_t ddF_y = -2 * r;
	int16_t x = 0;
	int16_t y = r;

    SSD1306_DrawPixel(x0, y0 + r, c);
    SSD1306_DrawPixel(x0, y0 - r, c);
    SSD1306_DrawPixel(x0 + r, y0, c);
    SSD1306_DrawPixel(x0 - r, y0, c);
    SSD1306_DrawLine(x0 - r, y0, x0 + r, y0, c);

    while (x < y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;

        SSD1306_DrawLine(x0 - x, y0 + y, x0 + x, y0 + y, c);
        SSD1306_DrawLine(x0 + x, y0 - y, x0 - x, y0 - y, c);

        SSD1306_DrawLine(x0 + y, y0 + x, x0 - y, y0 + x, c);
        SSD1306_DrawLine(x0 + y, y0 - x, x0 - y, y0 - x, c);
    }

    if(0 == is_show_str)
    {
       SSD1306_UpdateScreen(); 
    }
}

