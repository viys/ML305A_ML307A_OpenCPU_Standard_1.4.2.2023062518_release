#include "app.h"
#include "drv_uart.h"
#include "drv_virt_at.h"
#include "hal_call.h"
#include "hal_modem.h"

#define DBG_NAME "app"

void dbg_th(void* argument)
{
    /*串口由于LOG功能,已在cm_opencpu_entry()开头初始化*/
    int readLen = 0 ;
    /* 创建信号量 */
	u0_uart_sem = osSemaphoreNew(1, 0, NULL);
    DBG_F("hmi_uart_sem ID: %d\r\n", u0_uart_sem);
    hal_modem_at_init();
    while(1){
        osSemaphoreAcquire(u0_uart_sem, osWaitForever);
        readLen = u0_uart_read(rxBuff);
        my_virt_at_test((unsigned char*)rxBuff, strlen(rxBuff));
        // hal_make_call("13333762618");
        u0_printf("Send %s (%d)\r\n",rxBuff , readLen);
    }
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