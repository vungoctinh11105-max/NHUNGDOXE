#ifndef APP_IPC_H
#define APP_IPC_H

#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "event_groups.h"

/* ===== Kieu du lieu gui qua Queue (nhom thong nhat) ===== */
typedef struct {
    int16_t  temperature;   /* don vi 0.1 do C */
    uint16_t humidity;      /* don vi 0.1 %    */
} SensorData_t;

/* ===== Handle dung chung - duoc tao trong app_ipc.c ===== */
extern QueueHandle_t      xSensorQueue;   /* Sensor -> Display/Comm */
extern SemaphoreHandle_t  xUartMutex;     /* bao ve UART dung chung */
extern EventGroupHandle_t xSystemEvents;  /* co bao trang thai he thong */

/* Bit trong xSystemEvents */
#define EVT_SENSOR_READY   (1 << 0)
#define EVT_COMM_CONNECTED (1 << 1)

void app_ipc_init(void);

#endif /* APP_IPC_H */
