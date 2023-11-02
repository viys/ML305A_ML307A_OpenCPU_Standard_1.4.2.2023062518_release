/*-----------------------------------------------------------------------------
File Name       :   HAL_call.c
Author          :    
Created Time    :   2020.06.28
Description     :   语音通话相关函数接口
-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
                             Dependencies
-----------------------------------------------------------------------------*/
#include "hal_modem.h"
#include "hal_call.h"
#include "includes.h"
#include "drv_uart.h"
#include "cm_virt_at.h"

#define DBG_NAME "volt"

/*----------------------------------------------------------------------------*
**                             Global Vars                                    *
**----------------------------------------------------------------------------*/
hal_call_data_t s_call_data_t = {HAL_CALL_STATE_NONE,{0}};     /* 语音结构体 */

/*----------------------------------------------------------------------------*
**                             Local Vars                                     *
**----------------------------------------------------------------------------*/
extern osMessageQueueId_t s_hal_modem_queue ;
extern osMutexId_t s_hal_modem_mutex ;

static osMessageQueueId_t s_call_msg_queue = NULL;

hal_call_callback hal_call_cb = NULL;

/*-----------------------------------------------------------------------------
Function Name	:	hal_make_call
Author			:	 
Created Time	:	2020.05.09
Description 	:	拨打语音电话(异步)
Input Argv		:   phone_number:电话号码
Output Argv 	:
Return Value	:   
-----------------------------------------------------------------------------*/
bool hal_make_call(char *phone_number)
{
    if(phone_number == NULL)
    {
        return false;
    }
    char call_cmd[32] = {0};
    snprintf(call_cmd,sizeof(call_cmd),"ATD%s;\r\n",phone_number);
    if(hal_modem_atcmd_exe(call_cmd,"OK",5000))  /* 拨打电话 */
    {
        s_call_data_t.call_state = HAL_CALL_STATE_CALLING;
        DBG_I("call \"%s\" success",phone_number);
        return true;
    }
    s_call_data_t.call_state = HAL_CALL_STATE_NONE;
    DBG_I("call \"%s\" fail",phone_number);
    return false;
}

/*-----------------------------------------------------------------------------
Function Name	:	hal_answer_call
Author			:	 
Created Time	:	2020.05.09
Description 	:	接听语音电话
Input Argv		:  
Output Argv 	:
Return Value	:   
-----------------------------------------------------------------------------*/
bool hal_answer_call()
{
    if(hal_modem_atcmd_exe("ATA\r\n","CONNECT",10000) == AT_OK)    /* 接听电话 */
    {
        DBG_I("answer a call success");
        return true;
    }
    DBG_I("answer a call fail or timeout");
    return false;
}

/*-----------------------------------------------------------------------------
Function Name	:	hal_hangup_call
Author			:	 
Created Time	:	2020.05.09
Description 	:	挂掉语音电话
Input Argv		:   
Output Argv 	:
Return Value	:   
-----------------------------------------------------------------------------*/
bool hal_hangup_call()
{
    if(hal_modem_atcmd_exe("ATH\r\n","OK",5000) == AT_OK)    /* 挂电话 */
    {
        DBG_I("hangup a call success");
        return true;
    }
    DBG_I("hangup a call fail");
    return false;
}

/*-----------------------------------------------------------------------------
Function Name	:	hal_clear_allcall
Author			:	 
Created Time	:	2020.05.09
Description 	:	挂掉所有语音电话
Input Argv		:   
Output Argv 	:
Return Value	:   
-----------------------------------------------------------------------------*/
bool hal_clear_allcall()
{
    if(hal_modem_atcmd_exe("AT+CHUP\r\n","OK",5000) == AT_OK)    /* 挂电话 */
    {
        DBG_I("hangup all call success");
        return true;
    }
    DBG_I("hangup all call fail");
    return false;
}


/*-----------------------------------------------------------------------------
Function Name	:	hal_get_phonenumber
Author			:	 
Created Time	:	2020.05.09
Description 	:	获取对方的电话号码
Input Argv		:   phonenumber:电话号码缓冲区
                    phonenumber_size:电话号码缓冲区大小
Output Argv 	:
Return Value	:   
-----------------------------------------------------------------------------*/
bool hal_get_phonenumber(char *phonenumber,uint8_t phonenumber_size)
{
    if(phonenumber == NULL || phonenumber_size <= 5)
    {
        return false;
    }

    int sms_id = 0;
    unsigned char at_rsp_data[128] = {0},key_line[64] = {0};
	char rsp_ret[4] = {-1};
    osMutexAcquire(s_hal_modem_mutex, osWaitForever);
    if(0 != cm_virt_at_init(hal_modem_at_callback))
    {
        osMutexRelease(s_hal_modem_mutex);
        return false;
    }
    if(cm_virt_at_send((uint8_t *)"AT+CLCC\r\n", strlen("AT+CLCC\r\n")) < 0)
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
			if(hal_get_keyword_line(key_line, sizeof(key_line),at_rsp_data,"+CLCC:"))
            {
                if (sscanf(key_line, "+CLCC:%*[^,],%*[^,],%*[^,],%*[^,],%*[^,],\"%[^\"]\"", phonenumber) == 1)
                {
                    cm_virt_at_deinit();
                    osMutexRelease(s_hal_modem_mutex);
                    DBG_I("get phonenumber success:\"%s\"", phonenumber);
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
Function Name	:	hal_send_call_msg
Author			:	 
Created Time	:	2020.05.09
Description 	:   发送语音消息
Input Argv		:   
Output Argv 	:
Return Value	:   
-----------------------------------------------------------------------------*/
void hal_send_call_msg(int msg,int param)
{
    hal_call_msg_t call_msg = {0};
    call_msg.msgid = msg;
    call_msg.param1 = param;
    osMessageQueuePut(s_call_msg_queue, &call_msg, 0, 0 / portTICK_PERIOD_MS);
}

/*-----------------------------------------------------------------------------
Function Name	:	hal_call_detect
Author			:	 
Created Time	:	2020.05.09
Description 	:   语音监测
Input Argv		:   
Output Argv 	:
Return Value	:   
-----------------------------------------------------------------------------*/
void hal_call_detect(char *data)
{
    if (strstr(data, "RING") != NULL)
    {
        s_call_data_t.call_state = HAL_CALL_STATE_RING;
    }
    else if (strstr(data, "+CLCC:") != NULL)
    {
        char *offset = NULL;
        offset = strstr(data, "+CLCC:");
        memset(s_call_data_t.phonenumber,0,sizeof(s_call_data_t.phonenumber));
        if (sscanf(offset, "+CLCC:%*[^,],%*[^,],%*[^,],%*[^,],%*[^,],\"%[^\"]\"", s_call_data_t.phonenumber) == 1)
        {
            if(s_call_data_t.call_state == HAL_CALL_STATE_RING)
            {
                hal_send_call_msg(HAL_CALL_STATE_RING,0);
            }
            else if(s_call_data_t.call_state == HAL_CALL_STATE_CONNECTED)
            {
                hal_send_call_msg(HAL_CALL_STATE_CONNECTED,0);
            }
        }
    }
    else if (strstr(data, "NO CARRIER") != NULL)
    {
        s_call_data_t.call_state = HAL_CALL_STATE_DISCONNECTED;
        hal_send_call_msg(HAL_CALL_STATE_DISCONNECTED,0);
    }
    else if(strstr(data, "CONNECT") != NULL)
    {
        s_call_data_t.call_state = HAL_CALL_STATE_CONNECTED;
    }
}

/*-----------------------------------------------------------------------------
Function Name	:	hal_deal_call_msg
Author			:	 
Created Time	:	2020.05.09
Description 	:   发送语音消息
Input Argv		:   
Output Argv 	:
Return Value	:   
-----------------------------------------------------------------------------*/
void hal_deal_call_msg(hal_call_msg_t *call_msg)
{
    switch (call_msg->msgid)
    {
        case HAL_CALL_STATE_RING:
        {
            if(hal_call_cb != NULL)
            {
                hal_call_cb(HAL_CALL_STATE_RING,s_call_data_t.phonenumber);
            }
            break;
        }
        case HAL_CALL_STATE_DISCONNECTED:
        {
            if(hal_call_cb != NULL)
            {
                hal_call_cb(HAL_CALL_STATE_DISCONNECTED,s_call_data_t.phonenumber);
            }
            s_call_data_t.call_state = HAL_CALL_STATE_NONE;
            break;
        }
        case HAL_CALL_STATE_CONNECTED:
        {
            if(hal_call_cb != NULL)
            {
                hal_call_cb(HAL_CALL_STATE_CONNECTED,s_call_data_t.phonenumber);
            }
            break;
        }
        default:
            break;
    }
}

/*-----------------------------------------------------------------------------
Function Name	:	hal_call_task
Author			:	 
Created Time	:	2020.05.09
Description 	:   语音线程
Input Argv		:   
Output Argv 	:
Return Value	:   
-----------------------------------------------------------------------------*/
void hal_call_task(void *p)
{
    hal_call_msg_t call_msg = {0};
    while(1)
    {
        if (osMessageQueueGet(s_call_msg_queue, &call_msg, NULL, osWaitForever) == osOK)
        {
            hal_deal_call_msg(&call_msg);
        }
    }
}

/*-----------------------------------------------------------------------------
Function Name	:	hal_call_init
Author			:	 
Created Time	:	2020.05.09
Description 	:   语音监测
Input Argv		:   
Output Argv 	:
Return Value	:   
-----------------------------------------------------------------------------*/
void hal_call_init(hal_call_callback call_cb)
{
    hal_call_cb = call_cb;

    s_call_msg_queue = osMessageQueueNew(10, sizeof(hal_call_msg_t), NULL); //创建短信消息队列

    osThreadAttr_t app_task_attr = {0};
    app_task_attr.name = "call_task";
    app_task_attr.stack_size = 4 * 1024;
    app_task_attr.priority = osPriorityNormal;
    osThreadNew(hal_call_task, 0, &app_task_attr);
}

/*-----------------------------------------------------------------------------
Function Name	:	hal_get_call_state
Author			:	 
Created Time	:	2020.05.09
Description 	:   获取语音状态
Input Argv		:   
Output Argv 	:
Return Value	:   
-----------------------------------------------------------------------------*/
int hal_get_call_state()
{
    return s_call_data_t.call_state;
}

/*-----------------------------------------------------------------------------
Function Name	:	hal_get_call_number
Author			:	 
Created Time	:	2020.05.09
Description 	:   获取来电或者通话中的号码
Input Argv		:   
Output Argv 	:
Return Value	:   
-----------------------------------------------------------------------------*/
bool hal_get_call_number(char *number,int numbersize)
{
    if(numbersize < strlen(s_call_data_t.phonenumber))
    {
        return false;
    }
    if(hal_get_call_state() != HAL_CALL_STATE_RING && hal_get_call_state() != HAL_CALL_STATE_CONNECTED)
    {
        return false;
    }
    memset(number,0,numbersize);
    strcpy(number,s_call_data_t.phonenumber);
    return true;
}