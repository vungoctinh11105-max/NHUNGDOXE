/* ===== APP - DisplayTask (Nghia): OLED + coi, ham gui thong bao ===== */
#ifndef TASK_DISPLAY_H
#define TASK_DISPLAY_H

#include <stdint.h>
#include "FreeRTOS.h"

BaseType_t task_display_create(void);

/* Goi tu task khac (vd GateTask): day ban tin vao displayQueue.
 *   l1 chu to, l2/l3 chu nho; holdMs = 0 -> giu den ban tin sau;
 *   beeps = so tieng bip ngan; longMs = bip dai (ms). */
void showMsg(const char *l1, const char *l2, const char *l3, uint16_t holdMs, uint8_t beeps, uint16_t longMs);
void showIdle(void);          /* quay ve man hinh cho */

#endif
