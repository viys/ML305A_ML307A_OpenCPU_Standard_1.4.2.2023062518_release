/*-----------------------------------------------------------------------------
File Name   	:   hal_gpio.c
Created Time    :   2021.12.21
Description     :   gpio接口函数
-----------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*
                               Dependencies                                  *
-----------------------------------------------------------------------------*/
#include "hal_modem.h"
#include "includes.h"
#include "cm_pm.h"
#include "cm_sys.h"
#include "cm_virt_at.h"
#include "drv_uart.h"
#include "cm_sim.h"

#define DBG_NAME "volt"

/*----------------------------------------------------------------------------*
**                             Mcaro Definitions                              *
**----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*
**                             Global Vars                                    *
**----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*
**                             Local Vars                                    *
**----------------------------------------------------------------------------*/
static char s_onemo_imei[16] = {0};
static char s_onemo_iccid[21] = {0};
static char s_onemo_imsi[16] = {0};
/*----------------------------------------------------------------------------*
**                             Local Func                                    *
**----------------------------------------------------------------------------*/
osMessageQueueId_t s_hal_modem_queue = NULL;
osMutexId_t s_hal_modem_mutex = NULL;


/*-----------------------------------------------------------------------------
Function Name	:   hal_sleep
Author			:	yangqiang    
Created Time	:   2021.10.24
Description 	:   ms延时
Input Argv		:
Output Argv 	:
Return Value	:   
-----------------------------------------------------------------------------*/
void hal_sleep(uint32_t delay_ms)
{
    osDelay(delay_ms / portTICK_PERIOD_MS);
}

/*-----------------------------------------------------------------------------
Function Name	:	hal_get_keyword_line
Author			:	yangqiang    
Created Time	:   2021.10.24
Description 	: 	获取关键字所在行的内容
Input Argv		:
Output Argv 	:
Return Value	:
-----------------------------------------------------------------------------*/
bool hal_get_keyword_line(char *dst_buf, uint16_t bufSize, char *src_buf, char *keyword)
{
    char *head = strstr((const char*)src_buf, (const char*)keyword);
    if(head == NULL)
    {
        DBG_E("keyword(%s) is null",keyword);
        return 0;
    }
    char *end  = strstr((const char*)head, "\r\n");

    if(end != NULL)
    {
        if(bufSize >= end - head)
        {
            memcpy((char *)dst_buf, head, end - head);
            return true;
        }
    }
    else
    {
        if(bufSize >= strlen(head))
        {
            memcpy((char *)dst_buf, head, strlen(head));
            return true;
        }
    }
    return false;
}


/*-----------------------------------------------------------------------------
Function Name	:	hal_wait_rsp_keyword
Author			:	yangqiang    
Created Time	:   2021.10.24
Description 	: 	等待模组返回指令的内容
Input Argv		:
Output Argv 	:
Return Value	:
-----------------------------------------------------------------------------*/
AT_Rsp_Typedef hal_wait_rsp_keyword(uint8_t *buf, uint8_t *keyword, uint16_t timeout)
{
    uint32_t retry = 0,slice = 100;
    while(strstr((const char*)buf, (const char*)keyword) == NULL)
    {
        if(retry * slice > timeout)
        {
            return AT_TIMEOUT;
        }
        hal_sleep(slice);
        retry ++;
    }
    return AT_OK;
}

/*-----------------------------------------------------------------------------
Function Name   :    hal_modem_urc_callback
Author          :    yangqiang
Created Time    :    2021-05-12
Description     :    urc上报
Input Praram    :    
Output Argv     :
Return Value    :   
-----------------------------------------------------------------------------*/
extern 
void hal_modem_urc_callback(unsigned char *param)
{
    if(param != NULL)
    {
        DBG_I("URC:%s",param);
        // hal_sms_detect(param);
        hal_call_detect(param);
    }   
}

/*-----------------------------------------------------------------------------
Function Name   :    hal_modem_at_callback
Author          :    yangqiang
Created Time    :    2021-05-12
Description     :    重新写socket参数
Input Praram    :    param:AT指令回复
Output Argv     :
Return Value    :   
-----------------------------------------------------------------------------*/
void hal_modem_at_init()
{
    if(s_hal_modem_mutex == NULL)
    {
        osMutexAttr_t modem_Mutex_attr = {"modem_Mutex_attr",osMutexRecursive | osMutexPrioInherit, 0,0U};
        s_hal_modem_mutex = osMutexNew(&modem_Mutex_attr);
    }
    if(s_hal_modem_queue == NULL)
    {
        s_hal_modem_queue = osMessageQueueNew(1, 4, NULL);
    }
    cm_virt_at_urc_reg(hal_modem_urc_callback);
}

/*-----------------------------------------------------------------------------
Function Name   :    hal_modem_at_callback
Author          :    yangqiang
Created Time    :    2021-05-12
Description     :    重新写socket参数
Input Praram    :    param:AT指令回复
Output Argv     :
Return Value    :   
-----------------------------------------------------------------------------*/
void hal_modem_at_callback(void * param)
{
    osMessageQueuePut(s_hal_modem_queue, param, 0, 1000 / portTICK_PERIOD_MS);
}

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
bool hal_modem_atcmd_exe(char *cmd,char *keyword,int timeout_ms)
{
	unsigned char at_rsp_data[64] = {0};
	char rsp_ret[4] = {-1};
    osMutexAcquire(s_hal_modem_mutex, osWaitForever);
    if(0 != cm_virt_at_init(hal_modem_at_callback))
    {
        osMutexRelease(s_hal_modem_mutex);
        return false;
    }
    uint16_t retry = 0;
    while(1)
    {
        if (cm_virt_at_send(cmd, strlen(cmd)) > 0)
        {
            break;
        }
        retry ++;
        if(retry >= 3)
        {
            cm_virt_at_deinit();
            osMutexRelease(s_hal_modem_mutex);
            return false;
        }
        osDelay(1000 / portTICK_PERIOD_MS);
    }
	
	if(osOK == osMessageQueueGet(s_hal_modem_queue, rsp_ret, 0, timeout_ms / portTICK_PERIOD_MS))
	{
		cm_virt_at_get(at_rsp_data, sizeof(at_rsp_data));
		if(HAL_AT_RESP_OK == atoi(rsp_ret))
        {
            if(keyword == NULL || strstr((const char*)keyword,"OK") != NULL)
            {
                cm_virt_at_deinit(); 
                osMutexRelease(s_hal_modem_mutex);
				return true;
            }
            else
            {
                if (strstr((const char*)at_rsp_data, keyword) != NULL)
                {
                    cm_virt_at_deinit();
                    osMutexRelease(s_hal_modem_mutex);
                    return true;
                }
            }
        }
	}
    cm_virt_at_deinit(); 
    osMutexRelease(s_hal_modem_mutex);
	return false;
}

/*-----------------------------------------------------------------------------
Function Name   :    hal_modem_at_state
Author          :    yangqiang
Created Time    :    2022-12-29
Description     :    检查AT状态
Input Praram    :    
Output Argv     :
Return Value    :   
-----------------------------------------------------------------------------*/
bool hal_modem_at_state()
{
    uint8_t retry = 0;
    while (retry < 3)
    {
        if (hal_modem_atcmd_exe("AT\r\n", "OK", 2000))
        {
            return true;
        }
        retry++;
        hal_sleep(1000);
    }
    return false;
}

/*-----------------------------------------------------------------------------
Function Name	:	hal_modem_close_echo
Author			:	yangqiang
Created Time	:   2021.10.24
Description 	:   关闭回显
Input Argv		:
Output Argv 	:
Return Value	:
-----------------------------------------------------------------------------*/
bool hal_modem_close_echo()
{
    if (AT_OK == hal_modem_atcmd_exe("ATE0\r\n", "OK", 2000))
    {
        return true;
    }
    return false;
}

/*-----------------------------------------------------------------------------
Function Name   :    hal_modem_get_cpin
Author          :    yangqiang
Created Time    :    2022-12-29
Description     :    获取CPIN状态
Input Praram    :    
Output Argv     :
Return Value    :   
-----------------------------------------------------------------------------*/
bool hal_modem_get_cpin()
{
	uint8_t retry = 0;
    while (retry < 20)
    {
        if (hal_modem_atcmd_exe("AT+CPIN?\r\n", "READY", 2000))
        {
            return true;
        }
        retry++;
        hal_sleep(1000);
    }
    return false;
}

/*-----------------------------------------------------------------------------
Function Name	:   hal_modem_get_cfun
Author			:	yangqiang
Created Time	:   2021.10.24
Description 	:   获取协议栈状态
Input Argv		:
Output Argv 	:
Return Value	:
-----------------------------------------------------------------------------*/
bool hal_modem_get_cfun()
{
    unsigned char at_rsp_data[64] = {0},key_line[32] = {0};
	char rsp_ret[4] = {-1};
    osMutexAcquire(s_hal_modem_mutex, osWaitForever);
    if(0 != cm_virt_at_init(hal_modem_at_callback))
    {
        osMutexRelease(s_hal_modem_mutex);
        return false;
    }
	if(cm_virt_at_send((uint8_t *)"AT+CFUN?\r\n", strlen("AT+CFUN?\r\n")) < 0)
    {
        cm_virt_at_deinit(); 
        osMutexRelease(s_hal_modem_mutex);
        return false;
    }
	if(osOK == osMessageQueueGet(s_hal_modem_queue, rsp_ret, 0, 3000 / portTICK_PERIOD_MS))
	{
		cm_virt_at_get(at_rsp_data, sizeof(at_rsp_data));
		if(HAL_AT_RESP_OK == atoi(rsp_ret))
        {
			if(hal_get_keyword_line(key_line, sizeof(key_line),at_rsp_data,"+CFUN:"))
            {
                int state = 0;
                if (1 == sscanf((const char *)key_line, "+CFUN: %d", &state))
                {
                    DBG_I("get cfun value success, state: %d", state);
                    if (state == 1)
                    {
                        cm_virt_at_deinit(); 
                        osMutexRelease(s_hal_modem_mutex);
                        return true;
                    }
                }
            }
		}
	}
    cm_virt_at_deinit(); 
    osMutexRelease(s_hal_modem_mutex);
	return false;
}


/*-----------------------------------------------------------------------------
Function Name	:   hal_modem_set_cfun
Author			:	yangqiang
Created Time	:   2021.10.24
Description 	:   配置CFUN
Input Argv		:   state:设置协议栈的状态
Output Argv 	:
Return Value	:
-----------------------------------------------------------------------------*/
bool hal_modem_set_cfun(uint8_t state)
{
    char cmd[20] = {0};
    sprintf(cmd, "AT+CFUN=%d\r\n", state);
    return hal_modem_atcmd_exe(cmd,"OK",2000);
}

/*-----------------------------------------------------------------------------
Function Name	:   hal_modem_get_csq
Author			:	yangqiang   yangqiang
Created Time	:   2021.10.24
Description 	:   获取csq
Input Argv		:   
Output Argv 	:
Return Value	:
-----------------------------------------------------------------------------*/
bool hal_modem_get_csq(char *rssi,char *ber)
{
    unsigned char at_rsp_data[64] = {0},key_line[32] = {0};
	char rsp_ret[4] = {-1};
    osMutexAcquire(s_hal_modem_mutex, osWaitForever);
    if(0 != cm_virt_at_init(hal_modem_at_callback))
    {
        osMutexRelease(s_hal_modem_mutex);
        return false;
    }
	if(cm_virt_at_send((uint8_t *)"AT+CSQ\r\n", strlen("AT+CSQ?\r\n")) < 0)
    {
        cm_virt_at_deinit(); 
        osMutexRelease(s_hal_modem_mutex);
        return false;
    }
	if(osOK == osMessageQueueGet(s_hal_modem_queue, rsp_ret, 0, 1000 / portTICK_PERIOD_MS))
	{
		cm_virt_at_get(at_rsp_data, sizeof(at_rsp_data));
		if(HAL_AT_RESP_OK == atoi(rsp_ret))
        {
			if(hal_get_keyword_line(key_line, sizeof(key_line),at_rsp_data,"+CSQ:"))
            {
                int l_csq = 0,l_ber = 0;
                if (2 == sscanf((const char *)key_line, "+CSQ: %d,%d", &l_csq,&l_ber))
                {
                    *rssi = l_csq;
                    *ber = l_ber;
                    DBG_I("get csq success:%d,%d",*rssi,*ber);
                    cm_virt_at_deinit(); 
                    osMutexRelease(s_hal_modem_mutex);
                    return true;
                }
            }
		}
	}
    cm_virt_at_deinit(); 
    osMutexRelease(s_hal_modem_mutex);
	return false;
}

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
bool hal_modem_get_imei(char *imei, int len)
{   
    if(len < 15)
    {
        return false;
    }
    if(strlen(s_onemo_imei) > 5)
    {
        strcpy(imei,s_onemo_imei);
        return true;
    }
    if(cm_sys_get_imei(imei) == 0)
    {
        strcpy(s_onemo_imei,imei);
        return true;
    }
    return false;
}

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
bool hal_modem_get_iccid(char *ccid, int len)
{
    unsigned char at_rsp_data[64] = {0},key_line[32] = {0};
	char rsp_ret[4] = {-1};
    if(strlen(s_onemo_iccid) > 5)
    {
        strcpy(ccid,s_onemo_iccid);
        return true;
    }
    osMutexAcquire(s_hal_modem_mutex, osWaitForever);
    if(0 != cm_virt_at_init(hal_modem_at_callback))
    {
        osMutexRelease(s_hal_modem_mutex);
        return false;
    }
	if(cm_virt_at_send((uint8_t *)"AT*ICCID?\r\n", strlen("AT*ICCID?\r\n")) < 0)
    {
        cm_virt_at_deinit(); 
        osMutexRelease(s_hal_modem_mutex);
        return false;
    }
	if(osOK == osMessageQueueGet(s_hal_modem_queue, rsp_ret, 0, 3000 / portTICK_PERIOD_MS))
	{
		cm_virt_at_get(at_rsp_data, sizeof(at_rsp_data));
		if(HAL_AT_RESP_OK == atoi(rsp_ret))
        {
			if(hal_get_keyword_line(key_line, sizeof(key_line),at_rsp_data,"*ICCID:"))
            {
                if (1 == sscanf((const char *)key_line, "*ICCID: \"%[^\"]\"", ccid))
                {
                    strcpy(s_onemo_iccid,ccid);
                    DBG_I("get iccid success, ccid: %s", ccid);
                    cm_virt_at_deinit(); 
                    osMutexRelease(s_hal_modem_mutex);
                    return true;
                }
            }
		}
	}
    cm_virt_at_deinit(); 
    osMutexRelease(s_hal_modem_mutex);
	return false;
}

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
bool hal_modem_get_imsi(char *imsi, int len)
{   
    if(len < 15)
    {
        return false;
    }
    if(strlen(s_onemo_imsi) > 5)
    {
        strcpy(imsi,s_onemo_imsi);
        return true;
    }
    if(cm_sim_get_imsi(imsi) == 0)
    {
        strcpy(s_onemo_imsi,imsi);
        return true;
    }
    return false;
}

/*-----------------------------------------------------------------------------
Function Name	:   hal_modem_pdp_active
Author			:	yangqiang
Created Time	:   2021.10.24
Description 	:   PDP激活
Input Argv		:
Output Argv 	:
Return Value	:
-----------------------------------------------------------------------------*/
bool hal_modem_simhotswap_off()
{
    return hal_modem_atcmd_exe("AT+MCFG=\"simhot\",0\r\n","OK",2000);
}

/*-----------------------------------------------------------------------------
Function Name	:   hal_get_cereg_state
Author			:	yangqiang
Created Time	:   2021.10.24
Description 	:   获取CEREG状态
Input Argv		:
Output Argv 	:
Return Value	:
-----------------------------------------------------------------------------*/
bool hal_modem_get_cereg_state()
{
    unsigned char at_rsp_data[64] = {0},key_line[32] = {0};
	char rsp_ret[4] = {-1};
    osMutexAcquire(s_hal_modem_mutex, osWaitForever);
    if(0 != cm_virt_at_init(hal_modem_at_callback))
    {
        osMutexRelease(s_hal_modem_mutex);
        return false;
    }
    if(cm_virt_at_send((uint8_t *)"AT+CEREG?\r\n", strlen("AT+CEREG?\r\n")) < 0)
    {
        cm_virt_at_deinit(); 
        osMutexRelease(s_hal_modem_mutex);
        return false;
    }
    if(osOK == osMessageQueueGet(s_hal_modem_queue, rsp_ret, 0, 3000 / portTICK_PERIOD_MS))
	{
		cm_virt_at_get(at_rsp_data, sizeof(at_rsp_data));
		if(HAL_AT_RESP_OK == atoi(rsp_ret))
        {
			if(hal_get_keyword_line(key_line, sizeof(key_line),at_rsp_data,"+CEREG:"))
            {
                int state = 0, cid = 0;
                if (2 == sscanf((const char *)key_line, "+CEREG: %d,%d", &cid, &state))
                {
                    DBG_I("get CEREG value success, state: %d", state);
                    if (state == 1 || state == 5)
                    {
                        DBG_I("cat1 net register success");
                        cm_virt_at_deinit(); 
                        osMutexRelease(s_hal_modem_mutex);
                        return true;
                    }
                }
            }
		}
	}
    cm_virt_at_deinit();
    osMutexRelease(s_hal_modem_mutex);
	return false;
}


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
bool hal_modem_get_cellInfo(uint32_t *lac, uint32_t *ci)
{
    unsigned char at_rsp_data[64] = {0},key_line[32] = {0};
	char rsp_ret[4] = {-1};
    osMutexAcquire(s_hal_modem_mutex, osWaitForever);
    if(0 != cm_virt_at_init(hal_modem_at_callback))
    {
        osMutexRelease(s_hal_modem_mutex);
        return false;
    }
	if(cm_virt_at_send((uint8_t *)"AT+CEREG=5;+CEREG?\r\n", strlen("AT+CEREG=5;+CEREG?\r\n")) < 0)
    {
        cm_virt_at_deinit(); 
        osMutexRelease(s_hal_modem_mutex);
        return false;
    }
    if(osOK == osMessageQueueGet(s_hal_modem_queue, rsp_ret, 0, 3000 / portTICK_PERIOD_MS))
	{
		cm_virt_at_get(at_rsp_data, sizeof(at_rsp_data));
		if(HAL_AT_RESP_OK == atoi(rsp_ret))
        {
			if(hal_get_keyword_line(key_line, sizeof(key_line),at_rsp_data,"+CEREG:"))
            {
                if (sscanf((const char *)key_line, "%*[^:]:%*[^,],%*[^,],\"%lx\",\"%lx\"", lac, ci) == 2)
                {
                    cm_virt_at_deinit(); 
                    osMutexRelease(s_hal_modem_mutex);
                    DBG_I("get cell info success, lac: %04x, ci: %08x", *lac, *ci);
                    return true;
                }
            }
		}
	}
    cm_virt_at_deinit(); 
    osMutexRelease(s_hal_modem_mutex);
	return false;
}

/*-----------------------------------------------------------------------------
Function Name   :   hal_get_apn
Author			:	yangqiang
Created Time	:   2021.10.24
Description     :   根据IMSI获取APN(默认为移动APN)
Input Argv      :   apn:apn
Output Argv     :
Return Value    :   0:移动
                    1:联通
                    2:电信
                    <0:错误
-----------------------------------------------------------------------------*/
int hal_modem_get_apn(char *apn, uint16_t apn_size)
{
    memset(apn, 0, apn_size);
    if (apn_size < 5)
    {
        memcpy(apn, "CMNET", strlen("CMNET")); /* 默认：移动APN */
        return -1;
    }
    char buf[32] = {0};
    cm_sim_get_imsi(buf);

    if (strncmp(buf, "46000", strlen("46000")) == 0 || strncmp(buf, "46002", strlen("46002")) == 0 || strncmp(buf, "46004", strlen("46004")) == 0 || strncmp(buf, "46007", strlen("46007")) == 0 || strncmp(buf, "46008", strlen("46008")) == 0)
    {
        memcpy(apn, "CMNET", strlen("CMNET")); /* 移动APN */
        DBG_I("Sim card is: china mobile");
        return 0;
    }

    if (strncmp(buf, "46001", strlen("46001")) == 0 || strncmp(buf, "46006", strlen("46006")) == 0 || strncmp(buf, "46009", strlen("46009")) == 0 || strncmp(buf, "46010", strlen("46010")) == 0)
    {
        memcpy(apn, "WONET", strlen("WONET")); /* 联通APN */
        DBG_I("Sim card is: china unicom");
        return 1;
    }

    if (strncmp(buf, "46003", strlen("46003")) == 0 || strncmp(buf, "46011", strlen("46011")) == 0 || strncmp(buf, "46012", strlen("46012")) == 0)
    {
        memcpy(apn, "CTNET", strlen("CTNET")); /* 电信APN */
        DBG_I("Sim card is: china net");
        return 2;
    }
    memcpy(apn, "CMNET", strlen("CMNET")); /* 默认：移动APN */
    return -1;
}

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
bool hal_modem_set_apn(char *apn,char *username,char *password)
{
    bool result = false;
    char cmd[64] = {0};
    sprintf(cmd, "AT+CGDCONT=1,\"IP\",\"%s\"\r\n", apn);
    result = hal_modem_atcmd_exe(cmd,"OK",2000);
    if(result && strlen(username) > 0)
    {
        memset(cmd,0,sizeof(cmd));
        sprintf(cmd, "AT*AUTHREQ=1,1,\"%s\",\"%s\"\r\n", username,password);
        result = hal_modem_atcmd_exe(cmd,"OK",2000);
    }
    return result;
}

/*-----------------------------------------------------------------------------
Function Name	:   hal_modem_pdp_active
Author			:	yangqiang
Created Time	:   2021.10.24
Description 	:   PDP激活
Input Argv		:
Output Argv 	:
Return Value	:
-----------------------------------------------------------------------------*/
bool hal_modem_pdp_active()
{
    unsigned int retry = 0;
    while (retry < 3)
    {
        if (AT_OK == hal_modem_atcmd_exe("AT+CGACT=1,1\r\n", "OK", 5000))
        {
            return true;
        }
        retry++;
        hal_sleep(1000);
    }
    return false;
}

/*-----------------------------------------------------------------------------
Function Name	:   hal_modem_pdp_deactive
Author			:	yangqiang
Created Time	:   2021.10.24
Description 	:   PDP去激活
Input Argv		:
Output Argv 	:
Return Value	:
-----------------------------------------------------------------------------*/
bool hal_modem_pdp_deactive()
{
    unsigned int retry = 0;
    while (retry < 3)
    {
        if (AT_OK == hal_modem_atcmd_exe("AT+CGACT=0,1\r\n", "OK", 2000))
        {
            return true;
        }
        retry++;
        hal_sleep(2000);
    }
    return false;
}

/*-----------------------------------------------------------------------------
Function Name	:   hal_get_pdp_state
Author			:	yangqiang
Created Time	:   2021.10.24
Description 	:   获取pdp状态
Input Argv		:
Output Argv 	:
Return Value	:
-----------------------------------------------------------------------------*/
bool hal_modem_get_pdp_state()
{
    unsigned char at_rsp_data[64] = {0},key_line[32] = {0};
	char rsp_ret[4] = {-1};
    osMutexAcquire(s_hal_modem_mutex, osWaitForever);
    if(0 != cm_virt_at_init(hal_modem_at_callback))
    {
        osMutexRelease(s_hal_modem_mutex);
        return false;
    }
    if(cm_virt_at_send((uint8_t *)"AT+CGACT?\r\n", strlen("AT+CGACT?\r\n")) < 0)
    {
        cm_virt_at_deinit(); 
        osMutexRelease(s_hal_modem_mutex);
        return false;
    }
    if(osOK == osMessageQueueGet(s_hal_modem_queue, rsp_ret, 0, 3000 / portTICK_PERIOD_MS))
	{
		cm_virt_at_get(at_rsp_data, sizeof(at_rsp_data));
		if(HAL_AT_RESP_OK == atoi(rsp_ret))
        {
			if(hal_get_keyword_line(key_line, sizeof(key_line),at_rsp_data,"+CGACT:"))
            {
                int cid = 0, state = 0;
                if (2 == sscanf((const char *)key_line, "+CGACT: %d,%d", &cid, &state))
                {
                    DBG_I("get CGACT value success, cid:%d,state: %d", cid, state);
                    if(state == 1)
                    {
                        cm_virt_at_deinit();
                        osMutexRelease(s_hal_modem_mutex);
                        return true;
                    }
                }
            }
		}
	}
    cm_virt_at_deinit(); 
    osMutexRelease(s_hal_modem_mutex);
	return false;
}

/*-----------------------------------------------------------------------------
Function Name	:   hal_modem_get_celldata
Author			:	yangqiang
Created Time	:   2021.10.24
Description 	:   获取基站信息
Input Argv		:
Output Argv 	:
Return Value	:
-----------------------------------------------------------------------------*/
bool hal_modem_get_celldata(hal_cell_data_t *cell_data)
{
    unsigned char at_rsp_data[300] = {0};
    char *offset = NULL;
	char rsp_ret[4] = {-1};
    osMutexAcquire(s_hal_modem_mutex, osWaitForever);
    if(0 != cm_virt_at_init(hal_modem_at_callback))
    {
        osMutexRelease(s_hal_modem_mutex);
        return false;
    }
    if(cm_virt_at_send((uint8_t *)"AT+EEMGINFO?;+ZDON?;+COPS?\r\n", strlen("AT+EEMGINFO?;+ZDON?;+COPS?\r\n")) < 0)
    {
        cm_virt_at_deinit(); 
        osMutexRelease(s_hal_modem_mutex);
        return false;
    }
    if(osOK == osMessageQueueGet(s_hal_modem_queue, rsp_ret, 0, 5000 / portTICK_PERIOD_MS))
	{
		cm_virt_at_get(at_rsp_data, sizeof(at_rsp_data));
		if(HAL_AT_RESP_OK == atoi(rsp_ret))
        {
            offset = strstr((const char*)at_rsp_data,"+ZDON");
			if(offset != NULL)
            {
                //+ZDON: "CMCC","460","00","","460","04","ROAM_OFF"
                sscanf(offset,"%*[^,],\"%d\",\"%d\"",&cell_data->mcc,&cell_data->mnc);
            }
            //见ASR AT文档
            //+EEMLTESVC:1120, 2, 0, 13112, 77, 38950, 38950, 40, 5, 50, 23, 24, 2, 10, 0, 1, 0, 929, 118, 0,1, 219511489, 2, 7, 2, 49, 0, 23, 0, 49, 15, 80, 1, 0,1, 0, 0, 0, 0, 0, 1, 15, 0, 0, 0, 0, 0, 0, 0, 0, 0, t3402:720,t3412:3240
            offset = strstr((const char*)at_rsp_data,"+EEMLTESVC");
			if(offset != NULL)
            {
                sscanf(offset,"%*[^,],%*[^,],%*[^,],%d,%*[^,],%*[^,],%*[^,],%*[^,],%*[^,],%d,%d,%d,%*[^,],%*[^,],%*[^,],%*[^,],%*[^,],%*[^,],%*[^,],%*[^,],%*[^,],%d,%*[^,],%*[^,],%*[^,],%*[^,],%*[^,],%*[^,],%*[^,],%d",
                &cell_data->tac,&cell_data->rsrp,&cell_data->rsrq,&cell_data->snr,&cell_data->cellid,&cell_data->rssi);
            }
            cell_data->rsrp = cell_data->rsrp - 140;
            if(cell_data->rsrq == 255)
            {
                cell_data->rsrq = -195; //无效值
            }
            else
            {
                cell_data->rsrq = ((-195 + 5 * cell_data->rsrq) > -30) ? -30 : (-195 + 5 * cell_data->rsrq);
            }
            cell_data->rssi = cell_data->rssi - 110;
            cm_virt_at_deinit();
            osMutexRelease(s_hal_modem_mutex);
            DBG_I("hal_cell_data_t:%d,%d,%d,%d,%d,%d,%u,%d",cell_data->mcc,cell_data->mnc,cell_data->tac,cell_data->rsrp,cell_data->rsrq,cell_data->snr,cell_data->cellid,cell_data->rssi);
            return true;
        }
	}
    cm_virt_at_deinit(); 
    osMutexRelease(s_hal_modem_mutex);
	return false;
}

/*-----------------------------------------------------------------------------
Function Name	:   hal_modem_wait_cereg_registered
Author			:	yangqiang
Created Time	:   2021.10.24
Description 	:   等待网络注册
Input Argv		:
Output Argv 	:
Return Value	:
-----------------------------------------------------------------------------*/
bool hal_modem_wait_cereg_registered(uint16_t time_s)
{
    unsigned int retry = 0;
    while (retry < time_s)
    {
        if (hal_modem_get_cereg_state())
        {
            return true;
        }
        retry++;
        hal_sleep(1000);
    }
    DBG_E("wait net registered timeout");
    return false;
}

/*-----------------------------------------------------------------------------
Function Name   :   hal_modem_wait_pdp_active
Author          :   yangqiang
Created Time    :   2020.07.28
Description     :   等待激活pdp
Input Argv      :
Output Argv     :
Return Value    :
-----------------------------------------------------------------------------*/
bool hal_modem_wait_pdp_active(uint16_t timeout)
{
    if (!hal_modem_wait_cereg_registered(timeout))
    {
        return false;
    }
    if (!hal_modem_get_pdp_state())
    {
        if (!hal_modem_pdp_active())
        {
            return false;
        }
    }
    return true;
}


/*-----------------------------------------------------------------------------
Function Name	:   hal_modem_network_init
Author			:	yangqiang
Created Time	:   2021.10.24
Description 	:   模组网络初始化
Input Argv		:
Output Argv 	:
Return Value	:
-----------------------------------------------------------------------------*/
int hal_modem_network_init()
{
    int net_step = 0;
    uint8_t retry = 0;
    while (1)
    {
        switch (net_step)
        {
            case 0:
            {
                if(hal_modem_at_state())
                {
                    hal_modem_close_echo();
                    hal_modem_simhotswap_off();
                    net_step++;
                }
                else
                {
                    return -1;
                }
                break;
            }
            /* 识别SIM卡 */
            case 1:
            {
                if (hal_modem_get_cpin())
                {
                    net_step++;
                }
                else
                {
                    return -2;
                }
                break;
            }
            /* 查询协议栈 */
            case 2:
            {
                if (!hal_modem_get_cfun())
                {
                    net_step++;
                    break;
                }
                net_step += 2;
                break;
            }
            /* 打开协议栈 */
            case 3:
            {
                if (!hal_modem_set_cfun(1))
                {
                    DBG_E("open cfun fail");
                    return -3;
                    break;
                }
                net_step = 0;
                break;
            }
            /* 等待网络注册(超时120S) */
            case 4:
            {
                if (!hal_modem_wait_cereg_registered(120))
                {
                    net_step = 9;
                    break;
                }
                net_step++;
                retry = 0;
                break;
            }
            /* 获取并配置APN */
            case 5:
            {
                char l_apn[32] = {0};
                hal_modem_get_apn(l_apn, sizeof(l_apn));
                if (!hal_modem_set_apn(l_apn,"",""))
                {
                    retry++;
                    if (retry > 3)
                    {
                        return -4;
                    }
                    hal_modem_pdp_deactive();
                    break;
                }
                net_step++;
                retry = 0;
                break;
            }
            /* 查询PDP状态 */
            case 6:
            {
                if(hal_modem_get_pdp_state())
                {
                    return 0;
                }
                retry++;
                if (retry > 5)
                {
                    retry = 0;
                    net_step++;
                }   
                break;
            }
            /* 激活pdp */
            case 7:
            {
                if (hal_modem_pdp_active())
                {
                    return 0;
                }
                retry++;
                if (retry > 3)
                {
                    return -5;
                    break;
                }
                net_step++;
                break;
            }
            /* 去激活pdp */
            case 8:
            {
                hal_modem_pdp_deactive();
                hal_sleep(100);
                net_step = 7;
                break;
            }
            /* 关闭协议栈 */
            case 9:
            {
                hal_modem_set_cfun(0);
                net_step = 0;
                break;
            }
            /* 重启 */
            case 255:
            {
                DBG_E("net init fail, system will reboot...");
                hal_sleep(1000);
                cm_pm_reboot();
                return -1;
            }
            default:
            {
                DBG_E("net step error:%d,will reboot...", net_step);
                hal_sleep(1000);
                cm_pm_reboot();
                break;
            }
        }
        hal_sleep(100);
    }
    DBG_E("cat1 network init fail, system will reboot...");
    hal_sleep(1000);
    cm_pm_reboot();
    return -1;
}