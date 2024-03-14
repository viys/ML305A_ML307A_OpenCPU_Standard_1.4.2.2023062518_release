/**
 * @file mqtt_client.c
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2024-03-01
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#include "drv_ntp.h"
#include "drv_mqtt.h"
#include "drv_uart.h"
#include "mqtt_client.h"
#include "stdlib.h"
#include "string.h"
#include "aiot_state_api.h"
#include "aiot_sysdep_api.h"
#include "aiot_mqtt_api.h"

#define DBG_NAME "MQTT"

static void mqtt_init(void* t, MqttClientInfo info)
{
    MQTTCLIENT* this = (MQTTCLIENT*)t;

    memcpy(&this->info, &info, sizeof(MqttClientInfo));

    if(this->info.deviceName == NULL){
        DBG_E("DeviceName is NULL");
        return;
    }

    this->state = IDLE;

    this->handle =  my_aliyun_mqtt_connect(this->info.mqttHost, \
                                           this->info.productKey, \
                                           this->info.deviceName, \
                                           this->info.deviceSecret);

    DBG_F("Mqtt init success\r\n");
}

static int mqtt_pub(void* t, char* payload, int len)
{
    int ret = 0;
    MQTTCLIENT* this = (MQTTCLIENT*)t;

    ret = my_mqtt_topic_pub(this->handle, this->info.pubTopic, (uint8_t *)payload, len);

    if(ret >= STATE_SUCCESS){
        DBG_I("Mqtt pub: %*s\r\n", len, payload);
    }else{
        DBG_E("Mqtt pub err");
    }
    
    return ret;
}

static int mqtt_sub(void* t)
{
    int ret = 0;
    MQTTCLIENT* this = (MQTTCLIENT*)t;

    // DBG_E("this->info.subTopic: %s", this->info.subTopic)
    // ret = my_mqtt_topic_sub(this->handle, this->info.subTopic);
    char *sub_topic = "/a16bTikWROS/n001/user/cmdRcv";

    ret = aiot_mqtt_sub(this->handle, sub_topic, NULL, 1, NULL);
    if (ret < 0) {
        DBG_I("aiot_mqtt_sub failed, res: -0x%04X\r\n", -ret);
        return ret;
    }

    // if(ret >= STATE_SUCCESS){
    //     DBG_I("Mqtt sub success\r\n");
    // }else{
    //     DBG_E("Mqtt sub err");
    // }

    return ret;
}

MqttClientState mqtt_get_state(void* t)
{
    // MQTTCLIENT;
}


MQTTCLIENT* MQTTCLIENT_CTOR(void)
{
    MQTTCLIENT *this = (MQTTCLIENT*)malloc(sizeof(MQTTCLIENT));

    /* 接口对接 */
    this->init = mqtt_init;

    this->api.pub = mqtt_pub;
    this->api.sub = mqtt_sub;

    return this;
}




