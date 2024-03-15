#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "aiot_state_api.h"
#include "aiot_sysdep_api.h"
#include "aiot_mqtt_api.h"
#include "cm_os.h"
#include "cm_sys.h"
#include "drv_uart.h"
#include "drv_mqtt.h"
#include "cm_mem.h"

#define DBG_NAME "mqtt"

/* 位于portfiles/aiot_port文件夹下的系统适配函数集合 */
extern aiot_sysdep_portfile_t g_aiot_sysdep_portfile;

/* 位于external/ali_ca_cert.c中的服务器证书 */
extern const char *ali_ca_cert;

static osThreadId_t g_mqtt_process_thread = NULL;
static osThreadId_t g_mqtt_recv_thread = NULL;
static uint8_t g_mqtt_process_thread_running = 0;
static uint8_t g_mqtt_recv_thread_running = 0;

/* TODO: 如果要关闭日志, 就把这个函数实现为空, 如果要减少日志, 可根据code选择不打印
 *
 * 例如: [1577589489.033][LK-0317] {YourDeviceName}&{YourProductKey}
 *
 * 上面这条日志的code就是0317(十六进制), code值的定义见core/aiot_state_api.h
 *
 */

/* 日志回调函数, SDK的日志会从这里输出 */
static int32_t __demo_state_logcb(int32_t code, char *message)
{
    DBG_I("%s", message);
    return 0;
}

int mqttTemp = 0;

/* MQTT事件回调函数, 当网络连接/重连/断开时被触发, 事件定义见core/aiot_mqtt_api.h */
static void __demo_mqtt_event_handler(void *handle, const aiot_mqtt_event_t *event, void *userdata)
{
    switch (event->type) {
        /* SDK因为用户调用了aiot_mqtt_connect()接口, 与mqtt服务器建立连接已成功 */
        case AIOT_MQTTEVT_CONNECT: {
            DBG_I("AIOT_MQTTEVT_CONNECT\r\n");
            /* TODO: 处理SDK建连成功, 不可以在这里调用耗时较长的阻塞函数 */
            mqttTemp = 1;
        }
        break;

        /* SDK因为网络状况被动断连后, 自动发起重连已成功 */
        case AIOT_MQTTEVT_RECONNECT: {
            DBG_I("AIOT_MQTTEVT_RECONNECT\r\n");
            /* TODO: 处理SDK重连成功, 不可以在这里调用耗时较长的阻塞函数 */
        }
        break;

        /* SDK因为网络的状况而被动断开了连接, network是底层读写失败, heartbeat是没有按预期得到服务端心跳应答 */
        case AIOT_MQTTEVT_DISCONNECT: {
            char *cause = (event->data.disconnect == AIOT_MQTTDISCONNEVT_NETWORK_DISCONNECT) ? ("network disconnect") :
                          ("heartbeat disconnect");
            DBG_I("AIOT_MQTTEVT_DISCONNECT: %s\r\n", cause);
            /* TODO: 处理SDK被动断连, 不可以在这里调用耗时较长的阻塞函数 */
        }
        break;

        default: {

        }
    }
}
extern osMessageQueueId_t mqtt_recv_queue;
/* MQTT默认消息处理回调, 当SDK从服务器收到MQTT消息时, 且无对应用户回调处理时被调用 */
static void __demo_mqtt_default_recv_handler(void *handle, const aiot_mqtt_recv_t *packet, void *userdata)
{
    switch (packet->type) {
        case AIOT_MQTTRECV_HEARTBEAT_RESPONSE: {
            DBG_I("heartbeat response\r\n");
            /* TODO: 处理服务器对心跳的回应, 一般不处理 */
        }
        break;

        case AIOT_MQTTRECV_SUB_ACK: {
            DBG_I("suback, res: -0x%04X, packet id: %d, max qos: %d\r\n",
                   -packet->data.sub_ack.res, packet->data.sub_ack.packet_id, packet->data.sub_ack.max_qos);
            /* TODO: 处理服务器对订阅请求的回应, 一般不处理 */
        }
        break;

        case AIOT_MQTTRECV_PUB: {
            DBG_I("pub, qos: %d, topic: %.*s\r\n", packet->data.pub.qos, packet->data.pub.topic_len, packet->data.pub.topic);
            DBG_I("pub, payload: %.*s\r\n", packet->data.pub.payload_len, packet->data.pub.payload);
            
            void* msg = cm_malloc(packet->data.pub.payload_len);
            memcpy(msg, packet->data.pub.payload, packet->data.pub.payload_len);
            osMessageQueuePut(mqtt_recv_queue, &msg, 0, 0);
            /* TODO: 处理服务器下发的业务报文, 此处如果收到rrpc的报文, 那么应答时使用相同的topic即可 */


        }
        break;

        case AIOT_MQTTRECV_PUB_ACK: {
            DBG_I("puback, packet id: %d\r\n", packet->data.pub_ack.packet_id);
            /* TODO: 处理服务器对QoS1上报消息的回应, 一般不处理 */
        }
        break;

        default: {

        }
    }
}

/* 执行aiot_mqtt_process的线程, 包含心跳发送和QoS1消息重发 */
static void *__demo_mqtt_process_thread(void *args)
{
    int32_t res = STATE_SUCCESS;

    while (g_mqtt_process_thread_running) {
        res = aiot_mqtt_process(args);
        if (res == STATE_USER_INPUT_EXEC_DISABLED) {
            break;
        }
        osDelay(1);
    }
    return NULL;
}

/* 执行aiot_mqtt_recv的线程, 包含网络自动重连和从服务器收取MQTT消息 */
static void *__demo_mqtt_recv_thread(void *args)
{
    int32_t res = STATE_SUCCESS;

    while (g_mqtt_recv_thread_running) {
        res = aiot_mqtt_recv(args);
        if (res < STATE_SUCCESS) {
            if (res == STATE_USER_INPUT_EXEC_DISABLED) {
                break;
            }
            osDelay(1);
        }
    }
    return NULL;
}

void* my_mqtt_init(void)
{
    void* mqttHandle;
    /* 创建1个MQTT客户端实例并内部初始化默认参数 */
    mqttHandle = aiot_mqtt_init();
    if (mqttHandle == NULL) {
        DBG_I("aiot_mqtt_init failed\r\n");
        return NULL;
    }
    return mqttHandle;
}

void* my_aliyun_mqtt_connect(char* host, char* productKey, char* deviceName, char* deviceSecret)
{
    void* handle = NULL;
    int32_t     res = STATE_SUCCESS;
    uint16_t    port = 443;      /* 无论设备是否使用TLS连接阿里云平台, 目的端口都是443 */
    aiot_sysdep_network_cred_t cred; /* 安全凭据结构体, 如果要用TLS, 这个结构体中配置CA证书等参数 */

    /* 配置SDK的底层依赖 */
    aiot_sysdep_set_portfile(&g_aiot_sysdep_portfile);
    /* 配置SDK的日志输出 */
    aiot_state_set_logcb(__demo_state_logcb);

    /* 创建SDK的安全凭据, 用于建立TLS连接 */
    memset(&cred, 0, sizeof(aiot_sysdep_network_cred_t));
    cred.option = AIOT_SYSDEP_NETWORK_CRED_SVRCERT_CA;  /* 使用RSA证书校验MQTT服务端 */
    cred.max_tls_fragment = 16384; /* 最大的分片长度为16K, 其它可选值还有4K, 2K, 1K, 0.5K */
    cred.sni_enabled = 1;                               /* TLS建连时, 支持Server Name Indicator */
    cred.x509_server_cert = ali_ca_cert;                 /* 用来验证MQTT服务端的RSA根证书 */
    cred.x509_server_cert_len = strlen(ali_ca_cert);     /* 用来验证MQTT服务端的RSA根证书长度 */

    /* 创建1个MQTT客户端实例并内部初始化默认参数 */
    handle = aiot_mqtt_init();
    if (handle == NULL) {
        DBG_I("aiot_mqtt_init failed\r\n");
        return NULL;
    }

    /* 配置MQTT服务器地址 */
    aiot_mqtt_setopt(handle, AIOT_MQTTOPT_HOST, (void *)host);
    /* 配置MQTT服务器端口 */
    aiot_mqtt_setopt(handle, AIOT_MQTTOPT_PORT, (void *)&port);
    /* 配置设备productKey */
    aiot_mqtt_setopt(handle, AIOT_MQTTOPT_PRODUCT_KEY, (void *)productKey);
    /* 配置设备deviceName */
    aiot_mqtt_setopt(handle, AIOT_MQTTOPT_DEVICE_NAME, (void *)deviceName);
    /* 配置设备deviceSecret */
    aiot_mqtt_setopt(handle, AIOT_MQTTOPT_DEVICE_SECRET, (void *)deviceSecret);
    /* 配置网络连接的安全凭据, 上面已经创建好了 */
    aiot_mqtt_setopt(handle, AIOT_MQTTOPT_NETWORK_CRED, (void *)&cred);
    /* 配置MQTT默认消息接收回调函数 */
    aiot_mqtt_setopt(handle, AIOT_MQTTOPT_RECV_HANDLER, (void *)__demo_mqtt_default_recv_handler);
    /* 配置MQTT事件回调函数 */
    aiot_mqtt_setopt(handle, AIOT_MQTTOPT_EVENT_HANDLER, (void *)__demo_mqtt_event_handler);

    /* 与服务器建立MQTT连接 */
    res = aiot_mqtt_connect(handle);
    if (res < STATE_SUCCESS) {
        /* 尝试建立连接失败, 销毁MQTT实例, 回收资源 */
        aiot_mqtt_deinit(&handle);
        DBG_I("aiot_mqtt_connect failed: -0x%04X\r\n", -res);
        DBG_I("please check variables like mqtt_host, produt_key, device_name, device_secret in demo\r\n");
        return NULL;
    }
    
    /* 创建一个单独的线程, 专用于执行aiot_mqtt_process, 它会自动发送心跳保活, 以及重发QoS1的未应答报文 */
    g_mqtt_process_thread_running = 1;

    osThreadAttr_t mqtt_process_attr = {0};
    
    mqtt_process_attr.name  = "mqtt_process_task";
    mqtt_process_attr.stack_size = 1024 * 4;
    mqtt_process_attr.priority = osPriorityNormal;
    g_mqtt_process_thread = osThreadNew((osThreadFunc_t)__demo_mqtt_process_thread,handle,&mqtt_process_attr);
	
    if (g_mqtt_process_thread == NULL) {
        DBG_I("pthread_create __demo_mqtt_process_thread failed: %d\r\n", res);
        return NULL;
    }

    /* 创建一个单独的线程用于执行aiot_mqtt_recv, 它会循环收取服务器下发的MQTT消息, 并在断线时自动重连 */
    g_mqtt_recv_thread_running = 1;

    osThreadAttr_t mqtt_recv_attr = {0};
    
    mqtt_recv_attr.name  = "mqtt_recv_task";
    mqtt_recv_attr.stack_size = 1024 * 4;
    mqtt_recv_attr.priority = osPriorityNormal;
    g_mqtt_recv_thread = osThreadNew((osThreadFunc_t)__demo_mqtt_recv_thread,handle,&mqtt_recv_attr);

    if (g_mqtt_recv_thread == NULL) {
        DBG_I("pthread_create __demo_mqtt_recv_thread failed: %d\r\n", res);
        return NULL;
    }

    return handle;
}

int my_mqtt_topic_pub(void* handle, char* pubTopic, char* payload, int len)
{
    int ret = 0;

    ret = aiot_mqtt_pub(handle, pubTopic, (uint8_t *)payload, len, 1);
    if (ret < 0) {
        DBG_I("aiot_mqtt_pub failed, res: -0x%04X\r\n", -ret);
    }

    return ret;
}

int my_mqtt_topic_sub(void* handle, char* subTopic)
{
    int ret =0;

    ret = aiot_mqtt_sub(handle, subTopic, NULL, 1, NULL);
    if (ret < 0) {
        DBG_I("aiot_mqtt_sub failed, res: -0x%04X\r\n", -ret);
        return ret;
    }

    return ret;
}
