/**
 * @file         cm_demo_ntp.c
 * @brief        NTP接口
 * @copyright    Copyright © 2021 China Mobile IOT. All rights reserved.
 * @author       WangPeng
 * @date         2021/07/26
 *
 */

#include "stdio.h"
#include "cm_ntp.h"
#include "drv_ntp.h"
#include "drv_uart.h"

#define DBG_NAME "ntp"

int ntpTemp = 0;

static void __cm_ntp_cb(cm_ntp_event_e event, void *event_param, void *cb_param)
{
    if (CM_NTP_EVENT_SYNC_OK == event)
    {
        ntpTemp = 1;
        DBG_I("[NTP][%s] get net time %s\r\n", (char *)cb_param, event_param); 
    }
    else if (CM_NTP_EVENT_SETTIME_FAIL == event)
    {
        DBG_I("[NTP][%s] get net time %s OK, but set time fail\r\n", (char *)cb_param, event_param); 
    }
    else
    {
        DBG_I("[NTP][%s] error\r\n", (char *)cb_param); 
        my_ntp_get();
    }
}

/**
 *  UART口NTP功能功能调试使用示例
 *  CM:NTP:GETNETTIME                           //仅获取网络时间，不同步至本地时间
 *  CM:NTP:SYNCHTIME                            //获取网络时间并将网络时间与本地时间同步
 */
int my_ntp_get(void)
{
    int ret = 0;
    uint16_t port = 123;
    uint32_t timeout = 10000;
    uint32_t dns_priority = 1;
    bool set_rtc = 1;

    cm_ntp_set_cfg(CM_NTP_CFG_SERVER, "cn.ntp.org.cn"); 
    cm_ntp_set_cfg(CM_NTP_CFG_PORT, &port);
    cm_ntp_set_cfg(CM_NTP_CFG_TIMEOUT, &timeout);
    cm_ntp_set_cfg(CM_NTP_CFG_DNS, &dns_priority);
    cm_ntp_set_cfg(CM_NTP_CFG_SET_RTC, &set_rtc);
    cm_ntp_set_cfg(CM_NTP_CFG_CB, __cm_ntp_cb);
    cm_ntp_set_cfg(CM_NTP_CFG_CB_PARAM, "SYNCHTIME");

    ret = cm_ntp_sync();
    if(ret != 0){
        DBG_I("[NTP]cm_ntp_sync() fail\r\n");
    }

    return ret;
}
