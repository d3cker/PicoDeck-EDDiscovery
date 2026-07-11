#include "touch.h"

#include <stdlib.h>

#include "board.h"
#include "hardware/gpio.h"
#include "pico/stdlib.h"

#define TOUCH_SAMPLES 5
#define TOUCH_DISCARD_SAMPLES 1
#define TOUCH_PAIR_MAX_DELTA 50

static uint16_t touch_read_adc(uint8_t command) {
    uint8_t response[2] = {0, 0};
    uint8_t zeroes[2] = {0, 0};

    gpio_put(PIN_TOUCH_CS, 0);
    spi_write_blocking(PICODECK_SPI, &command, 1);
    sleep_us(5);
    spi_write_read_blocking(PICODECK_SPI, zeroes, response, 2);
    gpio_put(PIN_TOUCH_CS, 1);

    return (uint16_t)((((uint16_t)response[0] << 8) | response[1]) >> 3);
}

static void sort_samples(uint16_t *values) {
    for (size_t i = 1; i < TOUCH_SAMPLES; ++i) {
        const uint16_t value = values[i];
        size_t j = i;
        while (j > 0 && values[j - 1] > value) {
            values[j] = values[j - 1];
            --j;
        }
        values[j] = value;
    }
}

static uint16_t touch_read_filtered(uint8_t command) {
    uint16_t values[TOUCH_SAMPLES];
    for (size_t i = 0; i < TOUCH_SAMPLES; ++i) {
        values[i] = touch_read_adc(command);
        sleep_us(200);
    }
    sort_samples(values);

    uint32_t sum = 0;
    for (size_t i = TOUCH_DISCARD_SAMPLES;
         i < TOUCH_SAMPLES - TOUCH_DISCARD_SAMPLES; ++i) {
        sum += values[i];
    }
    return (uint16_t)(sum /
        (TOUCH_SAMPLES - 2 * TOUCH_DISCARD_SAMPLES));
}

static bool touch_read_stable_pair(uint16_t *raw_x, uint16_t *raw_y) {
    const uint16_t x1 = touch_read_filtered(0xD0);
    const uint16_t y1 = touch_read_filtered(0x90);
    sleep_us(10);
    const uint16_t x2 = touch_read_filtered(0xD0);
    const uint16_t y2 = touch_read_filtered(0x90);

    if (abs((int)x1 - (int)x2) > TOUCH_PAIR_MAX_DELTA ||
        abs((int)y1 - (int)y2) > TOUCH_PAIR_MAX_DELTA) {
        return false;
    }

    *raw_x = (uint16_t)((x1 + x2) / 2u);
    *raw_y = (uint16_t)((y1 + y2) / 2u);
    return true;
}

void touch_init(void) {
    gpio_init(PIN_TOUCH_CS);
    gpio_set_dir(PIN_TOUCH_CS, GPIO_OUT);
    gpio_put(PIN_TOUCH_CS, 1);

    gpio_init(PIN_TOUCH_IRQ);
    gpio_set_dir(PIN_TOUCH_IRQ, GPIO_IN);
    gpio_pull_up(PIN_TOUCH_IRQ);
}

bool touch_poll(uint16_t *x, uint16_t *y) {
    if (gpio_get(PIN_TOUCH_IRQ) != 0) {
        return false;
    }

    spi_set_baudrate(PICODECK_SPI, TOUCH_BAUD_HZ);
    uint16_t raw_x = 0;
    uint16_t raw_y = 0;
    const bool stable = touch_read_stable_pair(&raw_x, &raw_y);
    spi_set_baudrate(PICODECK_SPI, LCD_BAUD_HZ);

    if (!stable || gpio_get(PIN_TOUCH_IRQ) != 0) {
        return false;
    }

    if (raw_x < 100 || raw_x > 4000 || raw_y < 100 || raw_y > 4000) {
        return false;
    }

    int32_t mapped_x = TOUCH_X_OFFSET +
        (((int32_t)TOUCH_X_FROM_RAW_Y_Q16 * raw_y) >> 16);
    int32_t mapped_y = TOUCH_Y_OFFSET +
        (((int32_t)TOUCH_Y_FROM_RAW_X_Q16 * raw_x) >> 16);

    if (mapped_x < 0) mapped_x = 0;
    if (mapped_x >= LCD_WIDTH) mapped_x = LCD_WIDTH - 1;
    if (mapped_y < 0) mapped_y = 0;
    if (mapped_y >= LCD_HEIGHT) mapped_y = LCD_HEIGHT - 1;

    /* Match the display's 180-degree cable-at-top orientation. */
    mapped_x = (LCD_WIDTH - 1) - mapped_x;
    mapped_y = (LCD_HEIGHT - 1) - mapped_y;

    *x = (uint16_t)mapped_x;
    *y = (uint16_t)mapped_y;
    return true;
}
