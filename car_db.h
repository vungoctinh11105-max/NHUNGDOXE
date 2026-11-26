/* ===== APP - ho so xe (Phu) (bien so, so du, dang gui hay khong) =====
 * Moi ham deu tu giu slotMutex ben trong -> goi tu task nao cung an toan.
 * KHONG goi tu ngat. */
#ifndef CAR_DB_H
#define CAR_DB_H

#include <stddef.h>

/* Ket qua car_db_check_in() */
enum { CHECKIN_NEW, CHECKIN_OLD, CHECKIN_ALREADY, CHECKIN_NO_MONEY, CHECKIN_DB_FULL };

int  car_db_parked_count(void);

/* Xe vao: tim/tao ho so, tru phi. *balOut = so du (sau khi tru, hoac so du hien tai neu HET TIEN) */
int  car_db_check_in(const char *plate, long *balOut);

/* Xe ra: tra 1 neu tim thay xe dang gui. shown = bien so luu trong ho so,
 * *deleted = 1 neu so du < phi va ho so bi xoa. */
int  car_db_check_out(const char *plate, char *shown, size_t shownLen, long *balOut, int *deleted);

/* Lenh ADD tu web: cong/tru tien (amt co the am), tao ho so moi neu amt > 0 */
void car_db_add_money(const char *plate, long amt);

/* Chuoi "STATE;<dang gui>;<max>;bien,du,gui;..." gui cho ESP32. Tra ve do dai. */
int  car_db_format_state(char *buf, size_t len);

/* Lay toi da maxRows dong "<bien> <du>k" cua xe dang gui (cho man hinh cho).
 * Tra ve so dong, *parked = tong so xe dang gui. */
int  car_db_parked_list(char lines[][24], int maxRows, int *parked);

#endif
