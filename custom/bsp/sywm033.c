#include "sywm033.h"
#include "string.h"
#include "drv_uart.h"
#include "cm_mem.h"
#include "sys.h"
#include "drv_gpio.h"

#define DBG_NAME "fp"

/**
 * @brief crc8校验函数
 * 
 * @param ptr 数据地址
 * @param len 数据长度
 * @return uint8_t 计算结果
 */
static uint8_t crc8_check(const uint8_t *ptr, uint16_t len)
{
    uint8_t i; 
    uint8_t crc = 0x00; /* 计算的初始crc值 */ 

	if (NULL == ptr || 0 == len)
	{
		return 0;
	}

    while (len--)
    {
        crc ^= *ptr++;  /* 每次先与需要计算的数据异或,计算完指向下一数据 */  
        for (i=8; i>0; --i)   /* 下面这段计算过程与计算一个字节crc一样 */  
        { 
            if (crc & 0x80)
                crc = (crc << 1) ^ 0x31;
            else
                crc = (crc << 1);
        }
    }

    return (uint32_t)crc; 
}

/**
 * @brief add16校验函数
 * 
 * @param ptr 数据地址
 * @param len 数据长度
 * @return int 和值
 */
static int add16_check(char *ptr, int len)
{
    int sum = 0;

    for(int i=0; i<len; i++){
        sum += ptr[i];
    }

    return sum;
}

/**
 * @brief 消息校验函数
 * 
 * @param data 
 * @return int 
 */
static int recode_check(char* data)
{
    int len = 0;
    int add16 = 0;

    if(data[0] != 0xEF) return 0;
    if(data[1] != 0x01) return 0;

    if(data[2] != 0xFF) return 0;
    if(data[3] != 0xFF) return 0;
    if(data[4] != 0xFF) return 0;
    if(data[5] != 0xFF) return 0;

    if(data[6] != 0x07) return 0;

    len = (int)((data[7]<<8) | data[8]);

    add16 = add16_check(&data[6], len-2+3);

    if(data[9+len-2] != ((add16>>8)&0xFF)) return 0;
    if(data[9+len-1] != ((add16)&0xFF)) return 0;

    return 1;
}

static int fp_vfy_pwd_back(void* t, char* data)
{
    FINGERPRINT* this = (FINGERPRINT*)t;

    switch(data[9])
    {
    case 0x00:
        /* 口令正确 */
        this->lastOpt.opt = PS_VfyPwd;
        this->lastOpt.ret = 0x01;
        return 0;
        break;
    case 0x01:
        /* 接收报错误 */
        this->lastOpt.opt = PS_VfyPwd;
        this->lastOpt.ret = 0xff;
        return -1;
        break;
    case 0x13:
        this->lastOpt.opt = PS_VfyPwd;
        this->lastOpt.ret = 0xff;
        return -1;
        break;
    default:
        break;
    }
    return -1;
}

/**
 * @brief 指纹模块数据发送函数
 * 
 * @param logo 
 * @param msg 
 * @param len 
 */
void fp_send(void* t, FingerprintMsg* msg, int len)
{
    int add16 = 0;
    int cmdLen = 0;
    FINGERPRINT* this = (FINGERPRINT*)t;

    cmdLen = len + PS_ADD16_LEN;

    msg->logo = PS_CMD_HEADER;
    msg->bodylen[0] = (char)(cmdLen>>8);
    msg->bodylen[1] = (char)(cmdLen);

    /* -3为去掉loge和len */
    add16 = add16_check(&msg->logo, len+3);
    msg->body[cmdLen-2] = (add16>>8) & 0xFF;
    msg->body[cmdLen-1] = (add16) & 0xFF;
    
    cm_uart_write(CM_UART_DEV_1, msg, PS_HEADER_LEN+cmdLen, 1000);
}

static int fp_unpack(void* t, char* data)
{
    if(recode_check(data) == 1){
        /* 校验通过 */
        return 1;
    }else if(data[0] == 0x55){
        /* 开机消息 */
        return 2;
    }else{
        return 0;
    }
}

static void fp_enable(void* t, bool sw)
{
    FINGERPRINT* this = (FINGERPRINT*)t;
    if(sw){
        /* 使能指纹模块供电控制引脚 */
        // if(this->uartState == UART_NO_INIT){
        //     this->uart_init(this);
        // }
        my_fp_en_sw(FP_EN_ENABLE);
    }else{
        // if(this->uartState == UART_INIT){
        //     this->uart_deinit(this);
        // }

        my_fp_en_sw(FP_EN_DISABLE);
        if(this->status != abnromal){
            this->status = idle;
        }
    }

    /* 工作时间清零 */
    this->tick.workTime = 0;
}

static void fp_touch(void* t)
{
    FINGERPRINT* this = (FINGERPRINT*)t;

    /* 手指按下后，指纹模块需要首先对口令进行校验 */
    this->status = vfyPwd;
}

static FingerprintStatus fp_get_status(void* t)
{
    FINGERPRINT* this = (FINGERPRINT*)t;

    return this->status;
}

static FpUartStatus fp_get_uart_state(void* t)
{
    FINGERPRINT* this = (FINGERPRINT*)t;

    return this->uartState;
}

bool fp_coercion_id(char* list, char* opt, int id)
{
    int integer = 0;
    int remainder = 0;

    integer = id/8;
    remainder = id%8;

    if(0 == strcmp((const char *)opt, "add")){
        list[integer] |= (1<<remainder);
        return 1;
    }else if(0 == strcmp((const char *)opt, "delete")){
        list[integer] &= ~(1<<remainder);
        return 1;
    }else if(0 == strcmp((const char *)opt, "check")){
        if(integer <= 32){
            return (bool)((list[integer]>>(remainder))&0x01);
        }else{
            return 0;
        }
    }else{
        return 0;
    }
}

static bool fp_check_id(void* t, int id)
{
    int integer = 0;
    int remainder = 0;
    FINGERPRINT* this = (FINGERPRINT*)t;

    integer = id/8;
    remainder = id%8;
    
    if(integer <= 32){
        return (bool)((this->info.index[integer]>>(remainder))&0x01);
    }else{
        return 0;
    }
}

static void fp_clear_abnormal(void* t)
{
    FINGERPRINT* this = (FINGERPRINT*)t;

    this->tick.lockoutTime = 0;
    this->tick.abnormalTimes = 0;
    if(this->status == abnromal){
        this->status = idle;
    }
}

/**
 * @brief 指纹模块特殊操作（指纹删除和注册的异步数据保存和获取）
 * 
 * @param t 
 * @param act 
 *    =  load  - 载入参数 \n
 *    =  query - 查询参数
 * @param id 指纹id
 * @return FingerprintParam 
 */
static FingerprintParam fp_special_opt(void* t, char* act, int id)
{
    FingerprintParam ret;
    FINGERPRINT* this = (FINGERPRINT*)t;

    if(0 == strcmp((const char *)act, "load")){
        this->lastOpt.id = id; //清空指纹
        this->lastOpt.opt = 0x00;
        this->lastOpt.ret = 0x00;
    }else if(0 == strcmp((const char *)act, "query")){
        ret.id = this->lastOpt.id;
        ret.opt = this->lastOpt.opt;
        ret.ret = this->lastOpt.ret;
    }

    return ret;
}

static void fp_set_pwd(void* t)
{
    FINGERPRINT* this = (FINGERPRINT*)t;

    this->status = setPwd;

    // memcpy(this->info.pwd, pwd, 4);

    this->msg.body[0] = PS_SetPwd;

    memcpy(&this->msg.body[1], this->info.pwd, 4);

    this->send(this, &this->msg, 5);
}

static int fp_set_pwd_back(void* t, char* data)
{
    switch(data[9])
    {
    case 0x00:
        /* 口令设置成功 */
        return 0;
        break;
    case 0x01:
        /* 接收报错误 */
        return -1;
        break;
    default:
        break;
    }
    return -1;
}

/**
 * @brief 
 * 
 * @param t 
 */
static void fp_vfy_pwd(void* t)
{
    FINGERPRINT* this = (FINGERPRINT*)t;

    this->status = vfyPwd;

    this->msg.body[0] = PS_VfyPwd;

    this->msg.body[1] = this->info.pwd[0];
    this->msg.body[2] = this->info.pwd[1];
    this->msg.body[3] = this->info.pwd[2];
    this->msg.body[4] = this->info.pwd[3];

    this->send(this, &this->msg, 5);
}

/**
 * @brief 指纹检测函数
 * 
 * @param t 
 */
static void fp_identify(void* t)
{
    FINGERPRINT* this = (FINGERPRINT*)t;

    this->status = identify;

    this->msg.body[0] = PS_AutoIdentify;

    this->msg.body[1] = 0x03; //安全等级
    
    this->msg.body[2] = 0xFF;
    this->msg.body[3] = 0xFF; //ID号

    this->msg.body[4] = 0x02;
    this->msg.body[5] = 0x00; //参数


    this->send(this, &this->msg, 6);
}

/**
 * @brief 指纹检测消息处理函数
 * 
 * @param t 
 * @param data 
 * @return int 
 *    = -1 - 错误 \n
 *    =  0 - 正确 \n
 *    =  1 - 等待 \n
 *    =  2 - 超时
 */
static int fp_identify_back(void* t, char* data)
{
    FINGERPRINT* this = (FINGERPRINT*)t;

    this->lastOpt.opt = PS_AutoIdentify;

    switch(data[10]){
    case 0x00:
        /* 后面还有消息 */
        return 1;
        break;
    case 0x01:
        /* 指纹验证是否超时 */
        if(data[9] != 0x00){
            /* 超时 */
            this->lastOpt.ret = 0x26;
            return 2;
        }else{
            return 1;
        }
        break;
    case 0x05:
        /* 结果判断 */
        if(data[9] != 0x00){
            /* 指纹错误 */
            this->lastOpt.ret = 0xff;
            ++ this->tick.abnormalTimes;
            if(this->tick.abnormalTimes > FP_ABNORMAL_LIMIT_TIMES){
                this->status = abnromal;
            }
            return -1;
        }else{
            /* 指纹正确 */
            this->lastOpt.id = (int)((data[11]<<8) | data[12]);
            this->lastOpt.ret = 0x01;
            this->tick.abnormalTimes = 0;
            return 0;
        }
        break;
    default:
        break;
    }
    
    return -1;
}

static void fp_enroll(void* t)
{
    FINGERPRINT* this = (FINGERPRINT*)t;

    this->status = enroll;

    /* 目前限制为256个 */
    if(0<=this->lastOpt.id && this->lastOpt.id<255){
        this->lastOpt.opt = PS_AutoEnroll;
        this->lastOpt.ret = 0x00;

        this->msg.body[0] = PS_AutoEnroll;

        this->msg.body[1] = (this->lastOpt.id>>8) & 0xFF;
        this->msg.body[2] = this->lastOpt.id & 0xFF; //id
        this->msg.body[3] = 0x02; //录入次数
        this->msg.body[4] = 0x00;
        this->msg.body[5] = 0x78; //参数

        this->send(this, &this->msg, 6);
    }else{
        this->lastOpt.opt = PS_AutoEnroll;
        this->lastOpt.ret = 0xff;
    }
}

static int fp_enroll_back(void* t, char* data)
{
    FINGERPRINT* this = (FINGERPRINT*)t;

    if(data[9] != 0x00){
        /* 指纹录入失败 */
        this->lastOpt.opt = PS_AutoEnroll;
        if(data[9] == 0x26){
            /* 指纹录入超时 */
            this->lastOpt.ret = 0x26;
        }else if(data[9] == 0x27){
            /* 指纹已存在 */
            this->lastOpt.ret = 0x26;
        }else{
            /* 其他不重要的错误 */
            this->lastOpt.ret = 0xff;
        }
        return -1;
    }

    switch(data[11])
    {
    case 0x00:
        break;
    case 0x01:
        break;
    case 0x02:
        break;
    case 0xF0:
        break;
    case 0xF1:
        break;
    case 0xF2:
        /* 指纹录入成功 */
        this->lastOpt.opt = PS_AutoEnroll;
        this->lastOpt.ret = 0x01;
        return 0;
        break;
    default:
        break;
    }

    return 1;
}

static void fp_read_index(void* t)
{
    FINGERPRINT* this = (FINGERPRINT*)t;

    this->status = readIndex;

    this->msg.body[0] = PS_ReadIndexTable;
    this->msg.body[1] = 0x00;

    this->send(this, &this->msg, 2);
}

static int fp_read_index_back(void* t, char* data)
{
    FINGERPRINT* this = (FINGERPRINT*)t;

    switch(data[9])
    {
    case 0x00:
        /* 成功读到索引表 */
        memcpy(this->info.index, &data[10], 32);
        return 0;
        break;
    case 0x01:
        return -1;
        break;
    default:
        break;
    }

    return 0;
}

static void fp_delete(void* t)
{
    FINGERPRINT* this = (FINGERPRINT*)t;

    if(0<=this->lastOpt.id && this->lastOpt.id<=255){
        /* 删除单个指纹 */
        this->status = deletChar;

        this->lastOpt.opt = PS_DeletChar;
        this->lastOpt.ret = 0x00;

        this->msg.body[0] = PS_DeletChar;

        this->msg.body[1] = (this->lastOpt.id>>8) & 0xFF;
        this->msg.body[2] = this->lastOpt.id & 0xFF;
        this->msg.body[3] = 0x00;
        this->msg.body[4] = 0x01;

        this->send(this, &this->msg, 5);
    }else if(this->lastOpt.id == 65535){
        /* 清空指纹库 */
        this->status = deletChar;

        this->lastOpt.opt = PS_DeletChar;
        this->lastOpt.ret = 0x00;

        this->msg.body[0] = PS_Empty;
    
        this->send(this, &this->msg, 1);
    }else{
        this->lastOpt.opt = PS_DeletChar;
        this->lastOpt.ret = 0xff;
    }
}

static int fp_delete_back(void* t, char* data)
{
    FINGERPRINT* this = (FINGERPRINT*)t;
    switch (data[9])
    {
    case 0x00:
        /* 指纹删除成功 */
        this->lastOpt.opt = PS_DeletChar;
        this->lastOpt.ret = 0x01;
        return 0;
        break;
    case 0x01:
        this->lastOpt.opt = PS_DeletChar;
        this->lastOpt.ret = 0xff;
        return -1;
        break;
    case 0x10:
        this->lastOpt.opt = PS_DeletChar;
        this->lastOpt.ret = 0xff;
        return -1;
        break;
    case 0x11:
        this->lastOpt.opt = PS_Empty;
        this->lastOpt.ret = 0xff;
        return -1;
        break;
    default:
        break;
    }

    return 1;
}

static void fp_sleep(void* t)
{
    FINGERPRINT* this = (FINGERPRINT*)t;

    this->status = idle;

    this->msg.body[0] = PS_Sleep;
    this->send(this, &this->msg, 1);
}

static int fp_sleep_back(void* t, char* data)
{
    return 0;
}

static void fp_canncel(void* t)
{
    FINGERPRINT* this = (FINGERPRINT*)t;

    this->status = idle;

    this->msg.body[0] = PS_Cancel;
    
    this->send(this, &this->msg, 1);
}

static int fp_canncel_back(void* t, char* data)
{
    return 0;
}

static void fp_led_enable(void* t, bool sw)
{
    FINGERPRINT* this = (FINGERPRINT*)t;

    this->status = ctrolLed;

    this->msg.body[0] = PS_Ctrl_LED;

    if(sw){
        /* 开 */
        this->msg.body[1] = 0xF6;
    }else{
        this->msg.body[1] = 0xF5;
    }
    this->msg.body[2] = 0x00;
    this->msg.body[3] = 0x00;
    this->msg.body[4] = 0x00;

    this->send(this, &this->msg, 5);
}

static int fp_led_enable_back(void* t, char* data)
{
    return 0;
}

static void fp_tick_count(void* t)
{
    FINGERPRINT* this = (FINGERPRINT*)t;

    if(this->status == idle && my_fp_en_get()) return;

    if(this->status == abnromal){
        ++ this->tick.lockoutTime;
        if(this->tick.lockoutTime >= FP_LOCK_TIME_MAX){
            /* 清除异常 */
            this->interface.clear_abnormal(this);
        }
    }else{
        ++ this->tick.workTime;
        if(this->tick.workTime >= FP_WORK_TIME_MAX){
            /* 关闭指纹模块 */
            this->interface.enable(this, 0);
        }
    }
}

static void fp_uart_init(void* t){
    FINGERPRINT* this = (FINGERPRINT*)t;

    uart_open(FP_UART, 57600, u1_callback);

    this->uartState = UART_INIT;
}

static void fp_uart_deinit(void* t){
    FINGERPRINT* this = (FINGERPRINT*)t;

    this->uartState = UART_NO_INIT;
    cm_uart_close(FP_UART);
}



static void fp_init(void* t)
{
    FINGERPRINT* this = (FINGERPRINT*)t;

    my_fp_gpio_init();

    memset(&this->msg, 0, sizeof(this->msg));
    this->msg.headr[0] = 0xEF;
    this->msg.headr[1] = 0x01;
    this->msg.addr[0] = 0xFF;
    this->msg.addr[1] = 0xFF;
    this->msg.addr[2] = 0xFF;
    this->msg.addr[3] = 0xFF;
    this->status = idle;
}

FINGERPRINT* FINGERPRINT_CTOR(void)
{
    FINGERPRINT *this = (FINGERPRINT*)cm_malloc(sizeof(FINGERPRINT));

    /* init api */
    this->send = fp_send;  //指纹模块消息发送
    this->uart_init = fp_uart_init;  //指纹串口初始化
    this->uart_deinit = fp_uart_deinit;  //指纹串口去初始化

    this->interface.init = fp_init;  //指纹模块初始化
    this->interface.touch = fp_touch; //指纹触摸
    this->interface.unpack = fp_unpack; //接收消息解包
    this->interface.enable = fp_enable; //指纹模块使能
    this->interface.get_status =fp_get_status; //获取指纹模块通讯状态
    this->interface.special_opt = fp_special_opt; //指纹模块特殊操作（指纹删除和注册的异步数据保存和获取）
    this->interface.check_id = fp_check_id; //检查该id是否被注册
    this->interface.clear_abnormal = fp_clear_abnormal; //清除异常
    this->interface.tick_count = fp_tick_count; //计时
    this->interface.get_uart_state = fp_get_uart_state; //计时

    this->interface.set_pwd = fp_set_pwd; //设置口令
    this->interface.set_pwd_back = fp_set_pwd_back; //设置口令消息处理

    this->interface.vfy_pwd = fp_vfy_pwd; //验证口令
    this->interface.vfy_pwd_back = fp_vfy_pwd_back; //口令消息处理

    this->interface.identify = fp_identify; //指纹验证
    this->interface.identify_back = fp_identify_back; //指纹验证消息处理

    this->interface.enroll = fp_enroll; //指纹注册
    this->interface.enroll_back = fp_enroll_back; //指纹注册消息处理
    
    this->interface.read_index = fp_read_index; //指纹读索引表
    this->interface.read_index_back = fp_read_index_back; //指纹读索引表消息处理

    this->interface.delete = fp_delete; //指纹删除
    this->interface.delete_back = fp_delete_back; //指纹删除消息处理

    this->interface.sleep = fp_sleep; //指纹模块休眠
    this->interface.sleep_back = fp_sleep_back; //指纹模块休眠消息处理

    this->interface.cancel = fp_canncel; //指纹模块行为取消
    this->interface.cancel_back = fp_canncel_back; //指纹模块行为取消

    this->interface.led_enable = fp_led_enable; //led使能与失能
    this->interface.led_enable_back = fp_led_enable_back; //led使能与失能

    return (FINGERPRINT*)this;
}

void FINGERPRINT_DTOR(FINGERPRINT* t)
{
    /* 尽量不使用，构造一次不要释放，避免多次释放造成内存碎片化 */
    cm_free(t);
}
