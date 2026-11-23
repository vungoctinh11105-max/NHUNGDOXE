#ifndef HAL_COMMON_H
#define HAL_COMMON_H

#include <stdint.h>

/* Ma trang thai tra ve chung cho moi ham HAL */
typedef enum {
    HAL_OK = 0,
    HAL_ERROR,
    HAL_BUSY,
    HAL_TIMEOUT
} hal_status_t;

/* Quy uoc:
 * - Moi ham co the cho (blocking) PHAI co tham so timeout_ms.
 * - Ham dung chung tu nhieu task phai bao ve bang Mutex FreeRTOS.
 * - Callback goi tu ngat chi duoc dung API ...FromISR().
 */

#endif /* HAL_COMMON_H */
