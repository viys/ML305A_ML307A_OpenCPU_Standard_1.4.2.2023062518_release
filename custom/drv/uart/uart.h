#ifndef __UART_H
#define __UART_H

#include "cm_uart.h"
#include "cm_iomux.h"

/**
 * @brief 打开串口设备
 * 
 * @param dev 串口设备号
 * @param baudrate 波特率
 * @param callback 回调函数
 */
void uart_open(cm_uart_dev_e dev,int baudrate,void* callback);

/**
 * @brief 串口0回调函数
 * 
 * @param param 参数
 * @param type 事件类型
 */
void u0_callback(void *param, uint32_t type);

/**
 * @brief 串口0 Printf函数
 * 
 * @param str 
 * @param ... 
 */
void u0_printf(char *str, ...);

#endif // !__UART_H