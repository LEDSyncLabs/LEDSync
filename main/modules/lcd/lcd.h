#ifndef LCD_H
#define LCD_H

#include <stdbool.h>
#include <stdint.h>
#include "colors.h"
#include "commands.h"
#include "font.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define LCD_HOST SPI2_HOST

#define PIN_NUM_MISO GPIO_NUM_14
#define PIN_NUM_MOSI GPIO_NUM_26
#define PIN_NUM_CLK GPIO_NUM_27
#define PIN_NUM_CS GPIO_NUM_32

#define PIN_NUM_DC GPIO_NUM_25
#define PIN_NUM_RST GPIO_NUM_33

#define PIN_NUM_BCKL GPIO_NUM_21

#define DISPLAY_REAL_WIDTH 320
#define DISPLAY_REAL_HEIGHT 240
#define DISPLAY_SCALE 2

#define DISPLAY_WIDTH DISPLAY_REAL_WIDTH / DISPLAY_SCALE
#define DISPLAY_HEIGHT DISPLAY_REAL_HEIGHT / DISPLAY_SCALE

// To speed up transfers, every SPI transfer sends a bunch of lines. This define
// specifies how many. More means more memory use, but less overhead for setting
// up / finishing transfers. Make sure 240 is dividable by this.
#define PARALLEL_LINES 16

// Function prototypes
void lcd_init(void);
void lcd_cmd(const uint8_t cmd, bool keep_cs_active);
void lcd_data(const uint8_t *data, int len);
void lcd_display_clear(uint16_t color);
void lcd_display_update(void);
void lcd_pixel_put(int x, int y, uint16_t color);
void lcd_draw_char(int x, int y, char c, uint16_t color);
void lcd_draw_text(int x, int y, char *text, uint16_t color);
void lcd_invert_set(bool invert);
void lcd_draw_rect_outline(int x, int y, int w, int h, uint16_t color);
void lcd_draw_rect_filled(int x, int y, int w, int h, uint16_t color);
void lcd_draw_line(int x0, int y0, int x1, int y1, uint16_t color);
void lcd_draw_circle(int cx, int cy, int r, uint16_t color);
void lcd_draw_disk(int cx, int cy, int r, uint16_t color);
void lcd_demo(void);

#endif // LCD_H
