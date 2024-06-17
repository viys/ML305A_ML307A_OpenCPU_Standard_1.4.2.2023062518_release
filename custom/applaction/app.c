#include "app.h"
#include "battery.h"
#include "button.h"
#include "cJSON.h"
#include "cm_mem.h"
#include "cm_pm.h"
#include "display.h"
#include "drv_audio.h"
#include "drv_gpio.h"
#include "drv_mqtt.h"
#include "drv_ntp.h"
#include "drv_socket.h"
#include "drv_uart.h"
#include "drv_virt_at.h"
#include "motor.h"
#include "socket_client.h"
#include "sys.h"
#include "transceiver.h"

#define DBG_NAME "app"

BUTTON* Button = NULL;

SOCKETSERVER* SocketClient = NULL;

void LOCK_Handle(void* param) {
    int lockRet = 0;
    char* massgeQ = NULL;
    LOCK* Lock = LOCK_CTOR();
    lock = (LOCK_IMPLEMENTS*)Lock;
    Lock->init(Lock);

    lock_queue = osMessageQueueNew(4, sizeof(void*), NULL);
    while (1) {
        osMessageQueueGet(lock_queue, &massgeQ, 0, osWaitForever);
        switch (massgeQ[0]) {
            case 0x01:
                lockRet = lock->open(lock);
                if (lockRet == 0) {
                    DBG_I("Open successed\r\n");
                    my_ringtone_play(MP3_OPENLOCK_PATH);
                } else {
                    DBG_I("Open failed\r\n");
                    my_ringtone_play(MP3_OPEN_FAILED_PATH);
                }


                {
                    Lock_Msg lockMsg = lock->get_load(lock);

                    cJSON* root = cJSON_CreateObject();

                    cJSON_AddStringToObject(root, "deviceId", "n001");
                    cJSON_AddStringToObject(root, "requestId", "wechat");
                    cJSON_AddBoolToObject(root, "deviceType", 0);

                    cJSON* info = cJSON_CreateObject();
                    cJSON_AddStringToObject(info, "lockOpt", "open");
                    cJSON_AddStringToObject(info, "lockMode", lockMsg.mode);
                    cJSON_AddStringToObject(info, "lockParam", lockMsg.param);

                    cJSON_AddItemToObject(root, "deviceInfo", info);

                    void* msg = cm_malloc(sizeof(64));
                    msg = (void*)cJSON_PrintUnformatted(root);

                    osMessageQueuePut(mqtt_send_queue, &msg, 0, 0);

                    cJSON_Delete(root);
                }
                break;
            case 0x02:
                lockRet = lock->close1(lock);
                if (lockRet == 0) {
                    DBG_I("Close successed\r\n");
                    my_ringtone_play(MP3_CLOSELOCK_PATH);
                } else {
                    DBG_I("Close failed\r\n");
                    my_ringtone_play(MP3_CLOSE_FAILED_PATH);
                }
                {
                    Lock_Msg lockMsg = lock->get_load(lock);
                    cJSON* root = cJSON_CreateObject();

                    cJSON_AddStringToObject(root, "deviceId", "n001");
                    cJSON_AddStringToObject(root, "requestId", "wechat");
                    cJSON_AddBoolToObject(root, "deviceType", 0);

                    cJSON* info = cJSON_CreateObject();
                    cJSON_AddStringToObject(info, "lockOpt", "close");
                    cJSON_AddStringToObject(info, "lockMode", lockMsg.mode);
                    cJSON_AddStringToObject(info, "lockParam", lockMsg.param);

                    cJSON_AddItemToObject(root, "deviceInfo", info);

                    void* msg = cm_malloc(sizeof(64));
                    msg = (void*)cJSON_PrintUnformatted(root);

                    osMessageQueuePut(mqtt_send_queue, &msg, 0, 0);

                    cJSON_Delete(root);
                }
                break;
            default:
                break;
        }
        cm_free(massgeQ);
    }
}

void FP_RECV_Handle(void* param) {
    char* massgeQ = NULL;
    char* temp = NULL;
    FingerprintParam fpParam;
    fp->init(fp);
    uart_open(CM_UART_DEV_1, 57600, u1_callback);
    fp_uart_queue = osMessageQueueNew(4, sizeof(void*), NULL);
    while (1) {
        osMessageQueueGet(fp_uart_queue, &massgeQ, 0, osWaitForever);
        if (massgeQ[0] == 0xAA && fp->get_status(fp) == idle) {
            fp->touch(fp);
            fp->enable(fp, 1);
        } else {
            fp->data_handle(fp, massgeQ);
        }
        cm_free(massgeQ);
    }
}

void RTC_Count_Task(void* argument) {
    int rtcCount = 0;
    batteryCtrl->init(Battery);
    int lockTime = 0;
    while (1) {
        ++rtcCount;
//        for (int i = 0; i < 4; i++) {
//            /* 充电时可以高速检测 */
//            osDelayMs(200);
//
//        }
        osDelayMs(1000);

        fp->tick_count(fp);
        batteryCtrl->update_level(batteryCtrl);
        DBG_I("Bat %d %d\r\n", batteryCtrl->get_level(batteryCtrl), batteryCtrl->get_mode(batteryCtrl));
//        DBG_I("%d %d %d [%d]\r\n", my_io_get(HALL1_GPIO_NUM), my_io_get(HALL2_GPIO_NUM), my_io_get(HALL3_GPIO_NUM), lock->get_state(lock));
        if (lock->get_state(lock) == LOCK_STATE_OPEN && lockTime <= 10) {
//            DBG_I("%d %d %d %d\r\n", my_io_get(HALL1_GPIO_NUM), my_io_get(HALL2_GPIO_NUM), my_io_get(HALL3_GPIO_NUM), lock->get_state(lock));
            ++ lockTime;
        } else if (lock->get_state(lock) == LOCK_STATE_CLOSE) {
            lockTime = 0;
        } else if(lockTime > 10) {
            DBG_I("Time out\r\n");
            if (!my_io_get(HALL3_GPIO_NUM)) {
                lockTime = 0;
//                my_ringtone_play(MP3_CLOSELOCK_PATH);
                lock->load(lock, NULL, NULL, NULL);
                lock->load(lock, NULL, "auto", NULL);
                DBG_I("Atuo close door\r\n");
//                lock->close1(lock);
                void* msg = cm_malloc(1);
                ((uint8_t*)msg)[0] = 0x02;  // 对应通道宏定义
                osMessageQueuePut(lock_queue, &msg, 0, 0);
            }
        }
        //        if (lock->get_state(lock) == LOCK_STATE_OPEN && my_io_get(HALL3_GPIO_NUM)) {
//
//        }
    }
}

void my_button_callback(void) {
    void* msg = cm_malloc(1);
    ((uint8_t*)msg)[0] = 0x01;  // 对应通道宏定义
    osMessageQueuePut(button_click_queue, &msg, 0, 0);
}

void Button_Click_Thread(void* param) {
    bool bellButtonState = 0;
    void* massgeQ = NULL;
    osStatus_t ret = osStatusReserved;
    my_bell_gpio_init();
//    int checkRet = BUTTON_RELEASED;
//    ButtonInfo info = {.num = BELL_GPIO_NUM,
//                       .pin = BELL_GPIO_PIN,
//                       .mode = CM_GPIO_PULL_UP,
//                       .direction = CM_GPIO_DIRECTION_INPUT,
//                       .irq = CM_GPIO_IT_EDGE_RISING,
//                       .call_back = my_button_callback};
//
//    Button = BUTTON_CTOR();
//    Button->init(Button, info);
//    BUTTON_IMPLEMENTS* buttonCtrl = (BUTTON_IMPLEMENTS*)Button;
//
    button_click_queue = osMessageQueueNew(4, sizeof(void*), NULL);

    while (1) {
        ret = osMessageQueueGet(button_click_queue, &massgeQ, 0, 100);

        switch (ret) {
            case osOK:
//                buttonCtrl->pressed(Button);
//                DBG_I("Start BELL BUTTON\r\n");
                bellButtonState = 1;
                cm_free(massgeQ);
                break;
            case osError:
                if (bellButtonState && my_bell_level_get() == CM_GPIO_LEVEL_LOW) {
                    bellButtonState = 0;
//                    DBG_E("End BELL BUTTON\r\n");
                    my_ringtone_play(MP3_CALLSTART_PATH);
                        cJSON* root = cJSON_CreateObject();

                        cJSON_AddStringToObject(root, "deviceId", "n001");
                        cJSON_AddStringToObject(root, "requestId", "wechat");
                        cJSON_AddBoolToObject(root, "deviceType", 0);
                        cJSON_AddStringToObject(root, "doorBell", "on");

                        void* msg = cm_malloc(sizeof(64));
                        msg = (void*)cJSON_PrintUnformatted(root);

                        osMessageQueuePut(mqtt_send_queue, &msg, 0, 0);

                        cJSON_Delete(root);
                } else if(bellButtonState){
                    bellButtonState = 0;
                }
//                checkRet = buttonCtrl->check(Button);
//                if (checkRet != BUTTON_RELEASED) {
//                    DBG_W("Button %d\r\n", checkRet);
//                }
                break;
            default:
                break;
        }
    }
}

static void my_record_cb(cm_audio_record_event_e event, void* param) {
    /* 通知事件为中断触发，不可阻塞，不可做耗时较长的操作，例如不可使用UART打印log
     */
    static int packLen = 0;
    static char* recorddata = NULL;

    cm_audio_record_data_t* record_data = (cm_audio_record_data_t*)param;

    if (CM_AUDIO_RECORD_EVENT_DATA == event) {
        if (packLen == 0) {
            recorddata = (char*)cm_malloc(AMR_PACK_LEN);
        }

        if (packLen < AMR_PACK_LEN) {
            memcpy(&recorddata[packLen], record_data->data, record_data->len);
            packLen += record_data->len;
        } else {
            /*发送录音数据*/
            osMessageQueuePut(transceiver_queue, &recorddata, 0, 0);
            packLen = 0;
        }
    }
}

// 播放回调函数
static void my_player_cb(cm_audio_play_event_e event, void* param) {
    if (event == CM_AUDIO_PLAY_EVENT_FINISHED)  // 判断播放结束
    {
        /* 通知事件为中断触发，不可阻塞，不可做耗时较长的操作，例如不可使用UART打印log
         */
    }
}

void MQTT_RECV_Handle(void* param) {
    void* massgeQ = NULL;
    cJSON* cjson_whole = NULL;
    cJSON* cjson_deviceId = NULL;
    cJSON* cjson_requestId = NULL;
    cJSON* cjson_request = NULL;
    cJSON* cjson_commend = NULL;
    cJSON* cjson_item = NULL;
    bool ledState = 0;

    mqtt_recv_queue = osMessageQueueNew(4, sizeof(void*), NULL);

    bdr_motor_init();

    while (1) {
        osMessageQueueGet(mqtt_recv_queue, &massgeQ, 0, osWaitForever);

        cjson_whole = cJSON_Parse(massgeQ);

        cjson_commend = cJSON_GetObjectItem(cjson_whole, "commend");
        if (cjson_commend) {
            cjson_item = cJSON_GetObjectItem(cjson_commend, "door");
            if (cjson_item) {
                if (strcmp(cjson_item->valuestring, "open") == 0) {
                    DBG_F("Open door\r\n");
//                    my_ringtone_play(MP3_OPENLOCK_PATH);
                    lock->load(lock, NULL, NULL, NULL);
                    lock->load(lock, NULL, "weixin", NULL);
                    void* msg = cm_malloc(1);
                    ((uint8_t*)msg)[0] = 0x01;  // 对应通道宏定义
                    osMessageQueuePut(lock_queue, &msg, 0, 0);
                    fp->clear_abnormal(fp);
                } else if (strcmp(cjson_item->valuestring, "close") == 0) {
                    DBG_F("Close door\r\n");
//                    my_ringtone_play(MP3_CLOSELOCK_PATH);
                    lock->load(lock, NULL, NULL, NULL);
                    lock->load(lock, NULL, "weixin", NULL);
                    void* msg = cm_malloc(1);
                    ((uint8_t*)msg)[0] = 0x02;  // 对应通道宏定义
                    osMessageQueuePut(lock_queue, &msg, 0, 0);
                }
            }
            cjson_item = cJSON_GetObjectItem(cjson_commend, "fp");
            if (cjson_item) {
                if (strcmp(cjson_item->valuestring, "enroll") == 0) {
                    cjson_item = cJSON_GetObjectItem(cjson_commend, "id");
                    DBG_F("Enroll %d\r\n", cjson_item->valueint);
                    Fingerprint->status = enroll;
                    fp->special_opt(fp, "load", cjson_item->valueint);
                    fp->enable(fp, 1);
                } else if (strcmp(cjson_item->valuestring, "delete") == 0) {
                    cjson_item = cJSON_GetObjectItem(cjson_commend, "id");
                    Fingerprint->status = deletChar;
                    fp->special_opt(fp, "load", cjson_item->valueint);
                    DBG_F("Delete %d\r\n", cjson_item->valueint);
                    fp->enable(fp, 1);
                } else {
                }
            }
        }

        cjson_request = cJSON_GetObjectItem(cjson_whole, "request");
        if (cjson_request) {
            DBG_W("request: %s\r\n", cjson_request->valuestring);

            if (strcmp(cjson_request->valuestring, "info") == 0) {
                cJSON* root = cJSON_CreateObject();

                cJSON_AddStringToObject(root, "deviceId", "n001");
                cJSON_AddStringToObject(root, "requestId", "wechat");
                cJSON_AddBoolToObject(root, "deviceType", 0);

                cJSON* info = cJSON_CreateObject();
                cJSON_AddStringToObject(info, "imei", Transceiver->cfg.imei);
                cJSON_AddNumberToObject(info, "level", batteryCtrl->get_level(batteryCtrl));
                cJSON_AddNumberToObject(info, "batMode", batteryCtrl->get_mode(batteryCtrl));
                cJSON_AddNumberToObject(info, "lockState", lock->get_state(lock));

                cJSON_AddItemToObject(root, "deviceInfo", info);

                void* msg = cm_malloc(sizeof(64));
                msg = (void*)cJSON_PrintUnformatted(root);

                osMessageQueuePut(mqtt_send_queue, &msg, 0, 0);

                cJSON_Delete(root);
            }
        }

        cJSON_Delete(cjson_whole);
        cm_free(massgeQ);
    }
}

void MQTT_Client_Thread(void* param) {
    void* massgeQ = NULL;
    int temp = 0;

    MqttClientInfo mqttInfo = {
        .mqttHost = "iot-06z00b0c4a5stcb.mqtt.iothub.aliyuncs.com",
        .productKey = "a16bTikWROS",
        .deviceName = "n001",
        .deviceSecret = "f3640db1414c5883b7996ebdfb0eb958",
        .pubTopic = "/a16bTikWROS/n001/user/cmdSend",
        .subTopic = "/a16bTikWROS/n001/user/cmdRcv"};

    TransceiverCfg cfg = {
        .imei = "123456789012345",
        .imsi = "12345678901234567890",
        .targt.imei = "2234567890",
        .player_cb = my_player_cb,
        .record_cb = my_record_cb,
        .volume =100,
        .gain = 0,
    };

    Transceiver = TRANSCEIVER_CTOR();
    Transceiver->init(Transceiver, cfg);
    phone = (TRANSCEIVER_IMPLEMENTS*)Transceiver;
    my_audio_io_sw(1);
    phone->online(Transceiver);
    /* 设置指纹密码 */
    //    strncpy(Fingerprint->info.pwd, Transceiver->cfg.imei, 4);

    transceiver_queue = osMessageQueueNew(4, sizeof(void*), NULL);

    my_network_io_init();

    /* 连接mqtt前首先要同步ntp */
    my_ntp_get();
    osDelayMs(1000);
    while (ntpTemp == 0) {
        if (temp >= 50) {
            cm_pm_reboot();
        }
        ++temp;
        my_ntp_get();
        osDelayMs(1000);
    }

    mqtt->init(MqttClient, mqttInfo);
    mqtt->sub(mqtt);

    mqtt_send_queue = osMessageQueueNew(4, sizeof(void*), NULL);

    while (1) {
        osMessageQueueGet(mqtt_send_queue, &massgeQ, 0, osWaitForever);
        DBG_F("MQTT send %s\r\n", massgeQ);
        mqtt->pub(mqtt, massgeQ, strlen(massgeQ));
        cm_free(massgeQ);
    }
}

void Heart_Beat_Timer(void* param) {}

osThreadId_t osThreadCreat(const char* name, osThreadFunc_t func,
                           osPriority_t priority, uint32_t stacksize) {

    osThreadAttr_t thread_cfg = {
        /* 进程名 */
        .name = name,
        /* 优先级 */
        .priority = priority,
        /* 栈空间 */
        .stack_size = stacksize,
    };

    /* ML307A模组模组用户线程优先级必须低于或等于osPriorityNormal */
    if (osPriorityNormal > thread_cfg.priority) {
        thread_cfg.priority = osPriorityNormal;
    }

    return osThreadNew(func, NULL, (const osThreadAttr_t*)&thread_cfg);
}

osTimerId_t osTimerCreat(const char* name, osTimerFunc_t func,
                         osTimerType_t type) {
    osTimerAttr_t timer_attr = {
        .name = name,
    };
    return osTimerNew(func, type, NULL, &timer_attr);
}

void osDelayMs(uint32_t ms) {
    /* ML307A心跳周期为5ms */
    osDelay(ms / 5);
}