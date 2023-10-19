/*
 * @Author: yuzy 2628587386@qq.com
 * @Date: 2023-08-16 18:28:42
 * @LastEditors: yuzy 2628587386@qq.com
 * @LastEditTime: 2023-08-16 18:38:40
 * @FilePath: \ML307A_Template\custom\applaction\main.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "app.h"

osThreadId_t ThreadI_DT;   // 线程ID标识
osThreadAttr_t THREAD_CFG; // 线程配置结构体

int cm_opencpu_entry(char *param)
{
	uart_open(CM_UART_DEV_0, CM_UART_BAUDRATE_115200, u0_callback);
	u0_printf("ML307A Start\r\n");

	/* 创建互斥锁 */
	TASK_MT = osMutexNew(NULL);
	/* 创建信号量 */
	TASK_ST = osSemaphoreNew(1, 0, NULL);
	/* 创建定时器 */
	// tim = osTimerCreat("timer1",time1_callback,osTimerPeriodic);
	/* 创建进程 */
	osThreadCreat("task1", task1, 6, 1024);
	osThreadCreat("task2", task2, 5, 1024);
	osThreadCreat("task3", task3, 7, 1024);

	

	return 0;
}
