#include "intercom.h"
#include "drv_audio.h"
#include "stdlib.h"
#include "drv_gpio.h"

int intercom_init(void* t, IntercomCfg cfg){
    int ret = 0;

    INTERCOM* this = (INTERCOM*)t;

    memcpy(&this->cfg, &cfg, sizeof(IntercomCfg));

    my_amr_load_files();

    ret += my_audio_init(this->cfg.volume, this->cfg.gain);

    ret += my_audio_io_init();

    return ret;    
}

int intercom_audio_sw(void* t, int sw)
{
    int ret = 0;

    ret = my_audio_io_sw(sw);
    
    return ret;
}

/*
typedef enum{
    TRANSMITTER_OFFLINE = 0x00,
    TRANSMITTER_IDLE,
    TRANSMITTER_DIAL,
    TRANSCEIVER_BUSY_LINE,
    TRANSCEIVER_INCOMING
}transceiverStatus;
*/

int intercom_ring_play(void* t)
{
    int ret = 0;
    INTERCOM* this = (INTERCOM*)t;

    switch(*(this->parentState))
    {
    case TRANSMITTER_OFFLINE:
        my_ring_play(MP3_CALLSTART_PATH, this->cfg.player_cb);
        break;
    case TRANSMITTER_IDLE:
        my_ring_play(MP3_CLOSELOCK_PATH, this->cfg.player_cb);
        break;
    case TRANSMITTER_TARGETBUSY:
        my_ring_play(MP3_LINKNET_PATH, this->cfg.player_cb);
        break;
    case TRANSCEIVER_HANGUP:
        my_ring_play(MP3_OPENCLOK_PATH, this->cfg.player_cb);
        break;
    case TRANSCEIVER_TIMEOUT:
        my_ring_play(MP3_LINKNET_PATH, this->cfg.player_cb);
        break;
    case TRANSCEIVER_INCOMING:
        ret = -1;
        break;
    default:
        break;
    }

    return ret;
}

int intercom_open_stream(void* t)
{
    int ret = 0;
    INTERCOM* this = (INTERCOM*)t;

    my_audio_play_stop();
    ret += my_record_start(this->cfg.record_cb);

    ret += my_open_audio_stream();
    
    return ret;
}

int intercom_push_stream(void* t, char* data, int len)
{
    int ret = 0;

    /*需要完全关闭铃声播放才能打开*/
    ret = my_push_audio_stream(data, len);
    return ret;
}

int intercom_close_stream(void* t)
{
    int ret = 0;
    INTERCOM* this = (INTERCOM*)t;
    
    my_record_stop();
    my_close_audio_stream();
}

INTERCOM* INTERCOM_CTOR(TRANSCEIVER* parent)
{
    INTERCOM *this = (INTERCOM*)malloc(sizeof(INTERCOM));

    this->parentState = &parent->status;

    this->init = intercom_init;
    this->interface.open_stream = intercom_open_stream;
    this->interface.close_stream = intercom_close_stream;

    return this;
}