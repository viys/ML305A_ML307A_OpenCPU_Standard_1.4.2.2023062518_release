#include "motor.h"
#include "cm_mem.h"
#include "drv_gpio.h"
#include "drv_uart.h"
#include "includes.h"
#include "sys.h"

#define DBG_NAME "lock"

void bdr_motor_init(void) {
    cm_gpio_cfg_t cfg = {.direction = CM_GPIO_DIRECTION_OUTPUT,
                         .pull = CM_GPIO_PULL_DOWN};
    my_io_init_2(MOTOR_IN0_GPIO_NUM, MOTOR_IN0_GPIO_PIN, cfg,
                 CM_IOMUX_FUNC_FUNCTION2);
    my_io_init_2(MOTOR_IN1_GPIO_NUM, MOTOR_IN1_GPIO_PIN, cfg,
                 CM_IOMUX_FUNC_FUNCTION2);
    my_io_init_2(MOTOR_IN2_GPIO_NUM, MOTOR_IN2_GPIO_PIN, cfg,
                 CM_IOMUX_FUNC_FUNCTION2);
}

void bdr_hall_init(void) {
    cm_gpio_cfg_t cfg = {.direction = CM_GPIO_DIRECTION_INPUT,
                         .pull = CM_GPIO_PULL_DOWN};

    my_io_init_2(HALL1_GPIO_NUM, HALL1_GPIO_PIN, cfg, CM_IOMUX_FUNC_FUNCTION2);
    my_io_init_2(HALL2_GPIO_NUM, HALL2_GPIO_PIN, cfg, CM_IOMUX_FUNC_FUNCTION2);
    my_io_init_2(HALL3_GPIO_NUM, HALL3_GPIO_PIN, cfg, CM_IOMUX_FUNC_FUNCTION2);

    cfg.direction = CM_GPIO_DIRECTION_OUTPUT;
    cfg.pull = CM_GPIO_PULL_UP;
    my_io_init_2(HALL_EN_GPIO_NUM, HALL_EN_GPIO_PIN, cfg,
                 CM_IOMUX_FUNC_FUNCTION2);
}

static Hall_State bdr_hall_get_state(void) {
    Hall_State state;

    // osDelayMs(200);
    bool hall1 = my_io_get(HALL1_GPIO_NUM);
    bool hall2 = my_io_get(HALL2_GPIO_NUM);
    bool hall3 = my_io_get(HALL3_GPIO_NUM);

    // DBG_I("Hall 1[%d] 2[%d] 3[%d]\r\n", hall1, hall2, hall3);
    if (HALL_ON == hall1 && HALL_OFF == hall2) {
        state = LOCK_OPEN;
    } else if (HALL_OFF == hall1 && HALL_ON == hall2) {
        /* 门锁关闭 */
        state = LOCK_CLOSE;
    } else if (HALL_OFF == hall1 && HALL_OFF == hall2) {
        state = LOCK_MID;
    } else {
        DBG_I("Hall 1[%d] 2[%d] 3[%d]\r\n", hall1, hall2, hall3);
        state = LOCK_MID;
    }

    return state;
}

/*
 * 控制电机运动
 * direction 运动的方向     0 代表正转  关锁      1 代表反转 开锁
 *should be: 1 代表正转  关锁      0 代表反转 开锁
 * */
void bdr_motor_opt(Motor_OPT opt) {
    switch (opt) {
        case MOTOR_OPEN_LOCK:
            my_io_sw(MOTOR_IN0_GPIO_NUM, MOTOR_INT_ON);
            my_io_sw(MOTOR_IN1_GPIO_NUM, MOTOR_INT_ON);
            my_io_sw(MOTOR_IN2_GPIO_NUM, MOTOR_INT_OFF);
            break;
        case MOTOR_CLOSE_LOCK:
            my_io_sw(MOTOR_IN0_GPIO_NUM, MOTOR_INT_ON);
            my_io_sw(MOTOR_IN1_GPIO_NUM, MOTOR_INT_OFF);
            my_io_sw(MOTOR_IN2_GPIO_NUM, MOTOR_INT_ON);
            break;
        case MOTOR_STOP:
            my_io_sw(MOTOR_IN0_GPIO_NUM, MOTOR_INT_ON);  //??
            my_io_sw(MOTOR_IN1_GPIO_NUM, MOTOR_INT_ON);
            my_io_sw(MOTOR_IN2_GPIO_NUM, MOTOR_INT_ON);
            my_io_sw(MOTOR_IN0_GPIO_NUM, MOTOR_INT_OFF);  //??
            break;
        default:
            my_io_sw(MOTOR_IN0_GPIO_NUM, MOTOR_INT_ON);  //??
            my_io_sw(MOTOR_IN1_GPIO_NUM, MOTOR_INT_ON);
            my_io_sw(MOTOR_IN2_GPIO_NUM, MOTOR_INT_ON);
            my_io_sw(MOTOR_IN0_GPIO_NUM, MOTOR_INT_OFF);  //??
            break;
    }
}

void lock_init() {
    bdr_motor_init();
    bdr_hall_init();
    DBG_F("lock init\r\n");
}

#define MOTOR_OPT

static int lock_open(void* t) {
    Hall_State state;
    int time = 0;
    my_io_sw(HALL_EN_GPIO_NUM, 0);
    osDelayMs(200);
    state = bdr_hall_get_state();

    if (LOCK_OPEN != state) {
        DBG_I("Lock is close, need to open\r\n");
#ifdef MOTOR_OPT
        while (LOCK_OPEN != state && time < 250) {
            ++time;
            if (LOCK_OPEN != state) {
                bdr_motor_opt(MOTOR_OPEN_LOCK);
            } else {
                bdr_motor_opt(MOTOR_STOP);
            }
            osDelayMs(5);
            state = bdr_hall_get_state();
        }
        bdr_motor_opt(MOTOR_STOP);
#endif
    } else {
        DBG_I("Lock is open, no need to open\r\n");
    }
    my_io_sw(HALL_EN_GPIO_NUM, 1);

    return 0;
}

static int lock_close(void* t) {
    Hall_State state;
    int time = 0;
    my_io_sw(HALL_EN_GPIO_NUM, 0);
    osDelayMs(200);
    state = bdr_hall_get_state();

    if (LOCK_CLOSE != state) {
        DBG_I("Lock is open, need to close\r\n");
#ifdef MOTOR_OPT
        while (LOCK_CLOSE != state && time < 250) {
            ++time;
            if (LOCK_CLOSE != state) {
                bdr_motor_opt(MOTOR_CLOSE_LOCK);
            } else {
                bdr_motor_opt(MOTOR_STOP);
            }
            osDelayMs(5);
            state = bdr_hall_get_state();
        }
        bdr_motor_opt(MOTOR_STOP);
#endif
    } else {
        DBG_I("Lock is close, no need to close\r\n");
    }
    my_io_sw(HALL_EN_GPIO_NUM, 1);

    return 0;
}

LOCK* LOCK_CTOR(void) {
    LOCK* this = (LOCK*)cm_malloc(sizeof(LOCK));

    this->init = lock_init;
    this->api.open = lock_open;
    this->api.close1 = lock_close;

    return this;
}

void LOCK_DTOR(LOCK* t) {
    cm_free(t);
}
