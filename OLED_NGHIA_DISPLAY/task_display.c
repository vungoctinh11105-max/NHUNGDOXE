/* DisplayTask (prio 2): cho displayQueue; thong bao moi cat ngang thoi gian giu cua thong bao cu.
 * Driver: oled.c (tren hal_i2c), hal_gpio (coi). Man hinh cho lay danh sach xe tu car_db. */
#include <stdio.h>
#include <string.h>
#include "app_cfg.h"
#include "app_ipc.h"
#include "hal_gpio.h"
#include "oled.h"
#include "task_display.h"
#include "car_db.h"

/* ---------- gui thong bao cho DisplayTask ---------- */
void showMsg(const char *l1, const char *l2, const char *l3, uint16_t holdMs, uint8_t beeps, uint16_t longMs)
{
    DispMsg m;
    memset(&m, 0, sizeof(m));
    strncpy(m.l1, l1, sizeof(m.l1) - 1);
    strncpy(m.l2, l2, sizeof(m.l2) - 1);
    strncpy(m.l3, l3, sizeof(m.l3) - 1);
    m.holdMs = holdMs; m.beeps = beeps; m.longMs = longMs;
    xQueueSend(displayQueue, &m, pdMS_TO_TICKS(200));
}

void showIdle(void)
{
    DispMsg m;
    memset(&m, 0, sizeof(m));
    m.idle = 1;
    xQueueSend(displayQueue, &m, pdMS_TO_TICKS(200));
}

/* ---------- ve man hinh ---------- */
static void buzzMs(int ms)
{
    hal_buzzer(true);
    vTaskDelay(pdMS_TO_TICKS(ms));
    hal_buzzer(false);
}

static void drawIdle(void)
{
    char lines[5][24];
    char head[24];
    int rows, parked, r;

    rows = car_db_parked_list(lines, 5, &parked);      /* chup bang ho so (car_db tu giu slotMutex) */
    snprintf(head, sizeof(head), "BAI XE  Trong: %d/%d", MAX_SLOTS - parked, MAX_SLOTS);

    xSemaphoreTake(i2cMutex, portMAX_DELAY);
    oled_clear();
    oled_text(0, 0, head, 1);
    oled_hline(10);
    for (r = 0; r < rows; r++) oled_text(0, 14 + r * 10, lines[r], 1);
    if (rows == 0) oled_text(0, 24, "Chua co xe trong bai", 1);
    oled_flush();
    xSemaphoreGive(i2cMutex);
}

static void drawMsg(const DispMsg *m)
{
    xSemaphoreTake(i2cMutex, portMAX_DELAY);
    oled_clear();
    oled_text(0, 0, m->l1, 2);
    oled_text(0, 28, m->l2, 1);
    oled_text(0, 44, m->l3, 1);
    oled_flush();
    xSemaphoreGive(i2cMutex);
}

/* ---------- TASK ---------- */
static void DisplayTask(void *arg)
{
    DispMsg m;
    int holdForever = 0;
    uint32_t holdUntil = 0;
    (void)arg;
    xSemaphoreTake(i2cMutex, portMAX_DELAY);
    oled_init();
    xSemaphoreGive(i2cMutex);
    drawIdle();
    for (;;) {
        if (xQueueReceive(displayQueue, &m, pdMS_TO_TICKS(500)) == pdTRUE) {
            uint8_t i;
            if (m.idle) { holdForever = 0; holdUntil = 0; drawIdle(); continue; }
            drawMsg(&m);
            for (i = 0; i < m.beeps; i++) { buzzMs(80); vTaskDelay(pdMS_TO_TICKS(80)); }
            if (m.longMs) buzzMs(m.longMs);
            holdForever = (m.holdMs == 0);
            holdUntil = millis() + m.holdMs;
        } else if (!holdForever && millis() >= holdUntil) {
            drawIdle();                                /* lam moi man hinh cho */
            holdUntil = 0;
        }
    }
}

BaseType_t task_display_create(void)
{
    return xTaskCreate(DisplayTask, "Display", TASK_DISPLAY_STACK, NULL, TASK_DISPLAY_PRIO, NULL);
}
