#pragma once

#define BATTERY_CUTOFF_VOLTAGE      3000 // 3.0V
#define BATTERY_FULL_VOLTAGE        4200 // 4.2V
#define BATTERY_SAVING_MODE_PER     30   // %

typedef enum{
    BATTERY_CHARGING = 0x00, // 充电中
    BATTERY_WORKING,         // 工作中
    BATTERY_SAVING_MODE,     // 省电中
}BatteryMode;

typedef struct{
    long int voltage;
    int level;
    BatteryMode mode;
}BatteryInfo;

typedef struct{
    int (*update_level)(void*);
    int (*get_level)(void*);
}BATTERY_IMPLEMENTS;

typedef struct{
    BATTERY_IMPLEMENTS interface;
    BatteryInfo info;
    int (*init)(void*);
}BATTERY;

BATTERY* BATTERY_CTOR(void);
void BATTERY_DTOR(BATTERY* t);

