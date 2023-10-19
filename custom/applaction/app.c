#include "app.h"

/* 互斥锁 */
osMutexId_t TASK_MT = NULL;
/* 信号量 */
osSemaphoreId_t TASK_ST = NULL;
/* 定时器 */
osTimerId_t TASK_TT = NULL;

void task1(void *argument)
{
    osStatus_t resM;
    while (1)
    {
        resM = osMutexAcquire(TASK_MT,1000);
        if(resM == osOK){
            osDelayMs(100);
            u0_printf("task1 runing1\r\n");
            osMutexRelease(TASK_MT);
        }
        u0_printf("Task1 TASK_MT Timeout\r\n");
        // osDelayMs(1000);
    }
}

void task2(void *argument)
{
    osStatus_t resM;
    while (1)
    {
        resM = osMutexAcquire(TASK_MT,1000);
        if(resM == osOK){
            u0_printf("task2 runing>>>>>>>>>>>>>>\r\n");
            osDelayMs(1000);
            u0_printf("task2 runing..............\r\n");
            osMutexRelease(TASK_MT);
            osSemaphoreRelease(TASK_ST);
        }
        u0_printf("Task2 TASK_MT Timeout\r\n");
    }
    
}

void task3(void *argument)
{
    osStatus_t resS;
    while(1){
        resS = osSemaphoreAcquire(TASK_ST,1000);
        if(resS == osOK){
            u0_printf("task3 runing\r\n");
        }
    }
}

void time1_callback(void *param)
{

}

osThreadId_t osThreadCreat(const char * name,osThreadFunc_t func,osPriority_t priority,uint32_t stacksize)
{
	
	osThreadAttr_t thread_cfg =
	{
		/* 进程名 */
		.name = name,
        /* 优先级 */
		.priority = priority,
        /* 栈空间 */
		.stack_size = stacksize,
	};

    /* ML307A模组模组用户线程优先级必须低于或等于osPriorityNormal */
    if(osPriorityNormal > thread_cfg.priority){
        thread_cfg.priority = osPriorityNormal;
    }

	return osThreadNew(func,NULL,(const osThreadAttr_t *)&thread_cfg);

}

osTimerId_t osTimerCreat(const char * name,osTimerFunc_t func,osTimerType_t type)
{
    osTimerAttr_t timer_attr = 
    {
        .name = name,
    };
    return osTimerNew(func,type,NULL,&timer_attr);
}

void osDelayMs(uint32_t ms)
{
    /* ML307A心跳周期为5ms */
    osDelay(ms/5);
}