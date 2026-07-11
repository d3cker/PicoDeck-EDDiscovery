#pragma once

#include <stdint.h>
#include "hardware/spi.h"

#define DISPLAY_SPI spi1

/* Waveshare Pico-Eval-Board LCD/touch pinout. */
#define PIN_LCD_DC 8
#define PIN_LCD_CS 9
#define PIN_LCD_SCK 10
#define PIN_LCD_MOSI 11
#define PIN_LCD_MISO 12
#define PIN_LCD_BL 13
#define PIN_LCD_RST 15
#define PIN_TOUCH_CS 16
#define PIN_TOUCH_IRQ 17
#define PIN_SD_CS 22
#define PIN_USER_KEY 2

#define LCD_WIDTH 480
#define LCD_HEIGHT 320
#define LCD_INIT_BAUD_HZ 5000000u
#define LCD_BAUD_HZ 15000000u

#define USB_NET_DEVICE_IP_A 192
#define USB_NET_DEVICE_IP_B 168
#define USB_NET_DEVICE_IP_C 7
#define USB_NET_DEVICE_IP_D 1
#define USB_NET_HOST_IP_D 2

#define EDDISCOVERY_PORT 6502u

#define RGB565(r, g, b) \
    ((uint16_t)((((uint16_t)(r) & 0xF8u) << 8) | \
                (((uint16_t)(g) & 0xFCu) << 3) | \
                (((uint16_t)(b) & 0xF8u) >> 3)))

#define COLOR_BLACK RGB565(0, 0, 0)
#define COLOR_ORANGE RGB565(255, 127, 0)
#define COLOR_WHITE RGB565(240, 240, 240)
#define COLOR_GREEN RGB565(42, 210, 96)
#define COLOR_RED RGB565(235, 48, 48)
#define COLOR_CYAN RGB565(40, 190, 220)
#define COLOR_DARK_GRAY RGB565(28, 30, 34)
#define COLOR_MID_GRAY RGB565(80, 84, 90)

