/**
 * @file shell_port.c
 * @author Letter (NevermindZZT@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2019-02-22
 * 
 * @copyright (c) 2019 Letter
 * 
 */

// #include "FreeRTOS.h"
// #include "task.h"
#include "cm_os.h"
#include "shell.h"
#include "drv_uart.h"
// #include "serial.h"
// #include "stm32f4xx_hal.h"
// #include "usart.h"
// #include "cevent.h"
// #include "log.h"

Shell shell;
char shellBuffer[512];

// static SemaphoreHandle_t shellMutex;

/**
 * @brief 用户shell写
 * 
 * @param data 数据
 * @param len 数据长度
 * 
 * @return short 实际写入的数据长度
 */
short userShellWrite(char *data, unsigned short len)
{
    cm_uart_write(CM_UART_DEV_0, data, len, 1000);
    return len;
}


/**
 * @brief 用户shell读
 * 
 * @param data 数据
 * @param len 数据长度
 * 
 * @return short 实际读取到
 */
short userShellRead(char *data, unsigned short len)
{
    return cm_uart_read(CM_UART_DEV_0, data, len, 1000);
}

/**
 * @brief 用户shell上锁
 * 
 * @param shell shell
 * 
 * @return int 0
 */
int userShellLock(Shell *shell)
{
    // xSemaphoreTakeRecursive(shellMutex, portMAX_DELAY);
    return 0;
}

/**
 * @brief 用户shell解锁
 * 
 * @param shell shell
 * 
 * @return int 0
 */
int userShellUnlock(Shell *shell)
{
    // xSemaphoreGiveRecursive(shellMutex);
    return 0;
}

/**
 * @brief 用户shell初始化
 * 
 */
void userShellInit(void)
{
    // shellMutex = xSemaphoreCreateMutex();

    shell.write = userShellWrite;
    shell.read = userShellRead;
    // shell.lock = userShellLock;
    // shell.unlock = userShellUnlock;
    shellInit(&shell, shellBuffer, 512);

    osThreadAttr_t thread_cfg =
	{
		/* 进程名 */
		.name = "shell",
        /* 优先级 */
		.priority = osPriorityNormal,
        /* 栈空间 */
		.stack_size = 1024,
	};

    /* ML307A模组模组用户线程优先级必须低于或等于osPriorityNormal */

    if(osThreadNew(shellTask,&shell,(const osThreadAttr_t *)&thread_cfg) == NULL){
        // logError("shell task creat failed");
    }
}
