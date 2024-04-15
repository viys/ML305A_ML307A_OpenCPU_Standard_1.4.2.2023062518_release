#pragma once

#include "stdio.h"
#include "drv_uart.h"

typedef unsigned char uint8_t;
typedef unsigned short int uint16_t;
// typedef unsigned int uint32_t;
#define bool _Bool
// enum bool{false, true};

#define MAX_NUM_RX_BYTES    100   // Maximum RX bytes to receive in one go

/* 指纹异常次数限制 */
#define FP_ABNORMAL_LIMIT_TIMES        5
#define FP_LOCK_TIME_MAX               300
#define FP_WORK_TIME_MAX               10

#define FP_HEADER   0xAA

/* BYD */
#define PS_BYD_HEADER   0xEF01
#define PS_BYD_ADDR     0xFFFFFFFF
#define PS_HEADER_LEN   9
#define PS_BODY_MAX     40
#define PS_ADD16_LEN    2

#define PS_CMD_HEADER   0x01 //命令包
#define PS_DATA_HEADER  0x02 //数据包
#define PS_DEND_HEADER  0x08 //最后一个数据包

#define PS_Cancel               0x30 //取消指令
#define PS_AutoEnroll           0x31 //自动注册模板
#define PS_AutoIdentify         0x32 //自动验证指纹
#define PS_Sleep                0x33 //休眠指令
#define PS_DeletChar            0x0C //删除模板
#define PS_Empty                0x0D //清空指纹库
#define PS_ValidTempleteNum     0x1D //读取有效模板个数
#define PS_SetPwd               0x12 //设置口令
#define PS_VfyPwd               0x13 //验证口令
#define PS_ReadIndexTable       0x1F //读索引表
#define PS_Ctrl_LED             0x3C //灯光控制

typedef enum{
    idle = 0x00,    //空闲
    identify,       //指纹验证
    enroll,         //指纹注册
    deletChar,      //指纹删除
    empty,          //指纹清空
    validNum,       //忘了
    readIndex,      //读索引表
    ctrolLed,       //控制led
    vfyPwd,         //验证口令
    setPwd,         //设置口令
    abnromal        //异常
}FingerprintStatus;

typedef enum{
    UART_NO_INIT = 0x00,
    UART_INIT
}FpUartStatus;

typedef struct
{
    char pwd[4];            //口令
    char index[32];         //指纹id序列表
}FingerprintInfo;

typedef struct
{
    uint32_t workTime;      //单次工作时常
    uint32_t lockoutTime;   //锁定时间
    char abnormalTimes;     //异常次数
}FingerprintTick;


typedef struct
{
    int id;                 //本次操作id
    char opt;               //本次操作选项(对照PS_XXX宏定义)
    char ret;               //本次操作返回值(仅指纹注册和删除使用,opt每次清空,但ret不清空)
}FingerprintParam;

typedef struct
{
    /* header */
    char headr[2];          //PS_BYD_HEADER
    char addr[4];           //PS_BYD_ADDR
    char logo;
    char bodylen[2];        //包体总大小(包含校验码)
    /* body */
    char body[PS_BODY_MAX]; //消息体
}FingerprintMsg;

typedef struct{
    void (*data_handle)(void*, char*);               //指纹模块数据处理函数
    void (*init)(void*);                            //初始化指纹模块
    void (*touch)(void*);                               //指纹触摸
    int (*unpack)(void* t, char*);                      //接收消息解包
    void (*enable)(void*, bool);                        //指纹模块使能
    FingerprintStatus (*get_status)(void*);             //获取通讯状态
    FpUartStatus (*get_uart_state)(void*);             //获取通讯状态
    FingerprintParam (*special_opt)(void*, char*, int); //指纹模块特殊操作(注册,删除,验证载入和获取参数用)
    bool (*check_id)(void*, int);                       //检查该id是否被注册
    void (*clear_abnormal)(void*);                      //清除异常
    void (*tick_count)(void*);
    void (*set_pwd)(void*);                      //设置口令
    int (*set_pwd_back)(void*, char*);                  //设置口令消息处理
    void (*vfy_pwd)(void*);                             //验证口令
    int (*vfy_pwd_back)(void*, char*);                  //验证口令消息处理
    void (*identify)(void*);                            //指纹验证
    int (*identify_back)(void*, char*);                 //指纹验证消息处理
    void (*enroll)(void*);                              //指纹注册
    int (*enroll_back)(void*, char*);                   //指纹注册消息处理
    void (*read_index)(void*);                          //读索引表
    int (*read_index_back)(void*, char*);               //读索引表消息处理
    void (*delete)(void*);                              //指纹删除
    int (*delete_back)(void*, char*);                   //指纹删除消息处理
    void (*sleep)(void*);                               //休眠
    int (*sleep_back)(void*, char*);                    //休眠消息处理
    void (*cancel)(void*);                              //行为取消
    int (*cancel_back)(void*, char*);                   //行为取消消息处理
    void (*led_enable)(void*, bool);                    //led使能与失能
    int (*led_enable_back)(void*, char*);               //led使能与失能消息处理
}FINGERPRINT_IMPLEMENTS;

typedef struct{
    FINGERPRINT_IMPLEMENTS api;               //模块接口
    FingerprintInfo info;                           //模块信息
    FingerprintTick tick;                           //定时器参数
    FingerprintMsg msg;                             //发送消息存储区
    FingerprintStatus status;                       //通讯状态
    FpUartStatus uartState;                        //串口状态
    FingerprintParam lastOpt;                       //操作存储区
    void (*send)(void* t, FingerprintMsg*, int);    //消息发送
    void (*uart_init)(void*);                            //初始化指纹模块去初始化
    void (*uart_deinit)(void*);                            //初始化指纹模块去初始化
}FINGERPRINT;

/**
 * @brief 指纹对象构造函数
 * 
 * @return FINGERPRINT* 
 */
FINGERPRINT* FINGERPRINT_CTOR(void);

/**
 * @brief 指纹对象析构函数
 * 
 * @param t 
 */
void FINGERPRINT_DTOR(FINGERPRINT* t);

/**
 * @brief 给胁迫列表id号用的
 * 
 * @param list 
 * @param opt 
 * @param id 
 * @return true 
 * @return false 
 */
bool fp_coercion_id(char* list, char* opt, int id);