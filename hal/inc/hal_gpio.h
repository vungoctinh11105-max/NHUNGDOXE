#ifndef HAL_GPIO_H
#define HAL_GPIO_H

#include "hal_common.h"

hal_status_t hal_gpio_init(void);
hal_status_t hal_gpio_write(uint8_t port, uint8_t pin, uint8_t level);
uint8_t      hal_gpio_read(uint8_t port, uint8_t pin);
hal_status_t hal_gpio_toggle(uint8_t port, uint8_t pin);

#endif /* HAL_GPIO_H */
