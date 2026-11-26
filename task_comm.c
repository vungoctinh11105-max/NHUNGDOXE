/* CommTask (prio 1): moi 1 s doc UART (ADD tu web) va gui STATE cho ESP32.
 * requestPlate() duoc GateTask (Phan 1) goi khi co xe. */
#include <stdlib.h>
#include <string.h>
#include "app_cfg.h"
#include "app_ipc.h"
#include "hal_uart.h"
#include "car_db.h"
#include "task_comm.h"

static volatile int plateReady;
static char gotPlate[16];

void sendState(void)
{
    static char buf[400];                      /* chi CommTask va GateTask goi, nhung bao ve bang uartMutex */
    xSemaphoreTake(uartMutex, portMAX_DELAY);
    car_db_format_state(buf, sizeof(buf));     /* ben trong giu slotMutex */
    hal_uart_puts(buf);
    xSemaphoreGive(uartMutex);
}

static void handleLine(char *l)
{
    if (strncmp(l, "PLATE:", 6) == 0) {
        strncpy(gotPlate, l + 6, sizeof(gotPlate) - 1);
        gotPlate[sizeof(gotPlate) - 1] = 0;
        plateReady = 1;
    } else if (strncmp(l, "ADD:", 4) == 0) {            /* ADD:29A12345:5000 (co the am) */
        char *c = strrchr(l, ':');
        if (c && c > l + 4) {
            *c = 0;
            car_db_add_money(l + 4, atol(c + 1));
            sendState();
        }
    } else if (strncmp(l, "STATE?", 6) == 0) {
        sendState();
    }
}

/* doc cac byte UART dang cho, xu ly tung dong (GateTask va CommTask cung goi) */
static void pollUart(void)
{
    static char line[64];
    static size_t n = 0;
    for (;;) {
        char done[64];
        int have = 0, c;
        xSemaphoreTake(uartMutex, portMAX_DELAY);
        while (!have && (c = hal_uart_getc()) >= 0) {
            if (c == '\n') {
                line[n] = 0;
                if (n) { memcpy(done, line, n + 1); have = 1; }
                n = 0;
            } else if (c != '\r' && n < sizeof(line) - 1) {
                line[n++] = (char)c;
            }
        }
        xSemaphoreGive(uartMutex);
        if (!have) break;
        handleLine(done);                          /* goi khi da tha uartMutex */
    }
}

/* xin ESP quet bien so, cho toi da 25 s */
void requestPlate(char *out, size_t outLen)
{
    uint32_t t0;
    plateReady = 0;
    pollUart();                                    /* bo du lieu cu */
    plateReady = 0;
    xSemaphoreTake(uartMutex, portMAX_DELAY);
    hal_uart_puts("SCAN");
    xSemaphoreGive(uartMutex);
    t0 = millis();
    while (!plateReady && millis() - t0 < SCAN_TIMEOUT_MS) {
        pollUart();
        vTaskDelay(pdMS_TO_TICKS(5));
    }
    if (plateReady) { strncpy(out, gotPlate, outLen - 1); out[outLen - 1] = 0; }
    else { strncpy(out, "NONE", outLen - 1); out[outLen - 1] = 0; }
}

/* ---------- TASK ---------- */
static void CommTask(void *arg)
{
    TickType_t last = xTaskGetTickCount();
    (void)arg;
    for (;;) {
        pollUart();
        sendState();
        vTaskDelayUntil(&last, pdMS_TO_TICKS(1000));
    }
}

BaseType_t task_comm_create(void)
{
    return xTaskCreate(CommTask, "Comm", TASK_COMM_STACK, NULL, TASK_COMM_PRIO, NULL);
}
