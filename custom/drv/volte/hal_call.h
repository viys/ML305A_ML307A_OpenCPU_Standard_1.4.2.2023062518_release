/*-----------------------------------------------------------------------------
File Name		:   hal_call.h
Author			:   
Created Time	:   2020.10.23
Description		:   语音通话相关函数
-----------------------------------------------------------------------------*/
#ifndef __HAL_CALL_H__
#define __HAL_CALL_H__

/*---------------------------------------------------------------------------*
                               Dependencies                                  *
-----------------------------------------------------------------------------*/
#include "includes.h"

/*----------------------------------------------------------------------------*
**                             Mcaro Definitions                              *
**----------------------------------------------------------------------------*/

#define portTICK_PERIOD_MS 5

enum
{
    HAL_CALL_MSG_ANSWER,
    HAL_CALL_MSG_HANGUP,
};

enum
{
    HAL_CALL_STATE_NONE,        /* 默认无 */
    HAL_CALL_STATE_RING,        /* 响铃(有电话需要接听) */
    HAL_CALL_STATE_CALLING,     /* 语音通话连接中 */
    HAL_CALL_STATE_CONNECTED,   /* 语音通话连接成功 */
    HAL_CALL_STATE_DISCONNECTED /* 语音连接断开 */
} ;               /* 语音通话状态 */

typedef struct 
{
    int msgid;
    int param1;
}hal_call_msg_t;

/* 语音通话结构体 */
typedef struct
{
    int call_state;
	char phonenumber[20];
} hal_call_data_t;

typedef void (*hal_call_callback)(int state,char *number);

/*-----------------------------------------------------------------------------
Function Name	:	hal_call_detect
Author			:	 
Created Time	:	2020.05.09
Description 	:   语音监测
Input Argv		:   
Output Argv 	:
Return Value	:   
-----------------------------------------------------------------------------*/
void hal_call_detect(char *data);

/*-----------------------------------------------------------------------------
Function Name	:	hal_make_call
Author			:	yangqiang
Created Time	:	2020.05.09
Description 	:	拨打语音电话(异步)
Input Argv		:   phone_number:电话号码
Output Argv 	:
Return Value	:   
-----------------------------------------------------------------------------*/
bool hal_make_call(char *phone_number);

/*-----------------------------------------------------------------------------
Function Name	:	hal_answer_call
Author			:	yangqiang
Created Time	:	2020.05.09
Description 	:	接听语音电话
Input Argv		:  
Output Argv 	:
Return Value	:   
-----------------------------------------------------------------------------*/
bool hal_answer_call();

/*-----------------------------------------------------------------------------
Function Name	:	hal_hangup_call
Author			:	yangqiang
Created Time	:	2020.05.09
Description 	:	挂掉语音电话
Input Argv		:   
Output Argv 	:
Return Value	:   
-----------------------------------------------------------------------------*/
bool hal_hangup_call();

/*-----------------------------------------------------------------------------
Function Name	:	hal_clear_allcall
Author			:	yangqiang
Created Time	:	2020.05.09
Description 	:	挂掉所有语音电话
Input Argv		:   
Output Argv 	:
Return Value	:   
-----------------------------------------------------------------------------*/
bool hal_clear_allcall();

/*-----------------------------------------------------------------------------
Function Name	:	hal_get_phonenumber
Author			:	yangqiang
Created Time	:	2020.05.09
Description 	:	获取对方的电话号码
Input Argv		:   phonenumber:电话号码缓冲区
                    phonenumber_size:电话号码缓冲区大小
Output Argv 	:
Return Value	:   
-----------------------------------------------------------------------------*/
bool hal_get_phonenumber(char *phonenumber,uint8_t phonenumber_size);

/*-----------------------------------------------------------------------------
Function Name	:	hal_call_init
Author			:	 
Created Time	:	2020.05.09
Description 	:   语音初始化
Input Argv		:   
Output Argv 	:
Return Value	:   
-----------------------------------------------------------------------------*/
void hal_call_init(hal_call_callback call_cb);

/*-----------------------------------------------------------------------------
Function Name	:	hal_get_call_state
Author			:	 
Created Time	:	2020.05.09
Description 	:   获取语音状态
Input Argv		:   
Output Argv 	:
Return Value	:   
-----------------------------------------------------------------------------*/
int hal_get_call_state();

/*-----------------------------------------------------------------------------
Function Name	:	hal_get_call_number
Author			:	 
Created Time	:	2020.05.09
Description 	:   获取来电或者通话中的号码
Input Argv		:   
Output Argv 	:
Return Value	:   
-----------------------------------------------------------------------------*/
bool hal_get_call_number(char *number,int numbersize);

#endif
