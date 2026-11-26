/* ===== HAL I2C (Nghia - feature/hal-i2c-spi) =====
 * I2C "mem" (bit-bang bang GPIO) tren SCL PB6, SDA PB7 - dung cho OLED SSD1306.
 * Ghi 1 chieu, bo qua ACK. Bao ve bang i2cMutex o tang app neu nhieu task dung. */
#ifndef HAL_I2C_H
#define HAL_I2C_H

#include <stdint.h>

void hal_i2c_init(void);           /* PB6/PB7 ra mo-xa, nha bus (muc cao) */
void hal_i2c_start(void);
void hal_i2c_stop(void);
void hal_i2c_write(uint8_t b);     /* 8 bit MSB truoc + 1 xung ACK */

#endif
