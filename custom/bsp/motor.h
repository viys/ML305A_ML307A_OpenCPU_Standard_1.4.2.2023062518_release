#pragma once

typedef enum { MOTOR_OPEN_LOCK = 0x01, MOTOR_CLOSE_LOCK, MOTOR_STOP } Motor_OPT;

/* 5s检测一次以防门锁异常伸出 */
typedef enum { LOCK_OPEN = 0x00, LOCK_CLOSE, LOCK_MID } Hall_State;

void bdr_motor_opt(Motor_OPT opt);
void bdr_motor_init(void);
void bdr_hall_init(void);

typedef enum {
    LOCK_HALL_STATE_OPEN = 0x00,
    LOCK_HALL_STATE_CLOSE
} LOCK_HALL_STATE;

typedef enum { LOCK_STATE_OPEN = 0x00, LOCK_STATE_CLOSE } LOCK_STATE;

typedef struct {
    char opt;
    char param;
} Lock_Msg;

typedef struct {
    LOCK_STATE (*get_state)(void*);
    int (*close1)(void*);
    int (*open)(void*);
} LOCK_IMPLEMENTS;

typedef struct {
    LOCK_IMPLEMENTS api;
    void (*init)(void*);
    LOCK_STATE state;
} LOCK;

LOCK* LOCK_CTOR(void);
void LOCK_DTOR(LOCK* t);
