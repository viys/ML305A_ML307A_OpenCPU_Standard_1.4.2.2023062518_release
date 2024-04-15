#include "sys.h"
#include "app.h"
#include "cm_pm.h"
#include "cm_mem.h"
#include "drv_uart.h"
#include "drv_virt_at.h"
#include "button.h"
#include "drv_gpio.h"
#include "transceiver.h"
#include "display.h"
#include "socket_client.h"
#include "drv_socket.h"
#include "drv_audio.h"
#include "battery.h"
#include "drv_ntp.h"
#include "drv_mqtt.h"
#include "cJSON.h"
#include "motor.h"

#define DBG_NAME "app"

BUTTON* Button = NULL;
BATTERY* Battery = NULL;

SOCKETSERVER* SocketClient = NULL;

// INTERCOM 

void LOCK_Handle(void* param)
{
    char* massgeQ = NULL;
    LOCK* Lock = LOCK_CTOR();
    Lock->init(Lock);
    LOCK_IMPLEMENTS* lock = (LOCK_IMPLEMENTS*)Lock;

    lock_queue = osMessageQueueNew(4, sizeof(void*), NULL);
    while(1){
        osMessageQueueGet(lock_queue, &massgeQ, 0, osWaitForever);
        switch(massgeQ[0])
        {
        case 0x01:
            lock->open(lock);
            break;
        case 0x02:
            lock->close1(lock);
            break;
        default:
            break;
        }
        cm_free(massgeQ);
    }
}

void FP_RECV_Handle(void* param)
{
    char* massgeQ = NULL;
    char* temp = NULL;
    FingerprintParam fpParam;
    fp->init(fp);
    uart_open(CM_UART_DEV_1, 57600, u1_callback);
    fp_uart_queue = osMessageQueueNew(4, sizeof(void*), NULL);
    while(1){
        osMessageQueueGet(fp_uart_queue, &massgeQ, 0, osWaitForever);
        if(massgeQ[0] == 0xAA){
            fp->touch(fp);
            fp->enable(fp, 1);
        }else{
            fp->data_handle(fp, massgeQ);
        }
        cm_free(massgeQ);
    }
}

void RTC_Count_Task(void* argument)
{
    Battery = BATTERY_CTOR();
    BATTERY_IMPLEMENTS* batteryCtrl = (BATTERY_IMPLEMENTS*)Battery;    
    batteryCtrl->init(Battery);

    while(1){
        for(int i=0; i<4; i++){
            /* 充电时可以高速检测 */
            osDelayMs(200);
            Battery->interface.update_level(Battery);
        }
        osDelayMs(200);
        // DBG_I("Bat %d\r\n", Battery->interface.get_level(Battery));
    }
}

void my_button_callback(void){
    void* msg = cm_malloc(1);
    ((uint8_t*)msg)[0] = 0x01; //对应通道宏定义
    osMessageQueuePut(button_click_queue, &msg, 0, 0);
}

void Button_Click_Thread(void* param)
{
    void* massgeQ = NULL;
    osStatus_t ret = osStatusReserved;
    int checkRet = BUTTON_RELEASED;
    ButtonInfo info = {
        .num = BUTTON_NUM,
        .pin = BUTTON_PIN,
        .mode = CM_GPIO_PULL_UP,
        .direction = CM_GPIO_DIRECTION_INPUT,
        .irq = CM_GPIO_IT_EDGE_RISING,
        .call_back = my_button_callback
    };

    Button = BUTTON_CTOR();
    Button->init(Button, info);
    BUTTON_IMPLEMENTS* buttonCtrl = (BUTTON_IMPLEMENTS*)Button;

    button_click_queue = osMessageQueueNew(4, sizeof(void*), NULL);

    while(1){
        ret = osMessageQueueGet(button_click_queue, &massgeQ, 0, 100);
        switch(ret)
        {
        case osOK:
            buttonCtrl->pressed(Button);
            cm_free(massgeQ);
            break;
        case osError:
            checkRet =  buttonCtrl->check(Button);
            if(checkRet != BUTTON_RELEASED){
                void* msg = cm_malloc(1);
                ((uint8_t*)msg)[0] = checkRet;
                osMessageQueuePut(transceiver_queue, &msg, 0, 0);
            }
            break;
        default:
            break;
        }
    }
}

static void my_record_cb(cm_audio_record_event_e event, void *param)
{
    /* 通知事件为中断触发，不可阻塞，不可做耗时较长的操作，例如不可使用UART打印log */
    static int packLen = 0;
    static char* recorddata = NULL;

    cm_audio_record_data_t *record_data = (cm_audio_record_data_t *)param;

    if (CM_AUDIO_RECORD_EVENT_DATA == event)
    {
        if(packLen == 0){
            recorddata = (char*)cm_malloc(AMR_PACK_LEN);
        }
        
        if(packLen < AMR_PACK_LEN){
            memcpy(&recorddata[packLen], record_data->data, record_data->len);
            packLen += record_data->len;
        }else{
            /*发送录音数据*/
            osMessageQueuePut(transceiver_queue, &recorddata, 0, 0);
            packLen = 0;
        }
    }
}

//播放回调函数
static void my_player_cb(cm_audio_play_event_e event, void *param)
{
    if (event == CM_AUDIO_PLAY_EVENT_FINISHED)                      //判断播放结束
    {   
        /* 通知事件为中断触发，不可阻塞，不可做耗时较长的操作，例如不可使用UART打印log */
    }
}

void MQTT_RECV_Handle(void* param)
{
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

    while(1){
        osMessageQueueGet(mqtt_recv_queue, &massgeQ, 0, osWaitForever);

        cjson_whole = cJSON_Parse(massgeQ);

        cjson_commend = cJSON_GetObjectItem(cjson_whole, "commend");
        if(cjson_commend){
            cjson_item = cJSON_GetObjectItem(cjson_commend, "door");
            if(cjson_item){
                // switch(strcmp(cjson_item->valuestring)){
                // case "open":
                //     my_ringtone_play(MP3_CLOSELOCK_PATH);
                //     break;
                // case "close":
                //     break;
                
                // }
                DBG_W("door: \r\n");
                if(ledState){
                    my_ringtone_play(MP3_CLOSELOCK_PATH);
                    void* msg = cm_malloc(1);
                    ((uint8_t*)msg)[0] = 0x02; //对应通道宏定义
                    osMessageQueuePut(lock_queue, &msg, 0, 0);
                }else{
                    my_ringtone_play(MP3_OPENLOCK_PATH);
                    void* msg = cm_malloc(1);
                    ((uint8_t*)msg)[0] = 0x01; //对应通道宏定义
                    osMessageQueuePut(lock_queue, &msg, 0, 0);
                }
                my_run_io_sw(ledState);
                my_network_io_sw(!ledState);
                ledState = !ledState;
            }
        }

        cjson_request = cJSON_GetObjectItem(cjson_whole, "request");
        if(cjson_request){
            DBG_W("request: %s\r\n", cjson_request->valuestring);
            cJSON* root = cJSON_CreateObject();

	        cJSON_AddStringToObject(root, "deviceId","n001");
	        cJSON_AddStringToObject(root, "requestId","wechat");
	        cJSON_AddBoolToObject(root, "deviceType", 0);

            cJSON* info = cJSON_CreateObject();
	        cJSON_AddStringToObject(info, "imei", Transceiver->cfg.imei);

            cJSON_AddItemToObject(root, "deviceInfo", info);

            void* msg = cm_malloc(sizeof(15));
            msg = (void*)cJSON_PrintUnformatted(root);

            osMessageQueuePut(mqtt_send_queue, &msg, 0, 0);

            cJSON_Delete(root);
        }

        cJSON_Delete(cjson_whole);
        cm_free(massgeQ);
    }
}

void MQTT_Client_Thread(void* param)
{
    void* massgeQ = NULL;
    int temp = 0;

    MqttClientInfo mqttInfo = {
        .mqttHost = "iot-06z00b0c4a5stcb.mqtt.iothub.aliyuncs.com",
        .productKey = "a16bTikWROS",
        .deviceName = "n001",
        .deviceSecret = "f3640db1414c5883b7996ebdfb0eb958",
        .pubTopic = "/a16bTikWROS/n001/user/cmdSend",
        .subTopic = "/a16bTikWROS/n001/user/cmdRcv"
    };

    TransceiverCfg cfg =  {
        .imei = "123456789012345",
        .imsi = "12345678901234567890",
        .targt.imei = "2234567890",
        .player_cb = my_player_cb,
        .record_cb = my_record_cb,
        .volume = 100,
        .gain = 0,
    };

    Transceiver->init(Transceiver, cfg);
    my_audio_io_sw(1);
    phone->online(Transceiver);

    transceiver_queue = osMessageQueueNew(4, sizeof(void*), NULL);
    Battery = BATTERY_CTOR();
    BATTERY_IMPLEMENTS* batteryCtrl = (BATTERY_IMPLEMENTS*)Battery;    
    batteryCtrl->init(Battery);

    my_network_io_init();

    /* 连接mqtt前首先要同步ntp */
    my_ntp_get();
    osDelayMs(1000);
	while(ntpTemp == 0){
        if(temp >= 50){
            cm_pm_reboot();
        }
        ++ temp;
        my_ntp_get();
        osDelayMs(1000);
    }

    mqtt->init(MqttClient, mqttInfo);
    mqtt->sub(mqtt);

    mqtt_send_queue = osMessageQueueNew(4, sizeof(void*), NULL);
    
    while(1){
        osMessageQueueGet(mqtt_send_queue, &massgeQ, 0, osWaitForever);
        mqtt->pub(mqtt, massgeQ, strlen(massgeQ));
        cm_free(massgeQ);
    }
}

void Heart_Beat_Timer(void* param)
{
}

osThreadId_t osThreadCreat(const char * name,osThreadFunc_t func,osPriority_t priority,uint32_t stacksize)
{
	
	osThreadAttr_t thread_cfg =
	{
		/* 进程名 */
		.name = name,
        /* 优先级 */
		.priority = priority,
        /* 栈空间 */
		.stack_size = stacksize,
	};

    /* ML307A模组模组用户线程优先级必须低于或等于osPriorityNormal */
    if(osPriorityNormal > thread_cfg.priority){
        thread_cfg.priority = osPriorityNormal;
    }

	return osThreadNew(func,NULL,(const osThreadAttr_t *)&thread_cfg);

}

osTimerId_t osTimerCreat(const char * name,osTimerFunc_t func,osTimerType_t type)
{
    osTimerAttr_t timer_attr = 
    {
        .name = name,
    };
    return osTimerNew(func,type,NULL,&timer_attr);
}

void osDelayMs(uint32_t ms)
{
    /* ML307A心跳周期为5ms */
    osDelay(ms/5);
}