/*-----------------------------------------------------------------------------
File Name		:   hal_modem.h
Author			:	yangqiang    
Created Time	:   2020.10.23
Description		:   modem接口
-----------------------------------------------------------------------------*/
#ifndef __HAL_MODEM_H__
#define __HAL_MODEM_H__

/*---------------------------------------------------------------------------*
                               Dependencies                                  *
-----------------------------------------------------------------------------*/
#include "includes.h"

/*----------------------------------------------------------------------------*
**                             Mcaro Definitions                              *
**----------------------------------------------------------------------------*/

#define  HAL_AT_RESP_OK                 (0)     //  0  HAL_AT_RESP_OK               正确返回
#define  HAL_AT_RESP_CME_ERROR          (1)     //  1  HAL_AT_RESP_CME_ERROR        命令错误
#define  HAL_AT_RESP_INVALID_PARAM      (2)     //  2  HAL_AT_RESP_INVALID_PARAM    参数无效
#define  HAL_AT_RESP_TIME_OUT           (3)     //  3  HAL_AT_RESP_TIME_OUT         请求超时
#define  HAL_AT_RESP_SYS_ERROR          (4)     //  4  HAL_AT_RESP_SYS_ERROR        系统错误
#define portTICK_PERIOD_MS  5

#define HAL_AT_DELAY_INTERVAL (500 / portTICK_PERIOD_MS)

/*---------------------------------------------------------------------------*
                               Dependencies                                  *
-----------------------------------------------------------------------------*/
typedef enum
{
    AT_ERROR = 0,
    AT_OK,
    AT_TIMEOUT
}AT_Rsp_Typedef;    /* AT指令执行状态 */

typedef struct 
{
    int mcc;
    int mnc;
    unsigned int tac;
    unsigned int cellid;
    int rsrp;
    int rsrq;   //放大10倍
    int rssi;
    int snr;
}hal_cell_data_t;

/*----------------------------------------------------------------------------*
**                             Mcaro Definitions                              *
**----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
Function Name	:	hal_get_keyword_line
Author			:	yangqiang   
Created Time	:   2021.10.24
Description 	: 	获取关键字所在行的内容
Input Argv		:
Output Argv 	:
Return Value	:
-----------------------------------------------------------------------------*/
bool hal_get_keyword_line(char *dst_buf, uint16_t bufSize, char *src_buf, char *keyword);

/*-----------------------------------------------------------------------------
Function Name   :    hal_modem_at_callback
Author          :    yangqiang
Created Time    :    2021-05-12
Description     :    重新写socket参数
Input Praram    :    param:AT指令回复
Output Argv     :
Return Value    :   
-----------------------------------------------------------------------------*/
void hal_modem_at_callback(void * param);

/*-----------------------------------------------------------------------------
Function Name   :    hal_modem_at_init
Author          :    yangqiang
Created Time    :    2021-05-12
Description     :    虚拟AT初始化
Input Praram    :    
Output Argv     :
Return Value    :   
-----------------------------------------------------------------------------*/
void hal_modem_at_init();

/*-----------------------------------------------------------------------------
Function Name	:   hal_modem_atcmd_exe
Author			:	yangqiang
Created Time	:   2021.10.24
Description 	:   at执行
Input Argv		:   cmd:命令
                    cmdlen:命令长度
                    keyword:关键字
                    timeout_ms:超时等待时间
Output Argv 	:
Return Value	:
-----------------------------------------------------------------------------*/
bool hal_modem_atcmd_exe(char *cmd,char *keyword,int timeout_ms);

/*-----------------------------------------------------------------------------
Function Name   :    hal_modem_at_state
Author          :    yangqiang
Created Time    :    2022-12-29
Description     :    检查AT状态
Input Praram    :    
Output Argv     :
Return Value    :   
-----------------------------------------------------------------------------*/
bool hal_modem_at_state();

/*-----------------------------------------------------------------------------
Function Name	:	hal_modem_close_echo
Author			:	yangqiang
Created Time	:   2021.10.24
Description 	:   关闭回显
Input Argv		:
Output Argv 	:
Return Value	:
-----------------------------------------------------------------------------*/
bool hal_modem_close_echo();

/*-----------------------------------------------------------------------------
Function Name   :    hal_modem_get_cpin
Author          :    yangqiang
Created Time    :    2022-12-29
Description     :    获取CPIN状态
Input Praram    :    
Output Argv     :
Return Value    :   
-----------------------------------------------------------------------------*/
bool hal_modem_get_cpin();

/*-----------------------------------------------------------------------------
Function Name	:   hal_modem_get_cfun
Author			:	yangqiang
Created Time	:   2021.10.24
Description 	:   获取协议栈状态
Input Argv		:
Output Argv 	:
Return Value	:
-----------------------------------------------------------------------------*/
bool hal_modem_get_cfun();

/*-----------------------------------------------------------------------------
Function Name	:   hal_modem_set_cfun
Author			:	yangqiang
Created Time	:   2021.10.24
Description 	:   配置CFUN
Input Argv		:   state:设置协议栈的状态
Output Argv 	:
Return Value	:
-----------------------------------------------------------------------------*/
bool hal_modem_set_cfun(uint8_t state);

/*-----------------------------------------------------------------------------
Function Name	:   hal_modem_get_csq
Author			:	yangqiang   yangqiang
Created Time	:   2021.10.24
Description 	:   获取csq
Input Argv		:   
Output Argv 	:
Return Value	:
-----------------------------------------------------------------------------*/
bool hal_modem_get_csq(char *rssi,char *ber);

/*-----------------------------------------------------------------------------
Function Name	:   hal_modem_get_imei
Author			:	yangqiang
Created Time	:   2021.10.24
Description 	:   获取IMEI号
Input Argv		:   imei:存放imei缓冲区
                    len:缓冲区长度
Output Argv 	:   
Return Value	:
-----------------------------------------------------------------------------*/
bool hal_modem_get_imei(char *imei, int len);

/*-----------------------------------------------------------------------------
Function Name	:   hal_modem_get_iccid
Author			:	yangqiang
Created Time	:   2021.10.24
Description 	:   获取ICCID号
Input Argv		:   ccid:存放ccid缓冲区
                    len:缓冲区长度
Output Argv 	:   ccid：sim卡的ccid
Return Value	:
-----------------------------------------------------------------------------*/
bool hal_modem_get_iccid(char *ccid, int len);

/*-----------------------------------------------------------------------------
Function Name	:   hal_modem_get_imsi
Author			:	yangqiang
Created Time	:   2021.10.24
Description 	:   获取IMSI号
Input Argv		:   imsi:存放imsi缓冲区
                    len:缓冲区长度
Output Argv 	:   
Return Value	:
-----------------------------------------------------------------------------*/
bool hal_modem_get_imsi(char *imsi, int len);

/*-----------------------------------------------------------------------------
Function Name	:   hal_get_cereg_state
Author			:	yangqiang
Created Time	:   2021.10.24
Description 	:   获取CEREG状态
Input Argv		:
Output Argv 	:
Return Value	:
-----------------------------------------------------------------------------*/
bool hal_modem_get_cereg_state();

/*-----------------------------------------------------------------------------
Function Name	:	hal_modem_get_cellInfo
Author			:	yangqiang
Created Time	:   2021.10.24
Description 	:   获取基站相关信息
Input Argv		:
Output Argv 	:   lac:位置区码
                    ci:ci值
Return Value	:
-----------------------------------------------------------------------------*/
bool hal_modem_get_cellInfo(uint32_t *lac, uint32_t *ci);

/*-----------------------------------------------------------------------------
Function Name	:   hal_modem_set_apn
Author			:	yangqiang
Created Time	:   2021.10.24
Description 	:   配置APN参数
Input Argv		:   apn:apn名称
                    username:用户名
                    password:密码
Output Argv 	:
Return Value	:
-----------------------------------------------------------------------------*/
bool hal_modem_set_apn(char *apn,char *username,char *password);

/*-----------------------------------------------------------------------------
Function Name	:   hal_modem_pdp_active
Author			:	yangqiang
Created Time	:   2021.10.24
Description 	:   PDP激活
Input Argv		:
Output Argv 	:
Return Value	:
-----------------------------------------------------------------------------*/
bool hal_modem_pdp_active();

/*-----------------------------------------------------------------------------
Function Name	:   hal_modem_pdp_deactive
Author			:	yangqiang
Created Time	:   2021.10.24
Description 	:   PDP去激活
Input Argv		:
Output Argv 	:
Return Value	:
-----------------------------------------------------------------------------*/
bool hal_modem_pdp_deactive();

/*-----------------------------------------------------------------------------
Function Name	:   hal_get_pdp_state
Author			:	yangqiang
Created Time	:   2021.10.24
Description 	:   获取pdp状态
Input Argv		:
Output Argv 	:
Return Value	:
-----------------------------------------------------------------------------*/
bool hal_modem_get_pdp_state();

/*-----------------------------------------------------------------------------
Function Name	:   hal_modem_get_celldata
Author			:	yangqiang
Created Time	:   2021.10.24
Description 	:   获取基站信息
Input Argv		:
Output Argv 	:
Return Value	:
-----------------------------------------------------------------------------*/
bool hal_modem_get_celldata(hal_cell_data_t *cell_data);

/*-----------------------------------------------------------------------------
Function Name	:   hal_modem_wait_cereg_registered
Author			:	yangqiang
Created Time	:   2021.10.24
Description 	:   等待网络注册
Input Argv		:
Output Argv 	:
Return Value	:
-----------------------------------------------------------------------------*/
bool hal_modem_wait_cereg_registered(uint16_t time_s);

/*-----------------------------------------------------------------------------
Function Name   :   hal_modem_wait_pdp_active
Author          :   yangqiang
Created Time    :   2020.07.28
Description     :   等待激活pdp
Input Argv      :
Output Argv     :
Return Value    :
-----------------------------------------------------------------------------*/
bool hal_modem_wait_pdp_active(uint16_t timeout);

/*-----------------------------------------------------------------------------
Function Name	:   hal_modem_network_init
Author			:	yangqiang
Created Time	:   2021.10.24
Description 	:   模组网络初始化
Input Argv		:
Output Argv 	:
Return Value	:
-----------------------------------------------------------------------------*/
int hal_modem_network_init();

#endif
