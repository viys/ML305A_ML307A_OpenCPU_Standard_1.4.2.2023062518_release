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

#define DBG_NAME "app"

BUTTON* Button = NULL;
BATTERY* Battery = NULL;
TRANSCEIVER* Transceiver = NULL;
SOCKETSERVER* SocketClient = NULL;

void dbg_th(void* argument)
{
    /*串口由于LOG功能,已在cm_opencpu_entry()开头初始化*/
    int readLen = 0 ;
    /* 创建信号量 */
	u0_uart_sem = osSemaphoreNew(1, 0, NULL);
    DBG_F("hmi_uart_sem ID: %d\r\n", u0_uart_sem);
    
    while(1){
        #if 0
        osSemaphoreAcquire(u0_uart_sem, osWaitForever);
        readLen = u0_uart_read(rxBuff);
        my_virt_at_test((unsigned char*)rxBuff, strlen(rxBuff));
        u0_printf("Send %s (%d)\r\n",rxBuff , readLen);
        #else
        osDelayMs(1000);
        Battery->interface.update_level(Battery);
        u0_printf("%d\n", Battery->interface.get_level(Battery));
        // u0_printf("%d\n", Battery->interface.update_level(Battery));
        #endif
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
                ((uint8_t*)msg)[0] = checkRet; //对应通道宏定义
                osMessageQueuePut(transceiver_queue, &msg, 0, 0);
            }
            break;
        default:
            break;
        }
    }
}


#define PHONE_PRESS_RED     0x01
#define PHONE_PRESS_GREEN   0x02
#define PHONE_INCOMING      0x03
#define PHONE_SET_TARGET    0x04

void Transceiver_Thread(void* param)
{
    void* massgeQ = NULL;
    TransceiverInfo info =  {
        .imei = "123456789012345",
        .imsi = "12345678901234567890",
        .targt.imei = "2234567890"
    };
    
    Transceiver  = TRANSCEIVER_CTOR();
    Transceiver->init(Transceiver, info);

    TRANSCEIVER_IMPLEMENTS* phone = (TRANSCEIVER_IMPLEMENTS*)Transceiver;

    transceiver_queue = osMessageQueueNew(4, sizeof(void*), NULL);

    Battery = BATTERY_CTOR();
    Battery->init(Battery);
    BATTERY_IMPLEMENTS* batteryCtrl = (BATTERY_IMPLEMENTS*)Battery;    

    my_amr_load_files();
    my_audio_init(100);

    my_audio_io_init();
    my_audio_io_sw(1);

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
        case PHONE_SET_TARGET:{
            TargetInfo info;
            memcpy(&info, massgeQ+1, sizeof(TargetInfo));
            phone->set_targt(Transceiver, info);
            break;
        }
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
        cm_asocket_ioctl(SocketClient->socket, FIONREAD, &recv_avail);

        /* 接收数据 */
        memset(recv_buf, 0, 324+8);
        int ret = cm_asocket_recv(SocketClient->socket, recv_buf, sizeof(recv_buf), 0);
        if (ret > 0){
            // my_socket_recv_handle(recv_buf);
            // DBG_I("sock(%d) recv_ind: recv_avail=%d, recv_len=%d, data=%x %x %x %x %x %x %x %x %x\r\n"
            //         , MyTCP_CB.sock, recv_avail, ret,
            //         recv_buf[0],recv_buf[1],recv_buf[2],recv_buf[3],recv_buf[4],recv_buf[5],recv_buf[6],recv_buf[7],recv_buf[8]);
        }
        else
        {
            DBG_I("sock(%d) recv_ind error(%d)\r\n", SocketClient->socket, errno);

            if (ENOTCONN == errno)
            {
                /* Connection closed */
                /*全部状态清空*/
                // MySocket_CB.status = 0;
                // my_netled_status_sw(0);
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
        cm_asocket_getsockopt(SocketClient->socket, SOL_SOCKET, SO_ERROR, &sock_error, &opt_len);
        DBG_I("sock(%d) error_ind: sock_error(%d)\r\n", SocketClient->socket, sock_error);
        if (ECONNABORTED == sock_error)
        {
            /* Connection aborted */
            DBG_I("sock(%d) error_ind: Connection aborted\r\n", SocketClient->socket);
        }
        else if (ECONNRESET == sock_error)
        {
            /* Connection reset */
            DBG_I("sock(%d) error_ind: Connection reset\r\n", SocketClient->socket);
        }
        else if (ENOTCONN == sock_error)
        {
            /* Connection closed */
            DBG_I("sock(%d) error_ind: Connection closed\r\n", SocketClient->socket);
        }
        break;
    }
    default:
        break;
    }
}

void Socket_Client_Thread(void* param)
{
    void* massgeQ = NULL;
    SocketInfo info = {
        // .ip = "1234567890",
        .addr = "60.205.170.65",
        .port = 8107,
        .callback = socket_rev_callback
    };

    SocketClient = SOCKETSERVER_CTOR();

    my_network_io_init();
    my_network_io_sw(1);

    // while(SocketClient->init(SocketClient, info) != 0){
    //     osDelayMs(1000);
    // }

    SOCKETSERVER_IMPLEMENTS* socketCtrl = (SOCKETSERVER_IMPLEMENTS*)SocketClient;

    socket_send_queue = osMessageQueueNew(4, sizeof(void*), NULL);

    osTimerStart(HeartBeat_Timer, 4000/5);

    while(1){
        osMessageQueueGet(socket_send_queue, &massgeQ, 0, osWaitForever);
        socketCtrl->ssend(SocketClient, massgeQ, 10);
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