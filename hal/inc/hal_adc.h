#ifndef HAL_ADC_H
#define HAL_ADC_H

#include "hal_common.h"

hal_status_t hal_adc_init(void);
hal_status_t hal_adc_read(uint8_t channel, uint16_t *value, uint32_t timeout_ms);

#endif /* HAL_ADC_H */
