#ifndef APP_CFG_H
#define APP_CFG_H

#include "FreeRTOS.h"

/* ===== Do uu tien task (so lon = uu tien cao) =====
 * COMM > SENSOR > DISPLAY
 */
#define TASK_COMM_PRIO       (tskIDLE_PRIORITY + 3)
#define TASK_SENSOR_PRIO     (tskIDLE_PRIORITY + 2)
#define TASK_DISPLAY_PRIO    (tskIDLE_PRIORITY + 1)

/* ===== Stack size (don vi: words) ===== */
#define TASK_SENSOR_STACK    256
#define TASK_DISPLAY_STACK   512
#define TASK_COMM_STACK      512

/* ===== Chu ky task (ms) ===== */
#define TASK_SENSOR_PERIOD_MS   100
#define TASK_DISPLAY_PERIOD_MS  200

/* ===== Queue ===== */
#define SENSOR_QUEUE_LEN     10   /* so phan tu SensorData_t */

#endif /* APP_CFG_H */
