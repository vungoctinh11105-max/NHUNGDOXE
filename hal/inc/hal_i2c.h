#ifndef HAL_I2C_H
#define HAL_I2C_H

#include "hal_common.h"

hal_status_t hal_i2c_init(void);
hal_status_t hal_i2c_write(uint8_t dev_addr, uint8_t reg, const uint8_t *data, uint16_t len, uint32_t timeout_ms);
hal_status_t hal_i2c_read(uint8_t dev_addr, uint8_t reg, uint8_t *buf, uint16_t len, uint32_t timeout_ms);

#endif /* HAL_I2C_H */
