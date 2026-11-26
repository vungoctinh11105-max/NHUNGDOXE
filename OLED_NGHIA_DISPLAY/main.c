/*
 * BAI DO XE 4 CHO - STM32F103C8T6 + FreeRTOS (Keil uVision, ARMCC 5)
 *
 *   hal/   driver phan cung (Giai doan 1)
 *     hal_timer.c  Tinh   TIM3 PWM servo
 *     hal_gpio.c   Phu    IR + ngat EXTI, nut, coi
 *     hal_uart.c   Phu    USART1 toi ESP32
 *     hal_i2c.c    Nghia  I2C mem cho OLED
 *   app/   task FreeRTOS (Giai doan 2)
 *     GateTask    prio 3  su kien gateSem       task_gate.c, task_sensor.c   Tinh
 *     SensorTask  prio 2  20 ms
 *     CommTask    prio 1  1 s                   task_comm.c, car_db.c        Phu
 *     DisplayTask prio 2  su kien displayQueue  task_display.c, oled.c       Nghia
 */
#include "FreeRTOS.h"
#include "task.h"
#include "app_cfg.h"
#include "app_ipc.h"
#include "hal_gpio.h"
#include "hal_timer.h"
#include "hal_i2c.h"
#include "hal_uart.h"
#include "task_gate.h"
#include "task_sensor.h"
#include "task_display.h"
#include "task_comm.h"

/* ---------- hook FreeRTOS ---------- */
void vApplicationMallocFailedHook(void)
{
    hal_buzzer(true);
    for (;;) { }
}

void vApplicationStackOverflowHook(TaskHandle_t t, char *name)
{
    (void)t; (void)name;
    hal_buzzer(true);
    for (;;) { }
}

int main(void)
{
    BaseType_t ok = pdTRUE;

    /* 1. Hardware & HAL init */
    hal_gpio_init();                   /* IR + ngat, nut, coi */
    hal_timer_pwm_init();              /* servo */
    hal_i2c_init();                    /* chan OLED */
    hal_uart_init(115200);             /* ESP32 */
    hal_servo_us(0, SERVO_CLOSED_US);
    hal_servo_us(1, SERVO_CLOSED_US);

    /* 2. FreeRTOS IPC */
    ok &= app_ipc_init();

    /* 3. Create tasks */
    ok &= task_gate_create();
    ok &= task_sensor_create();
    ok &= task_display_create();
    ok &= task_comm_create();
    if (ok != pdTRUE) {
        hal_buzzer(true);              /* het heap: coi keu lien tuc */
        for (;;) { }
    }

    /* 4. Start scheduler */
    vTaskStartScheduler();
    for (;;) { }
}
