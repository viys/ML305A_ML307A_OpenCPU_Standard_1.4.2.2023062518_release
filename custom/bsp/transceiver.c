#include "transceiver.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "drv_audio.h"
#include "drv_uart.h"
#include "cm_sim.h"
#include "drv_gpio.h"

#define DBG_NAME "phone"

int transceiver_init(void* t ,TransceiverCfg device)
{
    int ret = 0;
    TRANSCEIVER* this = (TRANSCEIVER*)t;
    memcpy(&this->cfg, &device, sizeof(TransceiverCfg));

    this->status = TRANSMITTER_OFFLINE;

    my_amr_load_files();

    ret += my_audio_init(this->cfg.volume, this->cfg.gain);

    ret += my_audio_io_init();
    
    /*获取IMEI*/
    while ((ret = cm_sys_get_imei(this->cfg.imei))!=0){
        DBG_W("Get device IMEI failed: %d\r\n",ret);
        osDelayMs(10);
    }
    DBG_I("Device\tIMEI: %s\r\n", this->cfg.imei);

    /*获取SIM卡IMSI*/
    while ((ret = cm_sim_get_imsi(this->cfg.imsi))!=0){
        DBG_W("Get SIM IMSI failed: %d\r\n",ret);
        osDelayMs(2000);
    }
    DBG_I("SIM\tIMSI: %s\r\n",this->cfg.imsi);

    DBG_F("Transceiver init successed\r\n");
    
    return ret;
}

int transceiver_dial(void* t)
{
    int ret = 0;
    TRANSCEIVER* this = (TRANSCEIVER*)t;

    if(this->status == TRANSMITTER_IDLE){
        this->status = TRANSMITTER_DIAL;
        ret = 0;
    }else{
        ret = -1;
    }

    return ret;
}

int transceiver_answer(void* t)
{
    int ret = 0;
    TRANSCEIVER* this = (TRANSCEIVER*)t;

    if(this->status == TRANSCEIVER_INCOMING){
        DBG_F("Transceiver answer\r\n");
        ret = 0;
    }else{
        ret = -1;
    }

    return ret;
}

int transceiver_cancel(void* t)
{
    int ret = 0;
    TRANSCEIVER* this = (TRANSCEIVER*)t;

    if(this->status == TRANSMITTER_DIAL){
        this->status = TRANSMITTER_IDLE;
        ret = 0;
    }else{
        ret = -1;
    }

    return ret;
}

int transceiver_reject(void* t)
{
    int ret = 0;
    TRANSCEIVER* this = (TRANSCEIVER*)t;

    if(this->status == TRANSCEIVER_INCOMING){
        DBG_F("Transceiver amswer\r\n");
        ret = 0;
    }else{
        ret = -1;
    }

    return ret;
}

int transceiver_hangup(void* t)
{
    int ret = 0;
    TRANSCEIVER* this = (TRANSCEIVER*)t;

    if(this->status == TRANSCEIVER_BUSY_LINE){
        this->status = TRANSMITTER_IDLE;
        ret = 0;
    }else{
        ret = -1;
    }

    return ret;
}

int transceiver_ring(void* t, const char *path)
{
    int ret = 0;
    TRANSCEIVER* this = (TRANSCEIVER*)t;

    if(this->status == TRANSCEIVER_BUSY_LINE){
        ret = -1;
    }else if(path != NULL){
        DBG_I("play %s\r\r\n", path);
        my_ringtone_play(path);
        ret = 0;
    }else{
        my_audio_play_stop();
    }

    return ret;
}

transceiverStatus transceiver_get_status(void* t)
{
    transceiverStatus ret = TRANSMITTER_OFFLINE;
    TRANSCEIVER* this = (TRANSCEIVER*)t;

    // if(this->status>=TRANSMITTER_OFFLINE && this->status<=TRANSCEIVER_INCOMING){
        ret = this->status;
    // }else{
    //     ret = -1;
    // }

    return ret;
}

int transceiver_press_green(void* t)
{
    int ret = 0;
    TRANSCEIVER* this = (TRANSCEIVER*)t;
    
    switch(this->status)
    {
    case TRANSMITTER_OFFLINE:
        DBG_F("Transceiver offline\r\n");
        this->ring(this, MP3_CALLSTART_PATH);
        ret = -1;
        break;
    case TRANSMITTER_IDLE:
        DBG_F("Transceiver call imei\r\n");
        this->ring(this, MP3_LINKNET_PATH);
        this->dial(this);
        ret = 0;
        break;
    case TRANSMITTER_DIAL:
        DBG_F("Transceiver had dialed\r\n");
        ret = -1;
        break;
    case TRANSCEIVER_BUSY_LINE:
        DBG_F("Transceiver busy");
        ret = -1;
        break;
    case TRANSCEIVER_INCOMING:
        this->answer(this);
        ret = 0;
        break;
    default:
        break;
    }

    return ret;
}

int transceiver_press_red(void* t)
{
    int ret = 0;
    TRANSCEIVER* this = (TRANSCEIVER*)t;
    
    switch(this->status)
    {
    case TRANSMITTER_OFFLINE:
        DBG_F("Transceiver offline\r\n");
        this->ring(this, MP3_LINKNET_PATH);
        ret = -1;
        break;
    case TRANSMITTER_IDLE:
        DBG_F("Send message\r\n");
        this->ring(this, MP3_CALLSTART_PATH);
        ret = 0;
        break;
    case TRANSMITTER_DIAL:
        DBG_F("Cancel the dial\r\n");
        this->cancel(this);
        ret = 0;
        break;
    case TRANSCEIVER_BUSY_LINE:
        DBG_F("Transceiver busy");
        this->hangup(this);
        this->ring(this, MP3_CLOSELOCK_PATH);
        ret = 0;
        break;
    case TRANSCEIVER_INCOMING:
        DBG_F("Reject incoming call\r\n");
        this->reject(this);
        this->ring(this, MP3_OPENCLOK_PATH);
        ret = 0;
        break;
    default:
        break;
    }

    return ret;
}

int transceiver_set_targt(void* t, TargetInfo target)
{
    int ret = 0;
    TRANSCEIVER* this = (TRANSCEIVER*)t;

    memcpy(&this->cfg.targt, &target, sizeof(TargetInfo));

    return ret;
}

int transceiver_incoming(void* t)
{
    int ret = 0;
    TRANSCEIVER* this = (TRANSCEIVER*)t;

    if(this->status == TRANSMITTER_IDLE){
        this->status = TRANSCEIVER_INCOMING;
        DBG_F("incoming\r\n");
    }else{
        ret = this->status;
    }

    return ret;
}

int transceiver_online(void* t)
{
    int ret = 0;
    TRANSCEIVER* this = (TRANSCEIVER*)t;

    if(this->status == TRANSMITTER_OFFLINE){
        this->status = TRANSMITTER_IDLE;
    }else{
        ret = -1;
    }

    return ret;
}

int transceiver_open_stream(void* t)
{
    int ret = 0;
    TRANSCEIVER* this = (TRANSCEIVER*)t;

    my_audio_play_stop();
    ret += my_record_start(this->cfg.record_cb);

    ret += my_open_audio_stream();
    
    return ret;
}

int transceiver_play_stream(void* t, char* data, int len)
{
    int ret = 0;

    /*需要完全关闭铃声播放才能打开*/
    ret = my_push_audio_stream(data, len);
    return ret;
}

int transceiver_send_stream(void* t)
{
    int ret = 0;
    /*给socket发送消息队列*/

    return ret;
}

int transceiver_close_stream(void* t)
{
    int ret = 0;
    TRANSCEIVER* this = (TRANSCEIVER*)t;
    
    my_record_stop();
    my_close_audio_stream();

    return ret;
}

// int transceiver_

TRANSCEIVER* TRANSCEIVER_CTOR(void)
{
    TRANSCEIVER *this = (TRANSCEIVER*)malloc(sizeof(TRANSCEIVER));
    
    this->init = transceiver_init;
    this->answer = transceiver_answer;
    this->cancel = transceiver_cancel;
    this->dial = transceiver_dial;
    this->hangup = transceiver_hangup;
    this->reject = transceiver_reject;
    this->ring = transceiver_ring;
    this->open_stream = transceiver_open_stream;
    this->play_stream = transceiver_play_stream;
    this->send_stream = transceiver_send_stream;
    this->close_stream = transceiver_close_stream;
    this->interface.get_status = transceiver_get_status;
    this->interface.incoming = transceiver_incoming;
    this->interface.set_targt = transceiver_set_targt;
    this->interface.press_green = transceiver_press_green;
    this->interface.press_red = transceiver_press_red;
    this->interface.online = transceiver_online;
    return this;
}

void TRANSCEIVER_DTOR(TRANSCEIVER* t)
{
    free(t);
}