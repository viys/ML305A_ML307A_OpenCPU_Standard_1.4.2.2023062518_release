/**
 * @file main.c
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2023-8-26
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include "app.h"

osThreadId_t ThreadI_DT;   // 线程ID标识
osThreadAttr_t THREAD_CFG; // 线程配置结构体

/*串口信号量*/
osSemaphoreId_t u0_uart_sem = NULL;

int cm_opencpu_entry(char *param)
{
	uart_open(CM_UART_DEV_0, 921600, u0_callback);
	u0_printf("ML307A Start\r\n");

	/* 创建进程 */
	osThreadCreat("dbg_th", dbg_th, 4, 1024);


	

	return 0;
}
