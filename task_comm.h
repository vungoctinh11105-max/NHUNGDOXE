/* ===== APP - CommTask (Phu): giao thuc voi ESP32 =====
 *   STM32 -> ESP : "SCAN"                  xin chup va doc bien so
 *                  "STATE;n;max;bien,du,gui;..."  trang thai bai
 *   ESP -> STM32 : "PLATE:<bien so>"       ket qua doc bien so
 *                  "ADD:<bien so>:<tien>"  nap/tru tien tu web
 *                  "STATE?"                hoi trang thai
 */
#ifndef TASK_COMM_H
#define TASK_COMM_H

#include <stddef.h>
#include "FreeRTOS.h"

BaseType_t task_comm_create(void);

void sendState(void);                          /* gui STATE cho ESP32 */
void requestPlate(char *out, size_t outLen);   /* gui SCAN, cho PLATE toi da 25 s; "NONE" neu khong co */

#endif
