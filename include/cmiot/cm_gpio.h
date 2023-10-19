/**
 * @file        cm_gpio.h
 * @brief       GPIO接口
 * @copyright   Copyright © 2021 China Mobile IOT. All rights reserved.
 * @author      By ZHANGXW
 * @date        2021/03/09
 *
 * @defgroup gpio gpio
 * @ingroup PI
 * @{
 */

#ifndef __CM_GPIO_H__
#define __CM_GPIO_H__

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdint.h>
#include <stdbool.h>
/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Public Types
 ****************************************************************************/
/** 引脚ID,详情参照资源综述 */
typedef enum{
    CM_GPIO_NUM_0,               
    CM_GPIO_NUM_1,               
    CM_GPIO_NUM_2,               
    CM_GPIO_NUM_3,               
    CM_GPIO_NUM_4,               
    CM_GPIO_NUM_5,               
    CM_GPIO_NUM_6,          
    CM_GPIO_NUM_7,               
    CM_GPIO_NUM_8,               
    CM_GPIO_NUM_9,               
    CM_GPIO_NUM_10,              
    CM_GPIO_NUM_11,              
    CM_GPIO_NUM_12,           
    CM_GPIO_NUM_13,              
    CM_GPIO_NUM_14,              
    CM_GPIO_NUM_15,              
    CM_GPIO_NUM_16,              
    CM_GPIO_NUM_17,              
    CM_GPIO_NUM_18,              
    CM_GPIO_NUM_19,              
    CM_GPIO_NUM_20,              
    CM_GPIO_NUM_21,              
    CM_GPIO_NUM_22,              
    CM_GPIO_NUM_23,   
    CM_GPIO_NUM_24,    
    CM_GPIO_NUM_25,    
    CM_GPIO_NUM_26,    
    CM_GPIO_NUM_27,    
    CM_GPIO_NUM_28,    
    CM_GPIO_NUM_29,    
    CM_GPIO_NUM_30,
    CM_GPIO_NUM_31,
    CM_GPIO_NUM_32,
    CM_GPIO_NUM_33,
    CM_GPIO_NUM_34,
    CM_GPIO_NUM_35,
    CM_GPIO_NUM_36,               
    CM_GPIO_NUM_MAX
}cm_gpio_num_e;

/* *GPIO工作模式 */
typedef enum 
{
    CM_GPIO_MODE_NUM/*!<不可用 工作模式数量*/
} cm_gpio_mode_e;

/** I/O方向 */
typedef enum{
    CM_GPIO_DIRECTION_INPUT = 0,
    CM_GPIO_DIRECTION_OUTPUT,
}cm_gpio_direction_e;

/** 上/下拉 */
typedef enum{
    CM_GPIO_PULL_NONE,
    CM_GPIO_PULL_DOWN,
    CM_GPIO_PULL_UP,
}cm_gpio_pull_e;

/** 边沿+电平触发 */
typedef enum{
    CM_GPIO_IT_EDGE_RISING,
    CM_GPIO_IT_EDGE_FALLING,
    CM_GPIO_IT_EDGE_BOTH,
    CM_GPIO_IT_LEVEL_HIGH,/*!<不支持*/
    CM_GPIO_IT_LEVEL_LOW,/*!<不支持*/
}cm_gpio_interrupt_e;

/** 高低电平 */
typedef enum{
    CM_GPIO_LEVEL_LOW,
    CM_GPIO_LEVEL_HIGH,
}cm_gpio_level_e;

/** 命令码 */
typedef enum{
    CM_GPIO_CMD_SET_PULL,                  /*!< 上下拉设置命令码*/
    CM_GPIO_CMD_GET_PULL,                  /*!< 上下拉获取命令码*/
    CM_GPIO_CMD_SET_LEVEL,                 /*!< 驱动能力设置命令码*/
    CM_GPIO_CMD_GET_LEVEL,                 /*!< 驱动能力获取命令码*/
    CM_GPIO_CMD_SET_DIRECTION,             /*!< 输入输出设置命令码*/
    CM_GPIO_CMD_GET_DIRECTION,             /*!< 输入输出获取命令码*/
} cm_gpio_cmd_e;

/** 配置 */
typedef struct{
    cm_gpio_mode_e mode;/*!< 不支持*/
    cm_gpio_direction_e direction;
    cm_gpio_pull_e pull;
} cm_gpio_cfg_t;

/****************************************************************************
 * Public Data
 ****************************************************************************/


/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
***********************GPIO使用注意事项*************************************
* 1、GPIO引脚和SD卡控制引脚所用PIN脚存在复用关系，若用户使用此PIN脚进行
*    GPIO功能开发，请阅知cm_sd.h中【SD卡使用注意事项】
* 2、GPIO引脚和SD卡控制引脚复用关系见《资源综述》
***************************************************************************/

#ifdef __cplusplus
#define EXTERN extern "C"
extern "C"
{
#else
#define EXTERN extern
#endif

/**
 *  @brief 初始化GPIO
 *  
 *  @param [in] gpio_num GPIO号
 *  @param [in] cfg 配置
 *  
 *  @return 
 *    = 0 - 成功 \n
 *    < 0 - 失败, 返回值为错误码
 *  
 *  @details 初始化之前一定要先设置引脚复用
 */
int32_t cm_gpio_init(cm_gpio_num_e gpio_num, cm_gpio_cfg_t *cfg);

/**
 *  @brief 去初始化
 *  
 *  @param [in] gpio_num GPIO号
 *  
 *  @return 
 *    = 0 - 成功 \n
 *    < 0 - 失败, 返回值为错误码
 *  
 *  @details More details
 */
int32_t cm_gpio_deinit(cm_gpio_num_e gpio_num);

/**
 *  @brief 设置输出电平
 *  
 *  @param [in] gpio_num GPIO号
 *  @param [in] level 输出电平
 *  
 *  @return 
 *    = 0 - 成功 \n
 *    < 0 - 失败, 返回值为错误码
 *  
 *  @details More details
 */
int32_t cm_gpio_set_level(cm_gpio_num_e gpio_num, cm_gpio_level_e level);

/**
 *  @brief 读取输入电平
 *  
 *  @param [in] gpio_num GPIO号
 *  @param [out] level 输入电平
 *  
 *  @return 
 *    = 0 - 成功 \n
 *    < 0 - 失败, 返回值为错误码
 *  
 *  @details More details
 */
int32_t cm_gpio_get_level(cm_gpio_num_e gpio_num, cm_gpio_level_e *level);

/**
 *  @brief 设置上/下拉
 *  
 *  @param [in] gpio_num GPIO号
 *  @param [in] type 上下拉配置
 *  
 *  @return 
 *    = 0 - 成功 \n
 *    < 0 - 失败, 返回值为错误码
 *  
 *  @details More details
 */
int32_t cm_gpio_set_pull(cm_gpio_num_e gpio_num, cm_gpio_pull_e type);

/**
 *  @brief 读取上/下拉
 *  
 *  @param [in] gpio_num GPIO号
 *  @param [out] type 上下拉配置
 *  
 *  @return 
 *    = 0 - 成功 \n
 *    < 0 - 失败, 返回值为错误码
 *  
 *  @details More details
 */
int32_t cm_gpio_get_pull(cm_gpio_num_e gpio_num, cm_gpio_pull_e *type);

/**
 *  @brief I/O方向模式配置
 *  
 *  @param [in] gpio_num GPIO号
 *  @param [in] dir 输入/输出
 *  
 *  @return 
 *    = 0 - 成功 \n
 *    < 0 - 失败, 返回值为错误码
 *  
 *  @details More details
 */
int32_t cm_gpio_set_direction(cm_gpio_num_e gpio_num, cm_gpio_direction_e dir);

/**
 *  @brief 获取I/O方向模式配置
 *  
 *  @param [in] gpio_num GPIO号
 *  @param [out] dir 输入/输出
 *  
 *  @return 
 *    = 0 - 成功 \n
 *    < 0 - 失败, 返回值为错误码
 *  
 *  @details More details
 */
int32_t cm_gpio_get_direction(cm_gpio_num_e gpio_num, cm_gpio_direction_e *dir);

/**
 *  @brief 注册中断回调函数
 *  
 *  @param [in] gpio_num GPIO号
 *  @param [in] interrupt_cb 中断回调函数
 *  
 *  @return 
 *    = 0 - 成功 \n
 *    < 0 - 失败, 返回值为错误码
 *  
 *  @details 中断函数中不要处理耗时任务和打印log日志
 */
int32_t cm_gpio_interrupt_register(cm_gpio_num_e gpio_num, void *interrupt_cb);

/**
 *  @brief 使能中断
 *  
 *  @param [in] gpio_num GPIO号
 *  @param [in] intr_mode 中断触发方式
 *  
 *  @return 
 *    = 0 - 成功 \n
 *    < 0 - 失败, 返回值为错误码
 *  
 *  @details More details
 */
int32_t cm_gpio_interrupt_enable(cm_gpio_num_e gpio_num, cm_gpio_interrupt_e intr_mode);

/**
 *  @brief 失能中断
 *  
 *  @param [in] gpio_num GPIO号
 *  
 *  @return 
 *    = 0 - 成功 \n
 *    < 0 - 失败, 返回值为错误码
 *  
 *  @details More details
 */
int32_t cm_gpio_interrupt_disable(cm_gpio_num_e gpio_num);

/**
 *  @brief GPIO控制
 *  
 *  @param [in] gpio_num GPIO号
 *  @param [in] cmd 命令码
 *  @param [in/out] arg 命令
 *  
 *  @return 
 *    = 0 - 成功 \n
 *    < 0 - 失败, 返回值为错误码
 *  
 *  @details 根据不同的cmd值（命令码），arg参数类型须对应配置。\n
 *  cmd = \ref GPIO_CMD_SET_PULL, arg传入\ref cm_gpio_pull_e *               \n
 *  cmd = \ref GPIO_CMD_GET_PULL, arg传入\ref cm_gpio_pull_e *               \n
 *  cmd = \ref GPIO_CMD_SET_LEVEL, arg传入\ref cm_gpio_level_e *             \n
 *  cmd = \ref GPIO_CMD_GET_LEVEL, arg传入\ref cm_gpio_level_e *             \n
 *  cmd = \ref GPIO_CMD_SET_DIRECTION, arg传入\ref cm_gpio_direction_e *     \n
 *  cmd = \ref GPIO_CMD_GET_DIRECTION, arg传入\ref cm_gpio_direction_e *     \n
 */
int32_t cm_gpio_ioctl(cm_gpio_num_e gpio_num, int32_t cmd, void *arg);

#undef EXTERN
#ifdef __cplusplus
}
#endif


#endif /* __CM_GPIO_H__ */

/** @}*/
