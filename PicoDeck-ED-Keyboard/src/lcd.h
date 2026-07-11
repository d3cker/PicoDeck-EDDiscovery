#pragma once

#include <stddef.h>
#include <stdint.h>

void lcd_init(void);
void lcd_set_bus_speed(void);
void lcd_fill(uint16_t color);
void lcd_blit_rgb565_be(uint16_t x, uint16_t y, uint16_t width,
                        uint16_t height, const uint8_t *pixels,
                        size_t byte_count);

