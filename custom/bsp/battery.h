#pragma once

#define BATTERY_CUTOFF_VOLTAGE  3000  // 3.0V
#define BATTERY_FULL_VOLTAGE    4128  // 4.2V
#define BATTERY_SAVING_MODE_PER 30    // %
#define BATTERY_CHARG_END_PER   135   // % 拔掉后为94%
#define BATTERY_CHARGING_PER    110   // %

typedef enum {
    BATTERY_CHARGING = 0x00,  // 充电中
    BATTERY_CHARGED,          // 充电完成
    BATTERY_WORKING,          // 工作中
    BATTERY_SAVING_MODE,      // 省电中
} BatteryMode;

typedef struct {
    long int voltage;
    int level;
    BatteryMode mode;
} BatteryInfo;

typedef struct {
    int (*init)(void*);
    int (*update_level)(void*);
    int (*get_level)(void*);
    BatteryMode (*get_mode)(void*);
} BATTERY_IMPLEMENTS;

typedef struct {
    BATTERY_IMPLEMENTS interface;
    BatteryInfo info;
} BATTERY;

BATTERY* BATTERY_CTOR(void);
void BATTERY_DTOR(BATTERY* t);
