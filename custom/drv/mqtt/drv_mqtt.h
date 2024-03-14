#pragma once

#include <stdint.h>
#include <stddef.h>

void cm_test_aliyun(void);

/**
 * @brief 连接阿里云MQTT服务器函数
 * 
 * @note 在连接前系统需要成功获取网络时间，否则将链接失败
 */
void* my_aliyun_mqtt_connect(char* host, char* productKey, char* deviceName, char* deviceSecret);
int my_topic_pub(void* handle);
void* my_mqtt_init(void);
int my_mqtt_topic_pub(void* handle, char* pubTopic, char* payload, int len);
int my_mqtt_topic_sub(void* handle, char* subTopic);

extern int mqttTemp;
