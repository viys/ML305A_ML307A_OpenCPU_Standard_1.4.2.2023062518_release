#include <stdint.h>
#include "cm_adc.h"
#include "cm_os.h"
#include "drv_adc.h"
#include "drv_uart.h"

#define DBG_NAME "adc"

long int my_adc_get(void)
{
    int ret = 0;
    long int voltage;
	//测试ADC0
	ret = cm_adc_read(CM_ADC_0, &voltage);
	if(ret != 0)
	{
		DBG_I("adcCM_ADC_0 read err,ret=0x%08x\r\n", ret);
		return -1;
	}else{
        return voltage;
    }
}
