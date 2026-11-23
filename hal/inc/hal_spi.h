#ifndef HAL_SPI_H
#define HAL_SPI_H

#include "hal_common.h"

hal_status_t hal_spi_init(void);
hal_status_t hal_spi_transfer(const uint8_t *tx, uint8_t *rx, uint16_t len, uint32_t timeout_ms);

#endif /* HAL_SPI_H */
