#ifndef SSD1306_H
#define SSD1306_H

#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "i2ctest.h"

#define OLED_I2C_ADDRESS 0x3C
#define OLED_WIDTH 128
#define OLED_HEIGHT 64
#define OLED_PAGES (OLED_HEIGHT / 8)
#define FONT_SIZE 5

void oled_init(i2c_master_bus_handle_t *bus_handle, bool isSSD1306);
void oled_set_cursor(uint8_t page, uint8_t column);
void oled_print_char(uint8_t page, uint8_t column, char c);
void oled_print(uint8_t page, uint8_t column, const char *str);
void oled_printf(uint8_t page, uint8_t column, const char *fmt, ...);
void oled_clear(void);
void oled_draw_bitmap(uint8_t x, uint8_t y, const uint8_t *bitmap, uint8_t width, uint8_t height);
void oled_print_char_xy(uint8_t x, uint8_t y, char c);
void oled_print_xy(uint8_t x, uint8_t y, const char *str);
void oled_printf_xy(uint8_t x, uint8_t y, const char *fmt, ...);
void oled_draw_rectangle(uint8_t x, uint8_t y, uint8_t width, uint8_t height);

#endif