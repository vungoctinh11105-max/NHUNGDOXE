/* ===== APP - driver OLED SSD1306 (Nghia), dung hal_i2c ===== */
#ifndef OLED_H
#define OLED_H
void oled_init(void);                                  /* SSD1306 128x64, I2C dia chi 0x3C */
void oled_clear(void);
void oled_text(int x, int y, const char *s, int size); /* font 5x7, size 1 hoac 2 */
void oled_hline(int y);
void oled_flush(void);                                 /* day khung hinh ra man hinh */
#endif
