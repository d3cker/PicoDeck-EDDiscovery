#include "lcd.h"

#include <string.h>

#include "board.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "pico/stdlib.h"

static uint32_t s_lcd_baud_hz = LCD_INIT_BAUD_HZ;

static void lcd_select(bool selected) {
    gpio_put(PIN_LCD_CS, selected ? 0 : 1);
}

void lcd_set_bus_speed(void) {
    spi_set_baudrate(DISPLAY_SPI, s_lcd_baud_hz);
}

static void lcd_write_command(uint8_t command, const uint8_t *data,
                              size_t length) {
    lcd_set_bus_speed();
    lcd_select(true);
    gpio_put(PIN_LCD_DC, 0);
    spi_write_blocking(DISPLAY_SPI, &command, 1);
    lcd_select(false);

    /* Match Waveshare's LCD_WriteData(): one 16-bit transfer per parameter. */
    for (size_t i = 0; i < length; ++i) {
        const uint8_t physical_word[2] = {0x00, data[i]};
        lcd_select(true);
        gpio_put(PIN_LCD_DC, 1);
        spi_write_blocking(DISPLAY_SPI, physical_word, sizeof(physical_word));
        lcd_select(false);
    }
}

static uint8_t lcd_read_id(void) {
    const uint8_t command = 0xDC;
    const uint8_t dummy = 0x00;
    uint8_t response = 0x00;

    lcd_set_bus_speed();
    lcd_select(true);
    gpio_put(PIN_LCD_DC, 0);
    spi_write_blocking(DISPLAY_SPI, &command, 1);
    spi_write_read_blocking(DISPLAY_SPI, &dummy, &response, 1);
    lcd_select(false);
    return response;
}

static void lcd_set_window(uint16_t x, uint16_t y, uint16_t width,
                           uint16_t height) {
    const uint16_t x_end = (uint16_t)(x + width - 1);
    const uint16_t y_end = (uint16_t)(y + height - 1);
    const uint8_t columns[] = {
        (uint8_t)(x >> 8), (uint8_t)x,
        (uint8_t)(x_end >> 8), (uint8_t)x_end,
    };
    const uint8_t rows[] = {
        (uint8_t)(y >> 8), (uint8_t)y,
        (uint8_t)(y_end >> 8), (uint8_t)y_end,
    };

    lcd_write_command(0x2A, columns, sizeof(columns));
    lcd_write_command(0x2B, rows, sizeof(rows));
    lcd_write_command(0x2C, NULL, 0);
}

void lcd_init(void) {
    gpio_init(PIN_LCD_DC);
    gpio_set_dir(PIN_LCD_DC, GPIO_OUT);
    gpio_init(PIN_LCD_CS);
    gpio_set_dir(PIN_LCD_CS, GPIO_OUT);
    gpio_init(PIN_LCD_RST);
    gpio_set_dir(PIN_LCD_RST, GPIO_OUT);
    gpio_init(PIN_SD_CS);
    gpio_set_dir(PIN_SD_CS, GPIO_OUT);
    gpio_init(PIN_TOUCH_CS);
    gpio_set_dir(PIN_TOUCH_CS, GPIO_OUT);
    gpio_init(PIN_TOUCH_IRQ);
    gpio_set_dir(PIN_TOUCH_IRQ, GPIO_IN);
    gpio_pull_up(PIN_TOUCH_IRQ);

    gpio_put(PIN_LCD_CS, 1);
    gpio_put(PIN_TOUCH_CS, 1);
    gpio_put(PIN_SD_CS, 1);
    gpio_set_function(PIN_LCD_BL, GPIO_FUNC_PWM);
    const uint slice = pwm_gpio_to_slice_num(PIN_LCD_BL);
    pwm_set_wrap(slice, 10000);
    pwm_set_enabled(slice, true);
    lcd_set_backlight(0);

    s_lcd_baud_hz = LCD_INIT_BAUD_HZ;
    spi_init(DISPLAY_SPI, LCD_INIT_BAUD_HZ);
    gpio_set_function(PIN_LCD_SCK, GPIO_FUNC_SPI);
    gpio_set_function(PIN_LCD_MOSI, GPIO_FUNC_SPI);
    gpio_set_function(PIN_LCD_MISO, GPIO_FUNC_SPI);

    gpio_put(PIN_LCD_RST, 1);
    sleep_ms(500);
    gpio_put(PIN_LCD_RST, 0);
    sleep_ms(500);
    gpio_put(PIN_LCD_RST, 1);
    sleep_ms(500);

    /* Waveshare performs this read before choosing the 3.5-inch init path. */
    (void)lcd_read_id();

    lcd_write_command(0x21, NULL, 0); /* Display inversion on. */

    const uint8_t power2[] = {0x33};
    lcd_write_command(0xC2, power2, sizeof(power2));

    const uint8_t vcom[] = {0x00, 0x1E, 0x80};
    lcd_write_command(0xC5, vcom, sizeof(vcom));

    const uint8_t frame_rate[] = {0xB0};
    lcd_write_command(0xB1, frame_rate, sizeof(frame_rate));

    const uint8_t gamma_positive[] = {
        0x00, 0x13, 0x18, 0x04, 0x0F, 0x06, 0x3A, 0x56,
        0x4D, 0x03, 0x0A, 0x06, 0x30, 0x3E, 0x0F,
    };
    lcd_write_command(0xE0, gamma_positive, sizeof(gamma_positive));

    const uint8_t gamma_negative[] = {
        0x00, 0x13, 0x18, 0x01, 0x11, 0x06, 0x38, 0x34,
        0x4D, 0x06, 0x0D, 0x0B, 0x31, 0x37, 0x0F,
    };
    lcd_write_command(0xE1, gamma_negative, sizeof(gamma_negative));

    const uint8_t pixel_format[] = {0x55}; /* RGB565. */
    lcd_write_command(0x3A, pixel_format, sizeof(pixel_format));

    lcd_write_command(0x11, NULL, 0); /* Sleep out. */
    sleep_ms(120);
    lcd_write_command(0x29, NULL, 0); /* Display on. */

    /*
     * Waveshare D2U_L2R landscape orientation: 480 by 320.
     */
    const uint8_t display_function[] = {0x00, 0x62};
    lcd_write_command(0xB6, display_function, sizeof(display_function));
    const uint8_t memory_access[] = {0x28};
    lcd_write_command(0x36, memory_access, sizeof(memory_access));
    sleep_ms(200);

    s_lcd_baud_hz = LCD_BAUD_HZ;
    lcd_set_bus_speed();
    lcd_set_backlight(1000);
}

void lcd_set_backlight(uint16_t permille) {
    if (permille > 1000) permille = 1000;
    pwm_set_gpio_level(PIN_LCD_BL, (uint16_t)(permille * 10u));
}

void lcd_blit_rgb565_be(uint16_t x, uint16_t y, uint16_t width,
                        uint16_t height, const uint8_t *pixels,
                        size_t byte_count) {
    if (width == 0 || height == 0 || pixels == NULL) {
        return;
    }

    lcd_set_window(x, y, width, height);
    lcd_set_bus_speed();
    lcd_select(true);
    gpio_put(PIN_LCD_DC, 1);
    spi_write_blocking(DISPLAY_SPI, pixels, byte_count);
    lcd_select(false);
}

void lcd_fill(uint16_t color) {
    uint8_t row[LCD_WIDTH * 2];
    for (size_t i = 0; i < LCD_WIDTH; ++i) {
        row[i * 2] = (uint8_t)(color >> 8);
        row[i * 2 + 1] = (uint8_t)color;
    }

    lcd_set_window(0, 0, LCD_WIDTH, LCD_HEIGHT);
    lcd_set_bus_speed();
    lcd_select(true);
    gpio_put(PIN_LCD_DC, 1);
    for (size_t y = 0; y < LCD_HEIGHT; ++y) {
        spi_write_blocking(DISPLAY_SPI, row, sizeof(row));
    }
    lcd_select(false);
}
