/* ===== HAL UART (Phu - feature/hal-gpio-uart) =====
 * USART1 PA9/PA10 toi ESP32-CAM. Nhan bang ngat vao bo dem vong 128 byte. */
#ifndef HAL_UART_H
#define HAL_UART_H

#include <stdint.h>

void hal_uart_init(uint32_t baudrate);   /* 8N1; voi clock 8 MHz: 115200 -> BRR = 69 */
void hal_uart_puts(const char *s);       /* gui chuoi + '\n' (cho het truyen) */
int  hal_uart_getc(void);                /* lay 1 byte da nhan, -1 neu bo dem rong */

#endif
