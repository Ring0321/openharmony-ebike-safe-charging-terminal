

#ifndef _LCD_H_
#define _LCD_H_

#include <stdint.h>

#define USE_HORIZONTAL      3

#if ((USE_HORIZONTAL==0) || (USE_HORIZONTAL==1))
#define LCD_W 240
#define LCD_H 320
#else
#define LCD_W 320
#define LCD_H 240
#endif

#define LCD_WHITE           0xFFFF
#define LCD_BLACK           0x0000
#define LCD_BLUE            0x001F
#define LCD_BRED            0XF81F
#define LCD_GRED            0XFFE0
#define LCD_GBLUE           0X07FF
#define LCD_RED             0xF800
#define LCD_MAGENTA         0xF81F
#define LCD_GREEN           0x07E0
#define LCD_CYAN            0x7FFF
#define LCD_YELLOW          0xFFE0
#define LCD_BROWN           0XBC40
#define LCD_BRRED           0XFC07
#define LCD_GRAY            0X8430
#define LCD_DARKBLUE        0X01CF
#define LCD_LIGHTBLUE       0X7D7C
#define LCD_GRAYBLUE        0X5458
#define LCD_LIGHTGREEN      0X841F
#define LCD_LGRAY           0XC618
#define LCD_LGRAYBLUE       0XA651
#define LCD_LBBLUE          0X2B12

unsigned int lcd_init();

unsigned int lcd_deinit();

void lcd_fill(uint16_t xsta, uint16_t ysta, uint16_t xend, uint16_t yend, uint16_t color);

void lcd_draw_point(uint16_t x, uint16_t y, uint16_t color);

void lcd_draw_line(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);

void lcd_draw_rectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);

void lcd_draw_circle(uint16_t x0, uint16_t y0, uint8_t r, uint16_t color);

void lcd_show_chinese(uint16_t x, uint16_t y, uint8_t *s, uint16_t fc, uint16_t bc, uint8_t sizey, uint8_t mode);

void lcd_show_char(uint16_t x, uint16_t y, uint8_t num, uint16_t fc, uint16_t bc, uint8_t sizey, uint8_t mode);

void lcd_show_string(uint16_t x, uint16_t y, const uint8_t *p, uint16_t fc, uint16_t bc, uint8_t sizey, uint8_t mode);

void lcd_show_int_num(uint16_t x, uint16_t y, uint16_t num, uint8_t len, uint16_t fc, uint16_t bc, uint8_t sizey);

void lcd_show_float_num1(uint16_t x, uint16_t y, float num, uint8_t len, uint16_t fc, uint16_t bc, uint8_t sizey);

void lcd_show_picture(uint16_t x, uint16_t y, uint16_t length, uint16_t width, const uint8_t *pic);

#endif
