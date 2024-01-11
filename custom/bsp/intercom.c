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


INTERCOM* INTERCOM_CTOR(TRANSCEIVER* parent)
{
    INTERCOM *this = (INTERCOM*)malloc(sizeof(INTERCOM));

    this->parentState = &parent->status;

    this->init = intercom_init;
    

    return this;
}