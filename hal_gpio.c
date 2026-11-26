#include "bsp.h"
#include "hal_gpio.h"

void hal_gpio_init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPBEN | RCC_APB2ENR_AFIOEN;

    /* IR: dau vao keo len, ngat canh xuong */
    gpio_cfg(GPIOA, 0, CFG_IN_PULL); GPIOA->BSRR = (1u << 0);
    gpio_cfg(GPIOA, 1, CFG_IN_PULL); GPIOA->BSRR = (1u << 1);
    AFIO->EXTICR[0] &= ~0xFFu;                 /* EXTI0, EXTI1 = port A */
    EXTI->IMR  |= 0x3u;
    EXTI->FTSR |= 0x3u;
    NVIC_SetPriority(EXTI0_IRQn, 8);           /* thap hon muc toi da goi API FreeRTOS (5) */
    NVIC_SetPriority(EXTI1_IRQn, 8);
    NVIC_EnableIRQ(EXTI0_IRQn);
    NVIC_EnableIRQ(EXTI1_IRQn);

    /* Nut nhan */
    gpio_cfg(GPIOB, 14, CFG_IN_PULL); GPIOB->BSRR = (1u << 14);
    gpio_cfg(GPIOB, 15, CFG_IN_PULL); GPIOB->BSRR = (1u << 15);

    /* Buzzer */
    gpio_cfg(GPIOB, 12, CFG_OUT_PP);
    GPIOB->BRR = (1u << 12);
}

bool hal_ir_active(int gate)
{
    return (GPIOA->IDR & (1u << gate)) == 0;
}

bool hal_btn_pressed(int gate)
{
    return (GPIOB->IDR & (1u << (14 + gate))) == 0;
}

void hal_buzzer(bool on)
{
    if (on) GPIOB->BSRR = (1u << 12); else GPIOB->BRR = (1u << 12);
}

/* ---- ngat ngoai IR ---- */
void EXTI0_IRQHandler(void)
{
    EXTI->PR = 1u << 0;
    ir_isr(0);
}

void EXTI1_IRQHandler(void)
{
    EXTI->PR = 1u << 1;
    ir_isr(1);
}
