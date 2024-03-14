#include "app.h"
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
#include "mqtt_client.h"

#define DBG_NAME "app"

BUTTON* Button = NULL;
BATTERY* Battery = NULL;
TRANSCEIVER* Transceiver = NULL;
SOCKETSERVER* SocketClient = NULL;
MQTTCLIENT* MqttClient = NULL;
// INTERCOM 

const char productKey[] = "a16bTikWROS";
const char deviceName[] = "n001";
const char deviceSecret[] = "f3640db1414c5883b7996ebdfb0eb958";
const char pubTopic[] = "/a16bTikWROS/n001/user/cmdSend";
const char subTopic[] = "/a16bTikWROS/n001/user/cmdRcv";
const char mqttHost[] = "iot-06z00b0c4a5stcb.mqtt.iothub.aliyuncs.com";

void dbg_th(void* argument)
{
    


    while(1){
        // mqtt->pub(mqtt, (char*)pubTopic, sizeof(pubTopic));
        osDelayMs(1000);
    }
    
    // int ret = 0;
    // /*串口由于LOG功能,已在cm_opencpu_entry()开头初始化*/
    // int readLen = 0 ;
    // /* 创建信号量 */
	// u0_uart_sem = osSemaphoreNew(1, 0, NULL);
    // DBG_F("hmi_uart_sem ID: %d\r\n", u0_uart_sem);
    
    // // while(1){
    //     // ret = cm_test_ntp();
    // //     DBG_W("ret: %d", ret);
    // //     if(ret != 0) break;
    // //     osDelayMs(400);
    // // }

    // // cm_test_aliyun();
    // // while(cm_test_ntp() != 0){
    // //     osDelayMs(400);
    // // }
    
    // while(1){
    //     #if 0
    //     osSemaphoreAcquire(u0_uart_sem, osWaitForever);
    //     readLen = u0_uart_read(rxBuff);
    //     my_virt_at_test((unsigned char*)rxBuff, strlen(rxBuff));
    //     u0_printf("Send %s (%d)\r\n",rxBuff , readLen);
    //     #else
    //     /* 充电时可以高速检测 */
    //     osDelayMs(1000);
    //     // Battery->interface.update_level(Battery);
    //     // u0_printf("%d\n", Battery->interface.get_level(Battery));
    //     // // u0_printf("%d\n", Battery->interface.update_level(Battery));
    //     #endif
    // }
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


void Transceiver_Thread(void* param)
{
    void* massgeQ = NULL;
    TransceiverCfg cfg =  {
        .imei = "123456789012345",
        .imsi = "12345678901234567890",
        .targt.imei = "2234567890",
        .player_cb = my_player_cb,
        .record_cb = my_record_cb,
        .volume = 100,
        .gain = 0,
    };
    
    Transceiver = TRANSCEIVER_CTOR();
    Transceiver->init(Transceiver, cfg);

    my_audio_io_sw(1);

    TRANSCEIVER_IMPLEMENTS* phone = (TRANSCEIVER_IMPLEMENTS*)Transceiver;

    transceiver_queue = osMessageQueueNew(4, sizeof(void*), NULL);

    Battery = BATTERY_CTOR();
    Battery->init(Battery);
    BATTERY_IMPLEMENTS* batteryCtrl = (BATTERY_IMPLEMENTS*)Battery;    

    phone->online(Transceiver);

    while(1)
    {
        osMessageQueueGet(transceiver_queue, &massgeQ, 0, osWaitForever);
        switch(((uint8_t*)massgeQ)[0])
        {
        case PHONE_PRESS_GREEN:
            DBG_F("press green\r\n");
            phone->press_green(Transceiver);
            break;
        case PHONE_PRESS_RED:
            DBG_F("press red\r\n");
            phone->press_red(Transceiver);
            break;
        case PHONE_INCOMING:
            phone->incoming(Transceiver);
            break;
        case PHONE_SET_TARGET:{
            TargetInfo info;
            memcpy(&info, massgeQ+1, sizeof(TargetInfo));
            phone->set_targt(Transceiver, info);
            break;
        }
        case PHONE_RECORD_SEND:
            
            break;
        case PHONE_RECORD_RECV:

            break;
        default:
            break;
        }
        cm_free(massgeQ);
        screen_display(Transceiver);
    }
    
}

char recv_buf[324+8];
void socket_rev_callback(int sock, cm_asocket_event_e event, void *user_param)
{
    switch (event)
    {
    // responses
    case CM_ASOCKET_EV_CONNECT_OK: {
        /*直接赋值为1清除其他位*/
        // MySocket_CB.status = 1;
        my_network_io_sw(1);
        DBG_I("sock(%d) connect_ok\r\n", sock);
        break;
    }
    case CM_ASOCKET_EV_CONNECT_FAIL: {
        DBG_I("sock(%d) connect_fail\r\n", sock);
        break;
    }

    // indications
    case CM_ASOCKET_EV_RECV_IND: {
        /* 取得获取接收缓存中可读的数据长度 */
        int recv_avail = 0;
        cm_asocket_ioctl(sock, FIONREAD, &recv_avail);

        /* 接收数据 */
        memset(recv_buf, 0, 324+8);
        int ret = cm_asocket_recv(sock, recv_buf, sizeof(recv_buf), 0);
        if (ret > 0){
            // my_socket_recv_handle(recv_buf);
            // DBG_I("sock(%d) recv_ind: recv_avail=%d, recv_len=%d, data=%x %x %x %x %x %x %x %x %x\r\n"
            //         , MyTCP_CB.sock, recv_avail, ret,
            //         recv_buf[0],recv_buf[1],recv_buf[2],recv_buf[3],recv_buf[4],recv_buf[5],recv_buf[6],recv_buf[7],recv_buf[8]);
        }
        else
        {
            DBG_I("sock(%d) recv_ind error(%d)\r\n", sock, errno);

            if (ENOTCONN == errno)
            {
                /* Connection closed */
                /*全部状态清空*/
                // MySocket_CB.status = 0;
                // my_netled_status_sw(0);
                my_network_io_sw(0);
                DBG_I("sock(%d) recv_ind: Connection closed\r\n", MyTCP_CB.sock);
            }
        }
        break;
    }
    case CM_ASOCKET_EV_SEND_IND:
        // DBG_I("sock(%d) send_ind\r\n", MyTCP_CB.sock);
        break;
    case CM_ASOCKET_EV_ACCEPT_IND:
        // DBG_I("sock(%d) accept_ind\r\n", MyTCP_CB.sock);
        break;
    case CM_ASOCKET_EV_ERROR_IND: {
        /* 获取socket错误码 */
        int sock_error = 0;
        socklen_t opt_len = sizeof(sock_error);
        cm_asocket_getsockopt(sock, SOL_SOCKET, SO_ERROR, &sock_error, &opt_len);
        DBG_I("sock(%d) error_ind: sock_error(%d)\r\n", sock, sock_error);
        if (ECONNABORTED == sock_error)
        {
            /* Connection aborted */
            DBG_I("sock(%d) error_ind: Connection aborted\r\n", sock);
        }
        else if (ECONNRESET == sock_error)
        {
            /* Connection reset */
            DBG_I("sock(%d) error_ind: Connection reset\r\n", sock);
        }
        else if (ENOTCONN == sock_error)
        {
            /* Connection closed */
            DBG_I("sock(%d) error_ind: Connection closed\r\n", sock);
        }
        break;
    }
    default:
        break;
    }
}

void Socket_Client_Thread(void* param)
{
    MqttClientInfo mqttInfo = {
        .mqttHost = mqttHost,
        .productKey = productKey,
        .deviceName = deviceName,
        .deviceSecret = deviceSecret,
        .pubTopic = pubTopic,
        .subTopic = subTopic
    };

    MqttClient = MQTTCLIENT_CTOR();
    

    void* mqtt_handle = NULL;
    void* massgeQ = NULL;

    my_network_io_init();

    // while(SocketClient->init(SocketClient, info) != 0){
    //     osDelayMs(1000);
    // }

    socket_send_queue = osMessageQueueNew(4, sizeof(void*), NULL);

    osTimerStart(HeartBeat_Timer, 4000/5);
    my_ntp_get();
    while(ntpTemp == 0)
    {
        osDelayMs(1000);
    }
    // cm_test_aliyun();
    MqttClient->init(MqttClient, mqttInfo);

    osDelayMs(1000);

    MQTTCLIENT_IMPLEMENTS *mqtt = (MQTTCLIENT_IMPLEMENTS*)MqttClient;

    char *pub_payload = "{\"deviceType\":0,\"deviceId\":\"n001\",\"requestId\":\"wechat\",\"return\":{\"door\":\"open\"}}";
    
    mqtt->pub(mqtt, pub_payload, sizeof(pub_payload));
    mqtt->sub(mqtt);
    // my_aliyun_mqtt_connect(mqtt_handle);

    // while(mqttTemp == 0)
    // {
    //     osDelayMs(1000);
    // }

    // my_topic_pub(mqtt_handle);
    
    while(1){
        osMessageQueueGet(socket_send_queue, &massgeQ, 0, osWaitForever);
        // socketCtrl->ssend(SocketClient, massgeQ, 10);
    }
}

void Heart_Beat_Timer(void* param)
{
    Battery->interface.update_level(Battery);
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