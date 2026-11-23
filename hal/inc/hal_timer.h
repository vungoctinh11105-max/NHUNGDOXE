#ifndef HAL_TIMER_H
#define HAL_TIMER_H

#include "hal_common.h"

hal_status_t hal_timer_init(uint32_t period_us);
hal_status_t hal_timer_start(void);
hal_status_t hal_timer_stop(void);
hal_status_t hal_pwm_set_duty(uint8_t channel, uint8_t duty_percent);

#endif /* HAL_TIMER_H */
