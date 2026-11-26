/*
 * BAI DO XE 4 CHO - STM32F103C8T6 + FreeRTOS (Keil uVision, ARMCC 5)
 *
 *   IR vao OUT -> PA0      IR ra OUT -> PA1         (co vat can = muc THAP, ngat ngoai)
 *   Servo vao  -> PA6      Servo ra  -> PA7         (nguon 5V rieng, chung GND)
 *   Buzzer     -> PB12     Nut vao -> PB14 ; Nut ra -> PB15 (noi GND)
 *   OLED SSD1306 I2C: SDA PB7, SCL PB6
 *   UART toi ESP32-CAM: PA9 (TX) -> ESP GPIO2 ; PA10 (RX) <- ESP GPIO1 ; GND chung
 *
 * FREERTOS (theo de cuong)
 *   GateTask    prio 3  su kien: gateSem        xu ly xe vao / ra / nut thu cong, 2 servo
 *   SensorTask  prio 2  20 ms                   chong doi IR + 2 nut
 *   DisplayTask prio 2  su kien: displayQueue   OLED + coi
 *   CommTask    prio 1  1 s                     UART voi ESP32: nhan ADD, gui STATE
 *   Dong bo: gateSem, displayQueue, slotMutex, i2cMutex, uartMutex
 *
 * LUAT: xe moi -> ho so 20.000d; moi luot gui (luc VAO) tru 5.000d; luc RA neu du < 5.000d xoa ho so;
 *       toi da 4 xe, day thi coi 1 s va khong mo barie. Nap/tru tien qua web ESP (ADD:bienso:sotien).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "stm32f1xx.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "hw.h"
#include "oled.h"

#define MAX_SLOTS        4
#define MAX_CARS         16
#define NEW_DEPOSIT      20000L
#define PARK_FEE         5000L
#define SCAN_TIMEOUT_MS  25000UL
#define SERVO_CLOSED_US  600
#define SERVO_OPEN_US    1500

#define millis() ((uint32_t)xTaskGetTickCount())

typedef struct { int used; int parked; long balance; char plate[12]; } Car;
static Car cars[MAX_CARS];                    /* luon giu slotMutex khi doc/ghi */

typedef struct {
    int idle;
    char l1[12], l2[24], l3[24];
    uint16_t holdMs;
    uint8_t beeps;
    uint16_t longMs;
} DispMsg;

static int gateOpen[2];
static volatile int pendCar[2], pendBtn[2], gateBusy[2];
static volatile uint32_t cooldownUntil[2];

static QueueHandle_t displayQueue;
static SemaphoreHandle_t gateSem, slotMutex, i2cMutex, uartMutex;
static volatile int plateReady;
static char gotPlate[16];

/* ---------- ho tro (goi khi da giu slotMutex) ---------- */
static int parkedCountLocked(void)
{
    int i, n = 0;
    for (i = 0; i < MAX_CARS; i++) if (cars[i].used && cars[i].parked) n++;
    return n;
}

/* chap nhan sai khac toi da 1 ky tu (AI doc doi khi sai 1 ky tu) */
static int findCarLocked(const char *p, int onlyParked)
{
    int i;
    size_t k, n = strlen(p);
    for (i = 0; i < MAX_CARS; i++)
        if (cars[i].used && (!onlyParked || cars[i].parked) && strcmp(cars[i].plate, p) == 0) return i;
    for (i = 0; i < MAX_CARS; i++) {
        int diff = 0;
        if (!cars[i].used || (onlyParked && !cars[i].parked) || strlen(cars[i].plate) != n) continue;
        for (k = 0; k < n; k++) if (cars[i].plate[k] != p[k]) diff++;
        if (diff <= 1) return i;
    }
    return -1;
}

static int addCarLocked(const char *p, long balance)
{
    int i;
    for (i = 0; i < MAX_CARS; i++) {
        if (!cars[i].used) {
            cars[i].used = 1; cars[i].parked = 0; cars[i].balance = balance;
            strncpy(cars[i].plate, p, sizeof(cars[i].plate) - 1);
            cars[i].plate[sizeof(cars[i].plate) - 1] = 0;
            return i;
        }
    }
    return -1;
}

/* ---------- gui thong bao cho DisplayTask ---------- */
static void showMsg(const char *l1, const char *l2, const char *l3, uint16_t holdMs, uint8_t beeps, uint16_t longMs)
{
    DispMsg m;
    memset(&m, 0, sizeof(m));
    strncpy(m.l1, l1, sizeof(m.l1) - 1);
    strncpy(m.l2, l2, sizeof(m.l2) - 1);
    strncpy(m.l3, l3, sizeof(m.l3) - 1);
    m.holdMs = holdMs; m.beeps = beeps; m.longMs = longMs;
    xQueueSend(displayQueue, &m, pdMS_TO_TICKS(200));
}

static void showIdle(void)
{
    DispMsg m;
    memset(&m, 0, sizeof(m));
    m.idle = 1;
    xQueueSend(displayQueue, &m, pdMS_TO_TICKS(200));
}

/* ---------- UART voi ESP32 ---------- */
static void sendState(void)
{
    static char buf[400];                      /* chi CommTask va GateTask goi, nhung bao ve bang uartMutex */
    int i, n;
    xSemaphoreTake(uartMutex, portMAX_DELAY);
    xSemaphoreTake(slotMutex, portMAX_DELAY);
    n = snprintf(buf, sizeof(buf), "STATE;%d;%d;", parkedCountLocked(), MAX_SLOTS);
    for (i = 0; i < MAX_CARS && n < (int)sizeof(buf) - 32; i++)
        if (cars[i].used)
            n += snprintf(buf + n, sizeof(buf) - n, "%s,%ld,%d;", cars[i].plate, cars[i].balance, cars[i].parked ? 1 : 0);
    xSemaphoreGive(slotMutex);
    uart_puts(buf);
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
            const char *p;
            long amt;
            int i;
            *c = 0;
            p = l + 4;
            amt = atol(c + 1);
            xSemaphoreTake(slotMutex, portMAX_DELAY);
            i = findCarLocked(p, 0);
            if (i < 0 && amt > 0) i = addCarLocked(p, 0);
            if (i >= 0) { cars[i].balance += amt; if (cars[i].balance < 0) cars[i].balance = 0; }
            xSemaphoreGive(slotMutex);
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
        while (!have && (c = uart_getc()) >= 0) {
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
static void requestPlate(char *out, size_t outLen)
{
    uint32_t t0;
    plateReady = 0;
    pollUart();                                    /* bo du lieu cu */
    plateReady = 0;
    xSemaphoreTake(uartMutex, portMAX_DELAY);
    uart_puts("SCAN");
    xSemaphoreGive(uartMutex);
    t0 = millis();
    while (!plateReady && millis() - t0 < SCAN_TIMEOUT_MS) {
        pollUart();
        vTaskDelay(pdMS_TO_TICKS(5));
    }
    if (plateReady) { strncpy(out, gotPlate, outLen - 1); out[outLen - 1] = 0; }
    else { strncpy(out, "NONE", outLen - 1); out[outLen - 1] = 0; }
}

/* ---------- barie ---------- */
static void setGate(int g, int open)
{
    servo_us(g, open ? SERVO_OPEN_US : SERVO_CLOSED_US);
    gateOpen[g] = open;
}

/* mo barie, doi xe di qua (IR het vat can 1,5 s) roi dong */
static void passGate(int g)
{
    uint32_t t0 = millis(), lastSeen = 0;
    int seen = 0;
    setGate(g, 1);
    while (millis() - t0 < 20000UL) {
        if (ir_active(g)) { seen = 1; lastSeen = millis(); }
        if (seen && millis() - lastSeen > 1500) break;
        vTaskDelay(pdMS_TO_TICKS(20));
    }
    setGate(g, 0);
}

/* ---------- XE VAO (cong 0) ---------- */
enum { OK_NEW, OK_OLD, ALREADY, NO_MONEY, DB_FULL };

static void handleEntry(void)
{
    char plate[16], l3[24];
    int parked, i, res = OK_OLD;
    long bal = 0;

    xSemaphoreTake(slotMutex, portMAX_DELAY);
    parked = parkedCountLocked();
    xSemaphoreGive(slotMutex);
    if (parked >= MAX_SLOTS) {
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

    xSemaphoreTake(slotMutex, portMAX_DELAY);
    i = findCarLocked(plate, 0);
    if (i >= 0 && cars[i].parked) {
        res = ALREADY;
    } else {
        int isNew = 0;
        if (i < 0) { i = addCarLocked(plate, NEW_DEPOSIT); isNew = 1; }
        if (i < 0) res = DB_FULL;
        else if (cars[i].balance < PARK_FEE) { res = NO_MONEY; bal = cars[i].balance; }
        else {
            cars[i].balance -= PARK_FEE;
            cars[i].parked = 1;
            bal = cars[i].balance;
            res = isNew ? OK_NEW : OK_OLD;
        }
    }
    xSemaphoreGive(slotMutex);

    snprintf(l3, sizeof(l3), "Con: %ldd", bal);
    switch (res) {
    case ALREADY:  showMsg("DA TRONG BAI", plate, "Xe nay da vao roi", 2500, 3, 0); cooldownUntil[0] = millis() + 3000; return;
    case DB_FULL:  showMsg("DAY BO NHO", "Het cho luu ho so", "", 2500, 3, 0);       cooldownUntil[0] = millis() + 3000; return;
    case NO_MONEY: showMsg("HET TIEN", plate, "Vui long nap them", 3000, 0, 800);    cooldownUntil[0] = millis() + 3000; return;
    default: break;
    }
    showMsg(res == OK_NEW ? "XE MOI" : "MOI VAO", plate, l3, 0, 1, 0);
    sendState();
    passGate(0);
    showIdle();
    cooldownUntil[0] = millis() + 2000;
}

/* ---------- XE RA (cong 1) ---------- */
static void handleExit(void)
{
    char plate[16], shown[12], l3[24];
    int found = 0, i;

    showMsg("DANG QUET", "Xe ra...", "", 0, 0, 0);
    requestPlate(plate, sizeof(plate));
    if (strcmp(plate, "NONE") == 0 || strlen(plate) < 5) {
        showMsg("KHONG DOC", "Khong thay bien so", "Chinh xe, thu lai", 2000, 3, 0);
        cooldownUntil[1] = millis() + 3000;
        return;
    }
    shown[0] = 0; l3[0] = 0;
    xSemaphoreTake(slotMutex, portMAX_DELAY);
    i = findCarLocked(plate, 1);
    if (i >= 0) {
        found = 1;
        strncpy(shown, cars[i].plate, sizeof(shown) - 1);
        shown[sizeof(shown) - 1] = 0;
        cars[i].parked = 0;
        if (cars[i].balance < PARK_FEE) { snprintf(l3, sizeof(l3), "Het tien, xoa HS"); cars[i].used = 0; }
        else snprintf(l3, sizeof(l3), "Con: %ldd", cars[i].balance);
    }
    xSemaphoreGive(slotMutex);
    if (!found) {
        showMsg("KHONG THAY", plate, "Xe chua vao bai", 2500, 3, 0);
        cooldownUntil[1] = millis() + 3000;
        return;
    }
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
/* GateTask (3): cho gateSem, roi xu ly xe vao / xe ra / nut thu cong */
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
                if (millis() < cooldownUntil[g] || !ir_active(g)) continue;
                gateBusy[g] = 1;
                if (g == 0) handleEntry(); else handleExit();
                pendCar[g] = 0;
                gateBusy[g] = 0;
            }
        }
    }
}

/* SensorTask (2, 20 ms): chong doi IR + nut; IR con vat can sau cooldown -> thu lai */
static void SensorTask(void *arg)
{
    uint8_t irCnt[2] = {0, 0}, btnCnt[2] = {0, 0};
    int btnHeld[2] = {0, 0};
    TickType_t last = xTaskGetTickCount();
    (void)arg;
    for (;;) {
        int g;
        for (g = 0; g < 2; g++) {
            if (ir_active(g)) { if (irCnt[g] < 255) irCnt[g]++; } else irCnt[g] = 0;
            if (irCnt[g] >= 3 && !gateBusy[g] && !pendCar[g] && millis() > cooldownUntil[g]) {
                pendCar[g] = 1;
                xSemaphoreGive(gateSem);
            }
            if (btn_pressed(g)) {
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

static void buzzMs(int ms)
{
    buzzer(true);
    vTaskDelay(pdMS_TO_TICKS(ms));
    buzzer(false);
}

static void drawIdle(void)
{
    char lines[5][24];
    char head[24];
    int rows = 0, i, r;

    xSemaphoreTake(slotMutex, portMAX_DELAY);          /* chup bang ho so, tha mutex truoc khi ve */
    snprintf(head, sizeof(head), "BAI XE  Trong: %d/%d", MAX_SLOTS - parkedCountLocked(), MAX_SLOTS);
    for (i = 0; i < MAX_CARS && rows < 5; i++)
        if (cars[i].used && cars[i].parked) {
            snprintf(lines[rows], sizeof(lines[rows]), "%s %ldk", cars[i].plate, cars[i].balance / 1000);
            rows++;
        }
    xSemaphoreGive(slotMutex);

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

/* DisplayTask (2): cho displayQueue; thong bao moi cat ngang thoi gian giu cua thong bao cu */
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

/* CommTask (1): moi 1 s doc UART (ADD tu web) va gui STATE cho ESP32 */
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

/* ---------- hook FreeRTOS ---------- */
void vApplicationMallocFailedHook(void)
{
    buzzer(true);
    for (;;) { }
}

void vApplicationStackOverflowHook(TaskHandle_t t, char *name)
{
    (void)t; (void)name;
    buzzer(true);
    for (;;) { }
}

int main(void)
{
    BaseType_t ok = pdTRUE;
    hw_init();
    servo_us(0, SERVO_CLOSED_US);
    servo_us(1, SERVO_CLOSED_US);

    displayQueue = xQueueCreate(6, sizeof(DispMsg));
    gateSem   = xSemaphoreCreateBinary();
    slotMutex = xSemaphoreCreateMutex();
    i2cMutex  = xSemaphoreCreateMutex();
    uartMutex = xSemaphoreCreateMutex();

    ok &= xTaskCreate(GateTask,    "Gate",    320, NULL, 3, NULL);
    ok &= xTaskCreate(SensorTask,  "Sensor",  128, NULL, 2, NULL);
    ok &= xTaskCreate(DisplayTask, "Display", 320, NULL, 2, NULL);
    ok &= xTaskCreate(CommTask,    "Comm",    384, NULL, 1, NULL);
    if (ok != pdTRUE || !displayQueue || !gateSem || !slotMutex || !i2cMutex || !uartMutex) {
        buzzer(true);                                  /* het heap: coi keu lien tuc */
        for (;;) { }
    }
    vTaskStartScheduler();
    for (;;) { }
}
