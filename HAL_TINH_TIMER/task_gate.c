/* GateTask (prio 3): cho gateSem, roi xu ly xe vao / xe ra / nut thu cong.
 * Driver: hal_gpio (IR, nut), hal_timer (servo).
 * Goi sang: task_display (showMsg/showIdle), task_comm + car_db (requestPlate, car_db_*, sendState). */
#include <stdio.h>
#include <string.h>
#include "app_cfg.h"
#include "app_ipc.h"
#include "hal_gpio.h"
#include "hal_timer.h"
#include "task_gate.h"
#include "task_display.h"
#include "car_db.h"
#include "task_comm.h"

static int gateOpen[2];
volatile int pendCar[2], pendBtn[2], gateBusy[2];
volatile uint32_t cooldownUntil[2];

/* ---------- barie ---------- */
static void setGate(int g, int open)
{
    hal_servo_us(g, open ? SERVO_OPEN_US : SERVO_CLOSED_US);
    gateOpen[g] = open;
}

/* mo barie, doi xe di qua (IR het vat can 1,5 s) roi dong */
static void passGate(int g)
{
    uint32_t t0 = millis(), lastSeen = 0;
    int seen = 0;
    setGate(g, 1);
    while (millis() - t0 < 20000UL) {
        if (hal_ir_active(g)) { seen = 1; lastSeen = millis(); }
        if (seen && millis() - lastSeen > 1500) break;
        vTaskDelay(pdMS_TO_TICKS(20));
    }
    setGate(g, 0);
}

/* ---------- XE VAO (cong 0) ---------- */
static void handleEntry(void)
{
    char plate[16], l3[24];
    int res;
    long bal = 0;

    if (car_db_parked_count() >= MAX_SLOTS) {
        showMsg("BAI DAY", "Het cho trong", "", 1000, 0, 1000);
        cooldownUntil[0] = millis() + 3000;
        return;
    }
    showMsg("DANG QUET", "Xe vao...", "", 0, 0, 0);
    requestPlate(plate, sizeof(plate));
    if (strcmp(plate, "NONE") == 0 || strlen(plate) < 5) {
        showMsg("KHONG DOC", "Khong thay bien so", "Chinh xe, thu lai", 2000, 3, 0);
        cooldownUntil[0] = millis() + 3000;
        return;
    }

    res = car_db_check_in(plate, &bal);

    snprintf(l3, sizeof(l3), "Con: %ldd", bal);
    switch (res) {
    case CHECKIN_ALREADY:  showMsg("DA TRONG BAI", plate, "Xe nay da vao roi", 2500, 3, 0); cooldownUntil[0] = millis() + 3000; return;
    case CHECKIN_DB_FULL:  showMsg("DAY BO NHO", "Het cho luu ho so", "", 2500, 3, 0);       cooldownUntil[0] = millis() + 3000; return;
    case CHECKIN_NO_MONEY: showMsg("HET TIEN", plate, "Vui long nap them", 3000, 0, 800);    cooldownUntil[0] = millis() + 3000; return;
    default: break;
    }
    showMsg(res == CHECKIN_NEW ? "XE MOI" : "MOI VAO", plate, l3, 0, 1, 0);
    sendState();
    passGate(0);
    showIdle();
    cooldownUntil[0] = millis() + 2000;
}

/* ---------- XE RA (cong 1) ---------- */
static void handleExit(void)
{
    char plate[16], shown[12], l3[24];
    long bal = 0;
    int deleted = 0;

    showMsg("DANG QUET", "Xe ra...", "", 0, 0, 0);
    requestPlate(plate, sizeof(plate));
    if (strcmp(plate, "NONE") == 0 || strlen(plate) < 5) {
        showMsg("KHONG DOC", "Khong thay bien so", "Chinh xe, thu lai", 2000, 3, 0);
        cooldownUntil[1] = millis() + 3000;
        return;
    }
    if (!car_db_check_out(plate, shown, sizeof(shown), &bal, &deleted)) {
        showMsg("KHONG THAY", plate, "Xe chua vao bai", 2500, 3, 0);
        cooldownUntil[1] = millis() + 3000;
        return;
    }
    if (deleted) snprintf(l3, sizeof(l3), "Het tien, xoa HS");
    else         snprintf(l3, sizeof(l3), "Con: %ldd", bal);
    showMsg("XE RA", shown, l3, 0, 2, 0);
    sendState();
    passGate(1);
    showIdle();
    cooldownUntil[1] = millis() + 2000;
}

/* ---------- ngat ngoai IR: danh thuc GateTask bang binary semaphore ---------- */
void ir_isr(int g)
{
    BaseType_t woken = pdFALSE;
    if (gateBusy[g] || gateSem == NULL) return;
    pendCar[g] = 1;
    xSemaphoreGiveFromISR(gateSem, &woken);
    portYIELD_FROM_ISR(woken);
}

/* ---------- TASK ---------- */
static void GateTask(void *arg)
{
    (void)arg;
    for (;;) {
        int g;
        xSemaphoreTake(gateSem, portMAX_DELAY);
        for (g = 0; g < 2; g++) {
            if (pendBtn[g]) {
                pendBtn[g] = 0;
                setGate(g, !gateOpen[g]);
                showMsg(g == 0 ? "CONG VAO" : "CONG RA", gateOpen[g] ? "Mo thu cong" : "Dong thu cong", "", 1000, 1, 0);
                showIdle();
            }
            if (pendCar[g]) {
                pendCar[g] = 0;
                vTaskDelay(pdMS_TO_TICKS(60));                 /* chong doi: IR phai con vat can */
                if (millis() < cooldownUntil[g] || !hal_ir_active(g)) continue;
                gateBusy[g] = 1;
                if (g == 0) handleEntry(); else handleExit();
                pendCar[g] = 0;
                gateBusy[g] = 0;
            }
        }
    }
}

BaseType_t task_gate_create(void)
{
    return xTaskCreate(GateTask, "Gate", TASK_GATE_STACK, NULL, TASK_GATE_PRIO, NULL);
}
