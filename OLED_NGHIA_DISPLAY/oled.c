#include <stdint.h>
#include <string.h>
#include "oled.h"
#include "hal_i2c.h"

extern const uint8_t font5x7[95 * 5];

static uint8_t fb[128 * 8];

static void cmd(uint8_t c)
{
    hal_i2c_start();
    hal_i2c_write(0x78);               /* 0x3C << 1 */
    hal_i2c_write(0x00);
    hal_i2c_write(c);
    hal_i2c_stop();
}

void oled_init(void)
{
    static const uint8_t seq[] = {
        0xAE, 0xD5, 0x80, 0xA8, 0x3F, 0xD3, 0x00, 0x40, 0x8D, 0x14, 0x20, 0x00,
        0xA1, 0xC8, 0xDA, 0x12, 0x81, 0xCF, 0xD9, 0xF1, 0xDB, 0x40, 0xA4, 0xA6, 0xAF
    };
    unsigned i;
    for (i = 0; i < sizeof(seq); i++) cmd(seq[i]);
    oled_clear();
    oled_flush();
}

void oled_clear(void) { memset(fb, 0, sizeof(fb)); }

static void px(int x, int y)
{
    if (x < 0 || x > 127 || y < 0 || y > 63) return;
    fb[(y >> 3) * 128 + x] |= (uint8_t)(1u << (y & 7));
}

void oled_text(int x, int y, const char *s, int size)
{
    while (*s) {
        unsigned char ch = (unsigned char)*s++;
        if (ch < 32 || ch > 126) ch = '?';
        const uint8_t *g = &font5x7[(ch - 32) * 5];
        int col, row, dx, dy;
        for (col = 0; col < 5; col++)
            for (row = 0; row < 7; row++)
                if (g[col] & (1u << row))
                    for (dx = 0; dx < size; dx++)
                        for (dy = 0; dy < size; dy++)
                            px(x + col * size + dx, y + row * size + dy);
        x += 6 * size;
    }
}

void oled_hline(int y)
{
    int x;
    for (x = 0; x < 128; x++) px(x, y);
}

void oled_flush(void)
{
    int i;
    cmd(0x21); cmd(0); cmd(127);
    cmd(0x22); cmd(0); cmd(7);
    hal_i2c_start();
    hal_i2c_write(0x78);
    hal_i2c_write(0x40);
    for (i = 0; i < 128 * 8; i++) hal_i2c_write(fb[i]);
    hal_i2c_stop();
}
