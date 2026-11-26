#include "bsp.h"
#include "hal_timer.h"

void hal_timer_pwm_init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;

    /* Servo: TIM3 CH1/CH2, 8 MHz / 8 = 1 MHz, chu ky 20 ms */
    gpio_cfg(GPIOA, 6, CFG_AF_PP);
    gpio_cfg(GPIOA, 7, CFG_AF_PP);
    TIM3->PSC = 7;
    TIM3->ARR = 19999;
    TIM3->CCMR1 = (6u << 4) | TIM_CCMR1_OC1PE | (6u << 12) | TIM_CCMR1_OC2PE;  /* PWM mode 1 + preload */
    TIM3->CCR1 = 600;
    TIM3->CCR2 = 600;
    TIM3->CCER = TIM_CCER_CC1E | TIM_CCER_CC2E;
    TIM3->CR1 = TIM_CR1_ARPE | TIM_CR1_CEN;
    TIM3->EGR = TIM_EGR_UG;
}

void hal_servo_us(int gate, int pulse_us)
{
    if (gate == 0) TIM3->CCR1 = (uint32_t)pulse_us; else TIM3->CCR2 = (uint32_t)pulse_us;
}
