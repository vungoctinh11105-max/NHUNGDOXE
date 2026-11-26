/* ===== HAL GPIO (Phu - feature/hal-gpio-uart) =====
 * IR PA0/PA1 (ngat ngoai EXTI0/1, canh xuong), nut PB14/PB15, coi PB12. */
#ifndef HAL_GPIO_H
#define HAL_GPIO_H

#include <stdbool.h>

void hal_gpio_init(void);          /* IR + ngat, 2 nut, coi */
bool hal_ir_active(int gate);      /* gate 0 = vao, 1 = ra ; true = co vat can */
bool hal_btn_pressed(int gate);    /* true = dang nhan */
void hal_buzzer(bool on);

/* Callback ngat IR - KHONG dinh nghia o day, ma o tang app (app/src/task_gate.c).
 * Chay trong ngat: chi duoc goi API FreeRTOS co duoi ...FromISR. */
void ir_isr(int gate);

#endif
