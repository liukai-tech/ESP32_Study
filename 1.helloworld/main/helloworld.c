/*
* @file         helloworld.c 
* @brief        print "hello world"
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
#include "esp_system.h"


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
	printf("Hello World!\n");
	printf("Powered by Caesar.\n");
}


