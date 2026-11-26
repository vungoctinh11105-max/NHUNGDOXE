/* SensorTask (prio 2, chu ky 20 ms): chong doi IR + nut.
 * IR/nut on dinh 3 lan lien tiep (60 ms) -> dat co pendCar/pendBtn va danh thuc GateTask.
 * IR con vat can sau khi het cooldown -> thu lai (du ngat canh xuong da qua). */
#include "app_cfg.h"
#include "app_ipc.h"
#include "hal_gpio.h"
#include "task_gate.h"
#include "task_sensor.h"

static void SensorTask(void *arg)
{
    uint8_t irCnt[2] = {0, 0}, btnCnt[2] = {0, 0};
    int btnHeld[2] = {0, 0};
    TickType_t last = xTaskGetTickCount();
    (void)arg;
    for (;;) {
        int g;
        for (g = 0; g < 2; g++) {
            if (hal_ir_active(g)) { if (irCnt[g] < 255) irCnt[g]++; } else irCnt[g] = 0;
            if (irCnt[g] >= 3 && !gateBusy[g] && !pendCar[g] && millis() > cooldownUntil[g]) {
                pendCar[g] = 1;
                xSemaphoreGive(gateSem);
            }
            if (hal_btn_pressed(g)) {
                if (btnCnt[g] < 255) btnCnt[g]++;
                if (btnCnt[g] >= 3 && !btnHeld[g]) {
                    btnHeld[g] = 1;
                    pendBtn[g] = 1;
                    xSemaphoreGive(gateSem);
                }
            } else { btnCnt[g] = 0; btnHeld[g] = 0; }
        }
        vTaskDelayUntil(&last, pdMS_TO_TICKS(20));
    }
}

BaseType_t task_sensor_create(void)
{
    return xTaskCreate(SensorTask, "Sensor", TASK_SENSOR_STACK, NULL, TASK_SENSOR_PRIO, NULL);
}
