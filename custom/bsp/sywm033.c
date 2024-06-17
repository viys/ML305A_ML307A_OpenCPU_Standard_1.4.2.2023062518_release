#include "sywm033.h"
#include "cm_mem.h"
#include "drv_gpio.h"
#include "drv_uart.h"
#include "string.h"
#include "sys.h"
#include "drv_audio.h"
#include "app.h"
#include "cJSON.h"

#define DBG_NAME "fp"

/**
 * @brief crc8校验函数
 *
 * @param ptr 数据地址
 * @param len 数据长度
 * @return uint8_t 计算结果
 */
static uint8_t crc8_check(const uint8_t* ptr, uint16_t len) {
    uint8_t i;
    uint8_t crc = 0x00; /* 计算的初始crc值 */

    if (NULL == ptr || 0 == len) {
        return 0;
    }

    while (len--) {
        crc ^= *ptr++; /* 每次先与需要计算的数据异或,计算完指向下一数据 */
        for (i = 8; i > 0; --i) /* 下面这段计算过程与计算一个字节crc一样 */
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
static int add16_check(char* ptr, int len) {
    int sum = 0;

    for (int i = 0; i < len; i++) {
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
static int recode_check(char* data) {
    int len = 0;
    int add16 = 0;

    if (data[0] != 0xEF)
        return 0;
    if (data[1] != 0x01)
        return 0;

    if (data[2] != 0xFF)
        return 0;
    if (data[3] != 0xFF)
        return 0;
    if (data[4] != 0xFF)
        return 0;
    if (data[5] != 0xFF)
        return 0;

    if (data[6] != 0x07)
        return 0;

    len = (int)((data[7] << 8) | data[8]);

    add16 = add16_check(&data[6], len - 2 + 3);

    if (data[9 + len - 2] != ((add16 >> 8) & 0xFF))
        return 0;
    if (data[9 + len - 1] != ((add16) & 0xFF))
        return 0;

    return 1;
}

static int fp_vfy_pwd_back(void* t, char* data) {
    FINGERPRINT* this = (FINGERPRINT*)t;

    switch (data[9]) {
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
void fp_send(void* t, FingerprintMsg* msg, int len) {
    int add16 = 0;
    int cmdLen = 0;
    FINGERPRINT* this = (FINGERPRINT*)t;

    cmdLen = len + PS_ADD16_LEN;

    msg->logo = PS_CMD_HEADER;
    msg->bodylen[0] = (char)(cmdLen >> 8);
    msg->bodylen[1] = (char)(cmdLen);

    /* -3为去掉loge和len */
    add16 = add16_check(&msg->logo, len + 3);
    msg->body[cmdLen - 2] = (add16 >> 8) & 0xFF;
    msg->body[cmdLen - 1] = (add16) & 0xFF;

    cm_uart_write(CM_UART_DEV_1, msg, PS_HEADER_LEN + cmdLen, 1000);
}

static int fp_unpack(void* t, char* data) {
    if (recode_check(data) == 1) {
        /* 校验通过 */
        return 1;
    } else if (data[0] == 0x55) {
        /* 开机消息 */
        return 2;
    } else {
        return 0;
    }
}

static void fp_enable(void* t, bool sw) {
    FINGERPRINT* this = (FINGERPRINT*)t;
    if (sw) {
        /* 使能指纹模块供电控制引脚 */
        // if(this->uartState == UART_NO_INIT){
        //     this->uart_init(this);
        // }
        my_fp_en_sw(FP_EN_ENABLE);
    } else {
        // if(this->uartState == UART_INIT){
        //     this->uart_deinit(this);
        // }

        my_fp_en_sw(FP_EN_DISABLE);
        if (this->status != abnromal) {
            this->status = idle;
        }
    }

    /* 工作时间清零 */
    this->tick.workTime = 0;
}

static void fp_touch(void* t) {
    FINGERPRINT* this = (FINGERPRINT*)t;

    /* 手指按下后，指纹模块需要首先对口令进行校验 */
    this->status = vfyPwd;
}

static FingerprintStatus fp_get_status(void* t) {
    FINGERPRINT* this = (FINGERPRINT*)t;

    return this->status;
}

static FpUartStatus fp_get_uart_state(void* t) {
    FINGERPRINT* this = (FINGERPRINT*)t;

    return this->uartState;
}

bool fp_coercion_id(char* list, char* opt, int id) {
    int integer = 0;
    int remainder = 0;

    integer = id / 8;
    remainder = id % 8;

    if (0 == strcmp((const char*)opt, "add")) {
        list[integer] |= (1 << remainder);
        return 1;
    } else if (0 == strcmp((const char*)opt, "delete")) {
        list[integer] &= ~(1 << remainder);
        return 1;
    } else if (0 == strcmp((const char*)opt, "check")) {
        if (integer <= 32) {
            return (bool)((list[integer] >> (remainder)) & 0x01);
        } else {
            return 0;
        }
    } else {
        return 0;
    }
}

static bool fp_check_id(void* t, int id) {
    int integer = 0;
    int remainder = 0;
    FINGERPRINT* this = (FINGERPRINT*)t;

    integer = id / 8;
    remainder = id % 8;

    if (integer <= 32) {
        return (bool)((this->info.index[integer] >> (remainder)) & 0x01);
    } else {
        return 0;
    }
}

static void fp_clear_abnormal(void* t) {
    FINGERPRINT* this = (FINGERPRINT*)t;

    this->tick.lockoutTime = 0;
    this->tick.abnormalTimes = 0;
    if (this->status == abnromal) {
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
static FingerprintParam fp_special_opt(void* t, char* act, int id) {
    FingerprintParam ret;
    FINGERPRINT* this = (FINGERPRINT*)t;

    if (0 == strcmp((const char*)act, "load")) {
        this->lastOpt.id = id;  // 清空指纹
        this->lastOpt.opt = 0x00;
        this->lastOpt.ret = 0x00;
    } else if (0 == strcmp((const char*)act, "query")) {
        ret.id = this->lastOpt.id;
        ret.opt = this->lastOpt.opt;
        ret.ret = this->lastOpt.ret;
    }

    return ret;
}

static void fp_set_pwd(void* t) {
    FINGERPRINT* this = (FINGERPRINT*)t;

    this->status = setPwd;

    // memcpy(this->info.pwd, pwd, 4);

    this->msg.body[0] = PS_SetPwd;

    memcpy(&this->msg.body[1], this->info.pwd, 4);

    this->send(this, &this->msg, 5);
}

static int fp_set_pwd_back(void* t, char* data) {
    switch (data[9]) {
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
static void fp_vfy_pwd(void* t) {
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
static void fp_identify(void* t) {
    FINGERPRINT* this = (FINGERPRINT*)t;

    this->status = identify;

    this->msg.body[0] = PS_AutoIdentify;

    this->msg.body[1] = 0x03;  // 安全等级

    this->msg.body[2] = 0xFF;
    this->msg.body[3] = 0xFF;  // ID号

    this->msg.body[4] = 0x02;
    this->msg.body[5] = 0x00;  // 参数

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
static int fp_identify_back(void* t, char* data) {
    FINGERPRINT* this = (FINGERPRINT*)t;

    this->lastOpt.opt = PS_AutoIdentify;

    switch (data[10]) {
        case 0x00:
            /* 后面还有消息 */
            return 1;
            break;
        case 0x01:
            /* 指纹验证是否超时 */
            if (data[9] != 0x00) {
                /* 超时 */
                this->lastOpt.ret = 0x26;
                return 2;
            } else {
                return 1;
            }
            break;
        case 0x05:
            /* 结果判断 */
            if (data[9] != 0x00) {
                /* 指纹错误 */
                this->lastOpt.ret = 0xff;
                ++this->tick.abnormalTimes;
                if (this->tick.abnormalTimes > FP_ABNORMAL_LIMIT_TIMES) {
                    this->status = abnromal;
                }
                return -1;
            } else {
                /* 指纹正确 */
                this->lastOpt.id = (int)((data[11] << 8) | data[12]);
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

static void fp_enroll(void* t) {
    FINGERPRINT* this = (FINGERPRINT*)t;

    this->status = enroll;

    /* 目前限制为256个 */
    if (0 <= this->lastOpt.id && this->lastOpt.id < 255) {
        this->lastOpt.opt = PS_AutoEnroll;
        this->lastOpt.ret = 0x00;

        this->msg.body[0] = PS_AutoEnroll;

        this->msg.body[1] = (this->lastOpt.id >> 8) & 0xFF;
        this->msg.body[2] = this->lastOpt.id & 0xFF;  // id
        this->msg.body[3] = 0x02;                     // 录入次数
        this->msg.body[4] = 0x00;
        this->msg.body[5] = 0x78;  // 参数

        this->send(this, &this->msg, 6);
    } else {
        this->lastOpt.opt = PS_AutoEnroll;
        this->lastOpt.ret = 0xff;
    }
}

static int fp_enroll_back(void* t, char* data) {
    FINGERPRINT* this = (FINGERPRINT*)t;

    if (data[9] != 0x00) {
        /* 指纹录入失败 */
        this->lastOpt.opt = PS_AutoEnroll;
        if (data[9] == 0x26) {
            /* 指纹录入超时 */
            this->lastOpt.ret = 0x26;
        } else if (data[9] == 0x27) {
            /* 指纹已存在 */
            this->lastOpt.ret = 0x26;
        } else {
            /* 其他不重要的错误 */
            this->lastOpt.ret = 0xff;
        }
        return -1;
    }

    switch (data[11]) {
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

static void fp_read_index(void* t) {
    FINGERPRINT* this = (FINGERPRINT*)t;

    this->status = readIndex;

    this->msg.body[0] = PS_ReadIndexTable;
    this->msg.body[1] = 0x00;

    this->send(this, &this->msg, 2);
}

static int fp_read_index_back(void* t, char* data) {
    FINGERPRINT* this = (FINGERPRINT*)t;

    DBG_I("Read success %02x\r\n", data[9]);

    switch (data[9]) {
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

static void fp_delete(void* t) {
    FINGERPRINT* this = (FINGERPRINT*)t;

    if (0 <= this->lastOpt.id && this->lastOpt.id <= 255) {
        /* 删除单个指纹 */
        this->status = deletChar;

        this->lastOpt.opt = PS_DeletChar;
        this->lastOpt.ret = 0x00;

        this->msg.body[0] = PS_DeletChar;

        this->msg.body[1] = (this->lastOpt.id >> 8) & 0xFF;
        this->msg.body[2] = this->lastOpt.id & 0xFF;
        this->msg.body[3] = 0x00;
        this->msg.body[4] = 0x01;

        this->send(this, &this->msg, 5);
    } else if (this->lastOpt.id == 65535) {
        /* 清空指纹库 */
        this->status = deletChar;

        this->lastOpt.opt = PS_DeletChar;
        this->lastOpt.ret = 0x00;

        this->msg.body[0] = PS_Empty;

        this->send(this, &this->msg, 1);
    } else {
        this->lastOpt.opt = PS_DeletChar;
        this->lastOpt.ret = 0xff;
    }
}

static int fp_delete_back(void* t, char* data) {
    FINGERPRINT* this = (FINGERPRINT*)t;

    switch (data[9]) {
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

static void fp_sleep(void* t) {
    FINGERPRINT* this = (FINGERPRINT*)t;

    this->status = idle;

    this->msg.body[0] = PS_Sleep;
    this->send(this, &this->msg, 1);
}

static int fp_sleep_back(void* t, char* data) {
    return 0;
}

static void fp_canncel(void* t) {
    FINGERPRINT* this = (FINGERPRINT*)t;

    this->status = idle;

    this->msg.body[0] = PS_Cancel;

    this->send(this, &this->msg, 1);
}

static int fp_canncel_back(void* t, char* data) {
    return 0;
}

static void fp_led_enable(void* t, bool sw) {
    FINGERPRINT* this = (FINGERPRINT*)t;

    this->status = ctrolLed;

    this->msg.body[0] = PS_Ctrl_LED;

    if (sw) {
        /* 开 */
        this->msg.body[1] = 0xF6;
    } else {
        this->msg.body[1] = 0xF5;
    }
    this->msg.body[2] = 0x00;
    this->msg.body[3] = 0x00;
    this->msg.body[4] = 0x00;

    this->send(this, &this->msg, 5);
}

static int fp_led_enable_back(void* t, char* data) {
    return 0;
}

static void fp_tick_count(void* t) {
    FINGERPRINT* this = (FINGERPRINT*)t;

    if (this->status == idle && my_fp_en_get())
        return;

    if (this->status == abnromal) {
        ++this->tick.lockoutTime;
        if (this->tick.lockoutTime >= FP_LOCK_TIME_MAX) {
            /* 清除异常 */
            this->api.clear_abnormal(this);
        }
    } else {
        ++this->tick.workTime;
        if (this->tick.workTime >= FP_WORK_TIME_MAX) {
            /* 关闭指纹模块 */
            this->api.enable(this, 0);
        }
    }
}

static void fp_uart_init(void* t) {
    FINGERPRINT* this = (FINGERPRINT*)t;

    uart_open(FP_UART, 57600, u1_callback);

    this->uartState = UART_INIT;
}

static void fp_uart_deinit(void* t) {
    FINGERPRINT* this = (FINGERPRINT*)t;

    this->uartState = UART_NO_INIT;
    cm_uart_close(FP_UART);
}

static void fp_init(void* t) {
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

static int fp_rev_handle(void* t, char* data) {
    FINGERPRINT* this = (FINGERPRINT*)t;
    FingerprintParam param;
    FingerPrint_Res res = {
        .results = NULL,
        .opt = NULL,
        .id = 255
    };
//    DBG_W("%d ", this->api.get_status(this));
    switch (this->api.get_status(this)) {
        case vfyPwd:
            switch (this->api.vfy_pwd_back(this, data)) {
                case 0:
                    // 口令正确
                    DBG_F("VfyPwd success\r\n");

                    // Event_post(fpSyncEvent, SP_FP_IDENTIFY);
                    this->api.identify(this);

                    break;
                case -1:
                    // Event_post(thisSyncEvent, SP_FP_SLEEP);
                    DBG_F("VfyPwd error sleep\r\n");
                    this->api.enable(this, 0);
                default:
                    break;
            }
            break;

        case identify:
            switch (this->api.identify_back(this, data)) {
                case -1:
                    /* 指纹错误 */
                    // Event_post(thisSyncEvent, SP_FP_SLEEP);
                    DBG_E("Identify err sleep\r\n");
                    my_ringtone_play(MP3_VFY_ERR_PATH);
                    osDelayMs(200);
                    this->api.enable(this, 0);
                    break;
                case 0:
                    /* 指纹正确 */
                    param = this->api.special_opt(this, "query", 0);
                    DBG_F("Indentify success (%d) sleep\r\n", param.id);
                    char mode[10];
                    sprintf(mode, "fp:%d", param.id);
                    osDelayMs(200);
                    /* 记录开锁人id和方式 */
                    /* 执行开锁动作 */
                    DBG_F("Open door\r\n");
                    lock->load(lock, NULL, NULL, NULL);
                    lock->load(lock, NULL, mode, NULL);
//                    my_ringtone_play(MP3_OPENLOCK_PATH);
                    void* msg = cm_malloc(1);
                    ((uint8_t*)msg)[0] = 0x01;  // 对应通道宏定义
                    osMessageQueuePut(lock_queue, &msg, 0, 0);
                    this->api.enable(this, 0);
                    break;
                case 1:
                    break;
                case 2:
                    /* 超时 */
                    DBG_F("Identify timeout sleep\r\n");
                    this->api.enable(this, 0);
                    break;
                default:
                    break;
            }
            break;

        case idle:
            DBG_F("%s idle\r\n", __FUNCTION__);
            break;

        case cancel:
            DBG_F("%s cancel sleep", __FUNCTION__);
            this->api.enable(this, 0);
            break;

        case ctrolLed:
            DBG_F("%s ctrolLed sleep", __FUNCTION__);
            res.opt = "ctrolLed";
            res.results = "true";
            this->api.enable(this, 0);
        break;

        case readIndex:
            switch (this->api.read_index_back(this, data)) {
                case 0:
                    /* 读成功 */
                    DBG_UART("List:\r\n");
                    for (int i = 0; i < 255; i++) {
                        DBG_UART("[%03d] ", i);
                        DBG_UART("%d ", this->api.check_id(this, i));
                        if (i % 16 == 15) {
                            DBG_UART("\r\n");
                        }
                    }
                    DBG_UART("\r\n");
                    DBG_F("ReadIndex success sleep\r\n");
                    this->api.enable(this, 0);
                    res.opt = "readIndex";
                    res.results = "true";
                    break;
                case -1:
                    DBG_F("ReadIndex failed sleep\r\n");
                    this->api.enable(this, 0);
                    /* 读失败 */
                    DBG_F("ReadIndex success sleep\r\n");
                    this->api.enable(this, 0);
                    res.opt = "readIndex";
                    res.results = "false";
                    break;
                default:
                    break;
            }
            break;

        case enroll:
            switch (this->api.enroll_back(this, data)) {
                case -1:
                    /* 指纹录入失败 */
                    DBG_F("%s enroll failed (%d)", __FUNCTION__,
                          this->lastOpt.id);
                    osDelayMs(1000);
                    this->api.enable(this, 0);
                    res.opt = "enroll";
                    res.results = "false";
                    res.id = this->lastOpt.id;
                    {
                        cJSON* root = cJSON_CreateObject();

                        cJSON_AddStringToObject(root, "deviceId", "n001");
                        cJSON_AddStringToObject(root, "requestId", "wechat");
                        cJSON_AddBoolToObject(root, "deviceType", 0);

                        cJSON* info = cJSON_CreateObject();
                        cJSON_AddStringToObject(info, "fpOpt", "enroll");
                        cJSON_AddNumberToObject(info, "fpID", res.id);
                        cJSON_AddStringToObject(info, "fpRet", "false");

                        cJSON_AddItemToObject(root, "deviceInfo", info);

                        void* msg = cm_malloc(sizeof(64));
                        msg = (void*)cJSON_PrintUnformatted(root);

                        osMessageQueuePut(mqtt_send_queue, &msg, 0, 0);

                        cJSON_Delete(root);
                    }
                    break;
                case 0:
                    /* 指纹录入成功 */
                    DBG_F("%s enroll success (%d)\r\n", __FUNCTION__,
                          this->lastOpt.id);
                    // Event_post(thisSyncEvent, SP_FP_SLEEP);
                    this->status = setPwd;
//                    this->info.pwd[0] = bleCode[0];
//                    this->info.pwd[1] = bleCode[1];
//                    this->info.pwd[2] = bleCode[2];
//                    this->info.pwd[3] = bleCode[3];
                    this->api.set_pwd(this);
//                    thisStart = 0;
                    osDelayMs(1000);
                    this->api.enable(this, 0);
                    res.opt = "enroll";
                    res.results = "true";
                    res.id = this->lastOpt.id;
                     this->api.read_index(this);
                     {
                         cJSON* root = cJSON_CreateObject();

                         cJSON_AddStringToObject(root, "deviceId", "n001");
                         cJSON_AddStringToObject(root, "requestId", "wechat");
                         cJSON_AddBoolToObject(root, "deviceType", 0);

                         cJSON* info = cJSON_CreateObject();
                         cJSON_AddStringToObject(info, "fpOpt", "enroll");
                         cJSON_AddNumberToObject(info, "fpID", res.id);
                         cJSON_AddStringToObject(info, "fpRet", "true");

                         cJSON_AddItemToObject(root, "deviceInfo", info);

                         void* msg = cm_malloc(sizeof(64));
                         msg = (void*)cJSON_PrintUnformatted(root);

                         osMessageQueuePut(mqtt_send_queue, &msg, 0, 0);

                         cJSON_Delete(root);
                     }
                    break;
                case 1:
                    break;
                default:
                    break;
            }
            break;
        case deletChar:
            switch (this->api.delete_back(this, data)) {
                case 0:
                    /* 删除成功 */
                    DBG_F("%s deletChar success (%d)\r\n", __FUNCTION__,
                          this->lastOpt.id);
                    res.opt = "delete";
                    res.results = "ture";
                    res.id = this->lastOpt.id;
                    {
                        cJSON* root = cJSON_CreateObject();

                        cJSON_AddStringToObject(root, "deviceId", "n001");
                        cJSON_AddStringToObject(root, "requestId", "wechat");
                        cJSON_AddBoolToObject(root, "deviceType", 0);

                        cJSON* info = cJSON_CreateObject();
                        cJSON_AddStringToObject(info, "fpOpt", "delete");
                        cJSON_AddNumberToObject(info, "fpID", res.id);
                        cJSON_AddStringToObject(info, "fpRet", "true");

                        cJSON_AddItemToObject(root, "deviceInfo", info);

                        void* msg = cm_malloc(sizeof(64));
                        msg = (void*)cJSON_PrintUnformatted(root);

                        osMessageQueuePut(mqtt_send_queue, &msg, 0, 0);

                        cJSON_Delete(root);
                    }
                    break;
                case -1:
                    /* 删除失败 */
                    DBG_F("%s deletChar failed (%d)\r\n", __FUNCTION__,
                          this->lastOpt.id);
                    res.opt = "delete";
                    res.results = "false";
                    res.id = this->lastOpt.id;
                    break;
                default:
                    DBG_F("%s deletChar default (0x%02x)\r\n", __FUNCTION__,
                          data[9]);
                    break;
            }
            //	 	DBG_F("read_index after delete");
            // this->apiread_index(this);
            //	 	Event_post(thisSyncEvent, SP_FP_SLEEP);
            //	 	thisStart = 0;
            //	 	 Event_post(thisSyncEvent, SP_FP_SLEEP);
            this->api.enable(this, 0);
            break;

             case setPwd:
             	switch(this->api.set_pwd_back(this, data))
             	{
             	case -1:
             		DBG_F("%s setPwd failed sleep", __FUNCTION__);
             		break;
             	case 0:
             		DBG_F("%s setPwd success sleep", __FUNCTION__);
             		break;
             	default:
             		break;
             	}
                this->api.enable(this, 0);
             	break;

        default:
            break;
    }
    return 0;
}

static void fp_date_handle(void* t, char* data) {
    FINGERPRINT* this = (FINGERPRINT*)t;

    switch (this->api.unpack(this, data)) {
        case 0:

            if (this->api.get_status(this) == readIndex) {
                fp_rev_handle(this, data);
//                DBG_UART("data:\r\n");
//                for (int i=0; i<64; i++) {
//                    DBG_UART("[%d]%02x ", i, data[i]);
//                }
//                DBG_UART("\r\n");
            } else {
                osDelayMs(200);
                DBG_E("Protocol err! sleep\r\n");
            }
            break;
        case 1:
            /* 校验成功 */
            DBG_F("FP[%d] handle get date\r\n", this->status);
            fp_rev_handle(this, data);
            break;
        case 2:
            /* 读到开机消息 */
            switch (this->api.get_status(this)) {
                case vfyPwd:
                    this->api.vfy_pwd(this);
                    break;
                case enroll:
                    this->api.enroll(this);
                    break;
                case deletChar:
                    DBG_E("start delete\r\n");
                    this->api.delete(this);
                    break;
                case readIndex:
                    DBG_F("Read index\r\n");
                    this->api.read_index(this);
                    break;
                case setPwd:
                    this->api.set_pwd(this);
                    break;
                case idle:
                    DBG_E("FP ERR! sleep\r\n");
                    break;
                case ctrolLed:
                    break;
                case identify:
                    this->api.identify(this);
                    break;
                default:
                    break;
            }
            break;
        default:
            break;
    }
}

FINGERPRINT* FINGERPRINT_CTOR(void) {
    FINGERPRINT* this = (FINGERPRINT*)cm_malloc(sizeof(FINGERPRINT));

    /* init api */
    this->send = fp_send;                // 指纹模块消息发送
    this->uart_init = fp_uart_init;      // 指纹串口初始化
    this->uart_deinit = fp_uart_deinit;  // 指纹串口去初始化

    this->api.data_handle = fp_date_handle;  // 指纹模块返回数据处理函数
    this->api.init = fp_init;                // 指纹模块初始化
    this->api.touch = fp_touch;              // 指纹触摸
    this->api.unpack = fp_unpack;            // 接收消息解包
    this->api.enable = fp_enable;            // 指纹模块使能
    this->api.get_status = fp_get_status;  // 获取指纹模块通讯状态
    this->api.special_opt =
        fp_special_opt;  // 指纹模块特殊操作（指纹删除和注册的异步数据保存和获取）
    this->api.check_id = fp_check_id;              // 检查该id是否被注册
    this->api.clear_abnormal = fp_clear_abnormal;  // 清除异常
    this->api.tick_count = fp_tick_count;          // 计时
    this->api.get_uart_state = fp_get_uart_state;  // 计时

    this->api.set_pwd = fp_set_pwd;            // 设置口令
    this->api.set_pwd_back = fp_set_pwd_back;  // 设置口令消息处理

    this->api.vfy_pwd = fp_vfy_pwd;            // 验证口令
    this->api.vfy_pwd_back = fp_vfy_pwd_back;  // 口令消息处理

    this->api.identify = fp_identify;            // 指纹验证
    this->api.identify_back = fp_identify_back;  // 指纹验证消息处理

    this->api.enroll = fp_enroll;            // 指纹注册
    this->api.enroll_back = fp_enroll_back;  // 指纹注册消息处理

    this->api.read_index = fp_read_index;            // 指纹读索引表
    this->api.read_index_back = fp_read_index_back;  // 指纹读索引表消息处理

    this->api.delete = fp_delete;            // 指纹删除
    this->api.delete_back = fp_delete_back;  // 指纹删除消息处理

    this->api.sleep = fp_sleep;            // 指纹模块休眠
    this->api.sleep_back = fp_sleep_back;  // 指纹模块休眠消息处理

    this->api.cancel = fp_canncel;            // 指纹模块行为取消
    this->api.cancel_back = fp_canncel_back;  // 指纹模块行为取消

    this->api.led_enable = fp_led_enable;            // led使能与失能
    this->api.led_enable_back = fp_led_enable_back;  // led使能与失能

    return (FINGERPRINT*)this;
}

void FINGERPRINT_DTOR(FINGERPRINT* t) {
    /* 尽量不使用，构造一次不要释放，避免多次释放造成内存碎片化 */
    cm_free(t);
}
