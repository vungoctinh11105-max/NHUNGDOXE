#include "bsp.h"
#include "hal_i2c.h"

#define SCL_H() (GPIOB->BSRR = (1u << 6))
#define SCL_L() (GPIOB->BRR  = (1u << 6))
#define SDA_H() (GPIOB->BSRR = (1u << 7))
#define SDA_L() (GPIOB->BRR  = (1u << 7))

void hal_i2c_init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;

    /* I2C mem: SDA PB7, SCL PB6 mo-xa, muc cao */
    gpio_cfg(GPIOB, 6, CFG_OUT_OD);
    gpio_cfg(GPIOB, 7, CFG_OUT_OD);
    GPIOB->BSRR = (1u << 6) | (1u << 7);
}

static void i2c_delay(void)
{
    volatile int i;
    for (i = 0; i < 4; i++) { }
}

/* START: SDA xuong trong khi SCL cao */
void hal_i2c_start(void) { SDA_H(); SCL_H(); i2c_delay(); SDA_L(); i2c_delay(); SCL_L(); }

/* STOP: SDA len trong khi SCL cao */
void hal_i2c_stop(void)  { SDA_L(); SCL_H(); i2c_delay(); SDA_H(); i2c_delay(); }

void hal_i2c_write(uint8_t b)
{
    int i;
    for (i = 0; i < 8; i++) {
        if (b & 0x80) SDA_H(); else SDA_L();
        i2c_delay();
        SCL_H(); i2c_delay(); SCL_L();
        b <<= 1;
    }
    SDA_H();                       /* xung ACK, bo qua gia tri */
    i2c_delay(); SCL_H(); i2c_delay(); SCL_L();
}
