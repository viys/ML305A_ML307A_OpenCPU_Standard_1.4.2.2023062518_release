/**
 * @file        cm_pwm.h
 * @brief       PWM接口
 * @copyright   Copyright © 2021 China Mobile IOT. All rights reserved.
 * @author      By ZHANGXW
 * @date        2021/03/09
 *
 * @defgroup pwm pwm
 * @ingroup PI
 * @{
 */

#ifndef __CM_PWM_H__
#define __CM_PWM_H__

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdint.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Public Types
 ****************************************************************************/

/** 设备ID，详情参照资源综述 */
typedef enum{
    CM_PWM_DEV_0,              /*!< 设备0*/
    CM_PWM_DEV_1,              /*!< 设备1*/
    CM_PWM_DEV_2,              /*!< 设备2*/
    CM_PWM_DEV_3,              /*!< 设备3*/
    CM_PWM_DEV_NUM
} cm_pwm_dev_e;

/****************************************************************************
 * Public Data
 ****************************************************************************/


/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#ifdef __cplusplus
#define EXTERN extern "C"
extern "C"
{
#else
#define EXTERN extern
#endif

/**
 *  @brief 打开PWM设备
 *  
 *  @param [in] dev PWM设备ID
 *  @param [in] period 周期(ns)
 *  @param [in] period_h 周期高电平占用时间(ns)
 *  
 *  @return 
 *    = 0 - 成功 \n
 *    < 0 - 失败, 返回值为错误码
 *  
 *  @details PWM由于分频和算法设计存在一定的误差，受限于软件和硬件，因此period和period_h需根据下面备注进行设置\n
 *   How to get the PV(The value of after scaled clock cycles per cycle/T): \n
 *   InputClockT=13M;PWM_T=period;
 *  InputClockT * ('PV(The value of before scaled clock cycles per cycle)) = PWM_T \n
 *      'PV   = PWM_T / InputClockT \n
 *           = PWM_T * InputClockF \n
 *      PV = 'PV /(prescale + 1) -1 \n
 *  How to get the prescale: \n
 *      Because the internal clock period counter is 10-bit, to avoid overun. We can use the prescale.\n
 *      prescale real value  = ('PV - 1(if > 1024, then need prescale)) / 1024 \n
 *  
 *  How to get the Duty cycle: \n
 *      Duty cycle = (PV(the cycles per T) + 1) * ratio(Duty time/PWM_T)
*/
int32_t cm_pwm_open_ns(cm_pwm_dev_e dev, uint32_t period, uint32_t period_h);

/**
 *  @brief 关闭PWM设备
 *  
 *  @param [in] dev PWM设备ID
 *  
 *  @return 
 *    = 0 - 成功 \n
 *    < 0 - 失败, 返回值为错误码
 *  
 *  @details More details
 */
int32_t cm_pwm_close(cm_pwm_dev_e dev);

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /* __CM_PWM_H__ */

/** @}*/
