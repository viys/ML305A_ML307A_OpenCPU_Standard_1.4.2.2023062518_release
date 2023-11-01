#ifndef __APP_H
#define __APP_H

#include "includes.h"
#include "drv_uart.h"

#define FlagSET(Flag,bit)	(Flag |= (bit))		//Flag置位
#define FlagCLR(Flag,bit)	(Flag &= ~(bit))	//Flag清位
#define FlagGET(Flag,bit)	(Flag & (bit))		//Flag获取

/* 互斥量 */
extern osMutexId_t TASK_MT;
/* 信号量 */
extern osSemaphoreId_t u0_uart_sem;
/* 定时器 */
osTimerId_t TASK_TT;

/**
 * @brief 进程创建函数
 * 
 * @param name 进程名
 * @param func 进程函数
 * @param priority 优先级
 * @param stacksize 栈空间
 * @return osThreadId_t 进程ID
 */
osThreadId_t osThreadCreat(const char * name,osThreadFunc_t func,osPriority_t priority,uint32_t stacksize);

/**
 * @brief 定时器创建函数
 * 
 * @param name 定时器名
 * @param func 定时器回调函数
 * @param type 定时器类型(osTimerOnce for one-shot or osTimerPeriodic for periodic behavior. )
 * @return osTimerId_t 定时器ID
 */
osTimerId_t osTimerCreat(const char * name,osTimerFunc_t func,osTimerType_t type);

/**
 * @brief 延迟函数
 * 
 * @param ms 要是5的整数倍,最小为5ms
 */
void osDelayMs(uint32_t ms);

/**
 * @brief AT 测试进程
 * 
 * @param argument 
 */
void dbg_th(void* argument);

#endif // !__APP_H