#pragma once

#include "drv_uart.h"
#include "includes.h"
#include "mqtt_client.h"
#include "sywm033.h"
#include "transceiver.h"

#define FlagSET(Flag, bit) (Flag |= (bit))   // Flag置位
#define FlagCLR(Flag, bit) (Flag &= ~(bit))  // Flag清位
#define FlagGET(Flag, bit) (Flag & (bit))    // Flag获取

/* 定时器 */
extern osTimerId_t HeartBeat_Timer;

/* 互斥量 */
extern osMutexId_t TASK_MT;
/* 信号量 */
extern osSemaphoreId_t u0_uart_sem;
extern osSemaphoreId_t button_sem;

extern osMessageQueueId_t button_click_queue;
extern osMessageQueueId_t test_uart_queue;
extern osMessageQueueId_t fp_uart_queue;
extern osMessageQueueId_t lock_queue;
extern osMessageQueueId_t transceiver_queue;
extern osMessageQueueId_t mqtt_send_queue;
osMessageQueueId_t mqtt_recv_queue;

extern TRANSCEIVER* Transceiver;
extern TRANSCEIVER_IMPLEMENTS* phone;

extern MQTTCLIENT* MqttClient;
extern MQTTCLIENT_IMPLEMENTS* mqtt;

extern FINGERPRINT* Fingerprint;
extern FINGERPRINT_IMPLEMENTS* fp;

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
osThreadId_t osThreadCreat(const char* name, osThreadFunc_t func,
                           osPriority_t priority, uint32_t stacksize);

/**
 * @brief 定时器创建函数
 *
 * @param name 定时器名
 * @param func 定时器回调函数
 * @param type 定时器类型(osTimerOnce for one-shot or osTimerPeriodic for
 * periodic behavior. )
 * @return osTimerId_t 定时器ID
 */
osTimerId_t osTimerCreat(const char* name, osTimerFunc_t func,
                         osTimerType_t type);

/**
 * @brief 延迟函数
 *
 * @param ms 要是5的整数倍,最小为5ms
 */
void osDelayMs(uint32_t ms);

/**
 * @brief RTC进程（可阻塞）
 *
 * @param argument
 */
void RTC_Count_Task(void* argument);

void Test_RECV_Handle(void* argument);

/**
 * @brief 启动函数
 *
 * @param argument
 */
void startup_th(void* argument);

/**
 * @brief 按键点击线程
 *
 * @param param
 */
void Button_Click_Thread(void* param);

/**
 * @brief 通话器操作线程
 *
 * @param param
 */
void Transceiver_Thread(void* param);

/**
 * @brief MQTT客户端操作线程
 *
 * @param param
 */
void MQTT_Client_Thread(void* param);

/**
 * @brief MQTT接收线程
 *
 * @param param
 */
void MQTT_RECV_Handle(void* param);

/**
 * @brief 指纹接收线程
 *
 * @param param
 */
void FP_RECV_Handle(void* param);
void LOCK_Handle(void* param);

/**
 * @brief 心跳定时器
 *
 * @param param
 */
void Heart_Beat_Timer(void* param);
