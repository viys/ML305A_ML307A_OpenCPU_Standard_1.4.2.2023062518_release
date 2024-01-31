#pragma once
#include "cm_audio_player.h"
#include "cm_audio_recorder.h"
#include "amr_array.h"
#include "transceiver.h"

typedef enum{
    AFTER_RING_CONTINUE = 0x00,
    AFTER_RING_STOP,
    AFTER_RING_OPENSTREAM,
}intercomStatus;

typedef struct{
    int volume; // 喇叭音量
    int gain; // 麦克风增益
    void (*record_cb)(cm_audio_record_event_e event, void *param);
    void (*player_cb)(cm_audio_play_event_e event, void *param);
}IntercomCfg;

typedef struct{
    int (*ring)(void*);
    int (*open_stream)(void*);
    int (*close_stream)(void*);
}INTERCOM_IMPLEMENTS;

typedef struct{
    INTERCOM_IMPLEMENTS interface;
    IntercomCfg cfg;
    transceiverStatus* parentState;
    intercomStatus status;
    int (*audio_sw)(void*, int);
    int (*init)(void*, IntercomCfg);
}INTERCOM;

INTERCOM* INTERCOM_CTOR(TRANSCEIVER* parent);
void INTERCOM_DTOR(INTERCOM);