/**
 * @file mqtt_client.h
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2024-03-01
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#pragma once

typedef struct {
    char* cmd;
    void* msg;
} Pub_Struct;

typedef struct {
    char* mqttHost;
    char* productKey;
    char* deviceName;
    char* deviceSecret;
    char* pubTopic;
    char* subTopic;
} MqttClientInfo;

typedef enum { IDLE = 0x00, BUSY } MqttClientState;

typedef struct {
    void (*init)(void*, MqttClientInfo);
    int (*pub)(void*, char*, int);
    int (*sub)(void*);
} MQTTCLIENT_IMPLEMENTS;

typedef struct {
    MQTTCLIENT_IMPLEMENTS api;
    MqttClientInfo info;
    MqttClientState state;
    void (*rcv_handle)(void*, char*);
    void* handle;
} MQTTCLIENT;

MQTTCLIENT* MQTTCLIENT_CTOR(void);
void MQTTCLIENT_DTOR(MQTTCLIENT* t);
