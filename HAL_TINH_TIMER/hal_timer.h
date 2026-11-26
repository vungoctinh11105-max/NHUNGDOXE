/* ===== HAL TIMER (Tinh - feature/hal-timer-adc) =====
 * TIM3 phat PWM 50 Hz tren CH1 (PA6, servo cong vao) va CH2 (PA7, servo cong ra).
 * 8 MHz / (PSC+1 = 8) = 1 MHz -> 1 tick = 1 us ; ARR = 19999 -> chu ky 20 ms. */
#ifndef HAL_TIMER_H
#define HAL_TIMER_H

void hal_timer_pwm_init(void);              /* bat TIM3, 2 kenh PWM, xung ban dau 600 us */
void hal_servo_us(int gate, int pulse_us);  /* gate 0 = vao (CH1), 1 = ra (CH2) */

#endif
