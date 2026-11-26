/* ===== APP - GateTask (Tinh): xu ly xe vao / xe ra / nut thu cong ===== */
#ifndef TASK_GATE_H
#define TASK_GATE_H

#include <stdint.h>
#include "FreeRTOS.h"

/* Co trang thai cua 2 cong, dung chung voi SensorTask va ngat IR */
extern volatile int pendCar[2], pendBtn[2], gateBusy[2];
extern volatile uint32_t cooldownUntil[2];

BaseType_t task_gate_create(void);
/* ir_isr(gate) khai bao trong hal_gpio.h, dinh nghia trong task_gate.c */

#endif
