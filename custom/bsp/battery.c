#include "battery.h"
#include "drv_adc.h"
#include "string.h"
#include <stdlib.h>
#include "drv_uart.h"

#define DBG_NAME "battery"

#define BUFFER_SIZE 5
#define LEVELS 3

typedef struct {
    long int buffer[BUFFER_SIZE];   // 存储样本数据的缓冲区
    int index;                      // 当前样本的索引
    int count;                      // 已存储的样本数量
} MovingAverageFilter;

// 初始化滤波器
void initialize(MovingAverageFilter* filter) {
    filter->index = 0;
    filter->count = 0;
}

// 更新样本数据并返回滤波结果
long int update(MovingAverageFilter* filter, long int sample) {
    // 更新缓冲区
    filter->buffer[filter->index] = sample;
    filter->index = (filter->index + 1) % BUFFER_SIZE;
    if (filter->count < BUFFER_SIZE) {
        filter->count++;
    }

    // 计算滤波结果
    long int result = 0;
    int levels = LEVELS;
    int count = filter->count;
    int index = filter->index;
    for (int i = 0; i < levels; i++) {
        int n = count;
        long int sum = 0;
        for (int j = 0; j < n; j++) {
            sum += filter->buffer[(index + BUFFER_SIZE - j) % BUFFER_SIZE];
        }
        result = sum / n;
        count /= 2;
    }

    return result;
}

MovingAverageFilter batteryFilter;

int battery_init(void* t)
{
    int ret = 0;
    BATTERY* this = (BATTERY*)t;

    initialize(&batteryFilter);

    this->interface.update_level(this);
    

    return ret;
}

int battery_update_level(void* t)
{
    long int voltage = 0;
    BATTERY* this = (BATTERY*)t;
    
    voltage = my_adc_get();
    this->info.voltage = update(&batteryFilter, voltage) * 4;

    this->info.level = (int)((float)(this->info.voltage - BATTERY_CUTOFF_VOLTAGE) / (float)(BATTERY_FULL_VOLTAGE - BATTERY_CUTOFF_VOLTAGE) * 100);
    
    if(this->info.level >= BATTERY_CHARG_END_PER){
        /* 电池充电完成 */
        this->info.mode = BATTERY_CHARGED;
    }else if(this->info.level >= BATTERY_CHARGING_PER){
        /* 电池充电中 */
        this->info.mode = BATTERY_CHARGING;
    }else if(this->info.level >= BATTERY_SAVING_MODE_PER){
        /* 电池充工作中 */
        this->info.mode = BATTERY_WORKING;
    }else if(this->info.level < BATTERY_SAVING_MODE_PER){
        /* 电池省电中 */
        this->info.mode = BATTERY_SAVING_MODE;
    }else{}
    
    return this->info.level;
}

int battery_get_level(void* t)
{
    BATTERY* this = (BATTERY*)t;
    if(this->info.level<0){
        return -1;
    }else{
        return this->info.level;
    }
}

BATTERY* BATTERY_CTOR(void)
{
    BATTERY *this = (BATTERY*)malloc(sizeof(BATTERY));
    this->interface.init = battery_init;
    this->interface.update_level = battery_update_level;
    this->interface.get_level = battery_get_level;
    return (BATTERY*)this;
}

void BATTERY_DTOR(BATTERY* t)
{
    free(t);
}

