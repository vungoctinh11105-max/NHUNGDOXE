#include "bsp.h"
#include "hal_uart.h"

#define RX_SIZE 128
static volatile uint8_t rx_buf[RX_SIZE];          /* bo dem vong: ngat ghi o head, task doc o tail */
static volatile uint16_t rx_head = 0, rx_tail = 0;

void hal_uart_init(uint32_t baudrate)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_USART1EN;

    gpio_cfg(GPIOA, 9, CFG_AF_PP);                 /* TX */
    gpio_cfg(GPIOA, 10, CFG_IN_FLOAT);             /* RX */
    USART1->BRR = 8000000UL / baudrate;            /* 115200 -> 69 */
    USART1->CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE | USART_CR1_RXNEIE;
    NVIC_SetPriority(USART1_IRQn, 9);
    NVIC_EnableIRQ(USART1_IRQn);
}

static void uart_putc(char c)
{
    while ((USART1->SR & USART_SR_TXE) == 0) { }
    USART1->DR = (uint8_t)c;
}

void hal_uart_puts(const char *s)
{
    while (*s) uart_putc(*s++);
    uart_putc('\n');
    while ((USART1->SR & USART_SR_TC) == 0) { }
}

int hal_uart_getc(void)
{
    uint8_t c;
    if (rx_head == rx_tail) return -1;
    c = rx_buf[rx_tail];
    rx_tail = (uint16_t)((rx_tail + 1) % RX_SIZE);
    return c;
}

/* ---- ngat nhan ---- */
void USART1_IRQHandler(void)
{
    uint32_t sr = USART1->SR;
    if (sr & (USART_SR_RXNE | USART_SR_ORE)) {
        uint8_t c = (uint8_t)USART1->DR;       /* doc DR xoa co RXNE/ORE */
        uint16_t next = (uint16_t)((rx_head + 1) % RX_SIZE);
        if (next != rx_tail) { rx_buf[rx_head] = c; rx_head = next; }
    }
}
