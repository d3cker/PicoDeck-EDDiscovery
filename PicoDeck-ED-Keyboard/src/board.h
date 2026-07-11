#pragma once

#include <stdint.h>
#include "hardware/spi.h"

#define PICODECK_SPI spi1

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

#define LCD_WIDTH 480
#define LCD_HEIGHT 320
#define LCD_INIT_BAUD_HZ 4000000u
#define LCD_BAUD_HZ 15000000u
#define TOUCH_BAUD_HZ 3000000u

#define UI_COLUMNS 6
#define UI_ROWS 4
#define UI_TILE_WIDTH (LCD_WIDTH / UI_COLUMNS)
#define UI_TILE_HEIGHT (LCD_HEIGHT / UI_ROWS)

#define RGB565(r, g, b) \
    ((uint16_t)((((uint16_t)(r) & 0xF8u) << 8) | \
                (((uint16_t)(g) & 0xFCu) << 3) | \
                (((uint16_t)(b) & 0xF8u) >> 3)))

#define COLOR_BLACK RGB565(0, 0, 0)
#define COLOR_ORANGE RGB565(255, 127, 0)
#define COLOR_WHITE RGB565(240, 240, 240)
#define COLOR_DARK_GRAY RGB565(32, 32, 32)

/*
 * Default XPT2046 calibration for Waveshare Pico-ResTouch-LCD-3.5 in
 * 480x320 landscape mode. These are the proven U2D_R2L base coefficients;
 * touch.c flips both logical axes to match the cable-at-top display rotation.
 * Values are Q16.16 multipliers.
 */
#define TOUCH_X_FROM_RAW_Y_Q16 (-8709)
#define TOUCH_X_OFFSET 517
#define TOUCH_Y_FROM_RAW_X_Q16 5765
#define TOUCH_Y_OFFSET (-20)

/* Separate point-to-point subnet from the telemetry display (192.168.7.x). */
#define USB_NET_DEVICE_IP_A 192
#define USB_NET_DEVICE_IP_B 168
#define USB_NET_DEVICE_IP_C 8
#define USB_NET_DEVICE_IP_D 1
#define USB_NET_HOST_IP_D 2

#define EDDISCOVERY_PORT 6502u

#define COLOR_GREEN RGB565(42, 210, 96)
#define COLOR_RED RGB565(235, 48, 48)
#define COLOR_MID_GRAY RGB565(80, 84, 90)
