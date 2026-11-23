#ifndef HAL_UART_H
#define HAL_UART_H

#include "hal_common.h"

hal_status_t hal_uart_init(uint32_t baudrate);
hal_status_t hal_uart_send(const uint8_t *data, uint16_t len, uint32_t timeout_ms);
hal_status_t hal_uart_receive(uint8_t *buf, uint16_t len, uint32_t timeout_ms);

#endif /* HAL_UART_H */
