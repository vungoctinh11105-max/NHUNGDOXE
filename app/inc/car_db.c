#include <stdio.h>
#include <string.h>
#include "app_cfg.h"
#include "app_ipc.h"
#include "car_db.h"

typedef struct { int used; int parked; long balance; char plate[12]; } Car;
static Car cars[MAX_CARS];                    /* luon giu slotMutex khi doc/ghi */

/* ---------- ham noi bo (goi khi da giu slotMutex) ---------- */
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

/* ---------- API ---------- */
int car_db_parked_count(void)
{
    int n;
    xSemaphoreTake(slotMutex, portMAX_DELAY);
    n = parkedCountLocked();
    xSemaphoreGive(slotMutex);
    return n;
}

int car_db_check_in(const char *plate, long *balOut)
{
    int i, res = CHECKIN_OLD;

    xSemaphoreTake(slotMutex, portMAX_DELAY);
    i = findCarLocked(plate, 0);
    if (i >= 0 && cars[i].parked) {
        res = CHECKIN_ALREADY;
    } else {
        int isNew = 0;
        if (i < 0) { i = addCarLocked(plate, NEW_DEPOSIT); isNew = 1; }
        if (i < 0) res = CHECKIN_DB_FULL;
        else if (cars[i].balance < PARK_FEE) { res = CHECKIN_NO_MONEY; *balOut = cars[i].balance; }
        else {
            cars[i].balance -= PARK_FEE;
            cars[i].parked = 1;
            *balOut = cars[i].balance;
            res = isNew ? CHECKIN_NEW : CHECKIN_OLD;
        }
    }
    xSemaphoreGive(slotMutex);
    return res;
}

int car_db_check_out(const char *plate, char *shown, size_t shownLen, long *balOut, int *deleted)
{
    int i, found = 0;

    xSemaphoreTake(slotMutex, portMAX_DELAY);
    i = findCarLocked(plate, 1);
    if (i >= 0) {
        found = 1;
        strncpy(shown, cars[i].plate, shownLen - 1);
        shown[shownLen - 1] = 0;
        cars[i].parked = 0;
        *balOut = cars[i].balance;
        if (cars[i].balance < PARK_FEE) { cars[i].used = 0; *deleted = 1; }
        else *deleted = 0;
    }
    xSemaphoreGive(slotMutex);
    return found;
}

void car_db_add_money(const char *plate, long amt)
{
    int i;
    xSemaphoreTake(slotMutex, portMAX_DELAY);
    i = findCarLocked(plate, 0);
    if (i < 0 && amt > 0) i = addCarLocked(plate, 0);
    if (i >= 0) { cars[i].balance += amt; if (cars[i].balance < 0) cars[i].balance = 0; }
    xSemaphoreGive(slotMutex);
}

int car_db_format_state(char *buf, size_t len)
{
    int i, n;
    xSemaphoreTake(slotMutex, portMAX_DELAY);
    n = snprintf(buf, len, "STATE;%d;%d;", parkedCountLocked(), MAX_SLOTS);
    for (i = 0; i < MAX_CARS && n < (int)len - 32; i++)
        if (cars[i].used)
            n += snprintf(buf + n, len - n, "%s,%ld,%d;", cars[i].plate, cars[i].balance, cars[i].parked ? 1 : 0);
    xSemaphoreGive(slotMutex);
    return n;
}

int car_db_parked_list(char lines[][24], int maxRows, int *parked)
{
    int i, rows = 0;
    xSemaphoreTake(slotMutex, portMAX_DELAY);
    *parked = parkedCountLocked();
    for (i = 0; i < MAX_CARS && rows < maxRows; i++)
        if (cars[i].used && cars[i].parked) {
            snprintf(lines[rows], 24, "%s %ldk", cars[i].plate, cars[i].balance / 1000);
            rows++;
        }
    xSemaphoreGive(slotMutex);
    return rows;
}
