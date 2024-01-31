#pragma once
#include "cm_audio_player.h"
#include "cm_audio_recorder.h"

#define AMR_PACK_LEN 312

#define PHONE_PRESS_RED     0x01
#define PHONE_PRESS_GREEN   0x02
#define PHONE_INCOMING      0x03
#define PHONE_SET_TARGET    0x04
#define PHONE_RECORD_SEND   0x05
#define PHONE_RECORD_RECV   0x06

typedef enum{
    TRANSMITTER_OFFLINE = 0x00,
    TRANSMITTER_IDLE,
    TRANSMITTER_DIAL,
    TRANSMITTER_TARGETBUSY,
    TRANSCEIVER_BUSY_LINE,
    TRANSCEIVER_INCOMING,
    TRANSCEIVER_HANGUP,
    TRANSCEIVER_TIMEOUT
}transceiverStatus;

typedef struct{
    char imei[15];
}TargetInfo;
 
typedef struct{
    char imei[15];
    char imsi[20];
    TargetInfo targt;
    int volume; // 喇叭音量
    int gain; // 麦克风增益
    void (*record_cb)(cm_audio_record_event_e event, void *param);
    void (*player_cb)(cm_audio_play_event_e event, void *param);
}TransceiverCfg;

typedef struct
{
    int (*incoming)(void*);
    int (*press_green)(void*);
    int (*press_red)(void*);
    transceiverStatus (*get_status)(void*);
    int (*online)(void*);
    int (*set_targt)(void*, TargetInfo);
}TRANSCEIVER_IMPLEMENTS;

typedef struct
{
    TRANSCEIVER_IMPLEMENTS interface;
    transceiverStatus status;
    TransceiverCfg cfg;
    int (*dial)(void*);
    int (*answer)(void*);
    int (*cancel)(void*);
    int (*reject)(void*);
    int (*hangup)(void*);
    int (*ring)(void*, const char*);
    int (*init)(void*, TransceiverCfg);
    int (*open_stream)(void*);
    int (*play_stream)(void*, char* ,int);
    int (*send_stream)(void*, char* ,int);
    int (*close_stream)(void*);
}TRANSCEIVER;

TRANSCEIVER* TRANSCEIVER_CTOR(void);
void TRANSCEIVER_DTOR(TRANSCEIVER* t);

