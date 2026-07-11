#include "ui.h"

#include <string.h>

#include "board.h"
#include "buttons.h"
#include "font5x7.h"
#include "lcd.h"

static uint8_t tile_buffer[UI_TILE_WIDTH * UI_TILE_HEIGHT * 2];

#define SETUP_STRIPE_HEIGHT 12
static int setup_stripe_y;

static void set_pixel(int x, int y, uint16_t color) {
    if (x < 0 || x >= UI_TILE_WIDTH || y < 0 || y >= UI_TILE_HEIGHT) {
        return;
    }
    const size_t offset = ((size_t)y * UI_TILE_WIDTH + (size_t)x) * 2;
    tile_buffer[offset] = (uint8_t)(color >> 8);
    tile_buffer[offset + 1] = (uint8_t)color;
}

static void clear_tile(uint16_t color) {
    for (size_t i = 0; i < UI_TILE_WIDTH * UI_TILE_HEIGHT; ++i) {
        tile_buffer[i * 2] = (uint8_t)(color >> 8);
        tile_buffer[i * 2 + 1] = (uint8_t)color;
    }
}

static void draw_border(uint16_t color) {
    for (int inset = 1; inset <= 2; ++inset) {
        for (int x = inset; x < UI_TILE_WIDTH - inset; ++x) {
            set_pixel(x, inset, color);
            set_pixel(x, UI_TILE_HEIGHT - 1 - inset, color);
        }
        for (int y = inset; y < UI_TILE_HEIGHT - inset; ++y) {
            set_pixel(inset, y, color);
            set_pixel(UI_TILE_WIDTH - 1 - inset, y, color);
        }
    }
}

static uint16_t color_with_coverage(uint16_t color, uint8_t coverage) {
    if (coverage >= 3) {
        return color;
    }
    const uint16_t red = (uint16_t)(((color >> 11) & 0x1Fu) * coverage / 3u);
    const uint16_t green = (uint16_t)(((color >> 5) & 0x3Fu) * coverage / 3u);
    const uint16_t blue = (uint16_t)((color & 0x1Fu) * coverage / 3u);
    return (uint16_t)((red << 11) | (green << 5) | blue);
}

static void draw_icon(const uint8_t *mask, uint16_t color) {
    for (int y = 0; y < ICON_HEIGHT; ++y) {
        for (int x = 0; x < ICON_WIDTH; ++x) {
            const size_t pixel_index = (size_t)y * ICON_WIDTH + (size_t)x;
            const uint8_t shift = (uint8_t)(6u - ((pixel_index & 3u) * 2u));
            const uint8_t coverage = (uint8_t)((mask[pixel_index >> 2] >> shift) & 0x03u);
            if (coverage != 0) {
                set_pixel(ICON_OFFSET_X + x, ICON_OFFSET_Y + y,
                          color_with_coverage(color, coverage));
            }
        }
    }
}

#define FONT_SCALE 2
#define GLYPH_WIDTH 5
#define GLYPH_HEIGHT 7
#define GLYPH_ADVANCE 6

static int text_width(const char *text) {
    return text == NULL ? 0 :
        (int)strlen(text) * GLYPH_ADVANCE * FONT_SCALE - FONT_SCALE;
}

static void draw_text_line(const char *text, int y, uint16_t color) {
    if (text == NULL || text[0] == '\0') {
        return;
    }

    int x = (UI_TILE_WIDTH - text_width(text)) / 2;
    for (const char *cursor = text; *cursor != '\0'; ++cursor) {
        const uint8_t *glyph = font5x7_glyph(*cursor);
        for (int column = 0; column < 5; ++column) {
            for (int row = 0; row < 7; ++row) {
                if ((glyph[column] & (1u << row)) != 0) {
                    for (int dy = 0; dy < FONT_SCALE; ++dy) {
                        for (int dx = 0; dx < FONT_SCALE; ++dx) {
                            set_pixel(x + column * FONT_SCALE + dx,
                                      y + row * FONT_SCALE + dy, color);
                        }
                    }
                }
            }
        }
        x += GLYPH_ADVANCE * FONT_SCALE;
    }
}

static void setup_pixel(int x, int y, uint16_t color) {
    if (x < 0 || x >= LCD_WIDTH || y < setup_stripe_y ||
        y >= setup_stripe_y + SETUP_STRIPE_HEIGHT || y >= LCD_HEIGHT) return;
    size_t offset = ((size_t)(y - setup_stripe_y) * LCD_WIDTH + (size_t)x) * 2;
    tile_buffer[offset] = (uint8_t)(color >> 8);
    tile_buffer[offset + 1] = (uint8_t)color;
}

static int setup_text_width(const char *text, int scale) {
    size_t length = text ? strlen(text) : 0;
    return length ? ((int)length * 6 - 1) * scale : 0;
}

static void setup_text(int x, int y, const char *text, int scale,
                       uint16_t color) {
    if (!text) return;
    for (const char *cursor = text; *cursor; ++cursor) {
        const uint8_t *glyph = font5x7_glyph(*cursor);
        for (int column = 0; column < 5; ++column)
            for (int row = 0; row < 7; ++row)
                if (glyph[column] & (1u << row))
                    for (int dy = 0; dy < scale; ++dy)
                        for (int dx = 0; dx < scale; ++dx)
                            setup_pixel(x + column * scale + dx,
                                        y + row * scale + dy, color);
        x += 6 * scale;
    }
}

static void setup_centered(int y, const char *text, int scale,
                           uint16_t color) {
    setup_text((LCD_WIDTH - setup_text_width(text, scale)) / 2,
               y, text, scale, color);
}

void ui_render_startup(bool keyboard_ready, bool network_ready,
                       const char *eddiscovery_status) {
    for (setup_stripe_y = 0; setup_stripe_y < LCD_HEIGHT;
         setup_stripe_y += SETUP_STRIPE_HEIGHT) {
        memset(tile_buffer, 0, sizeof(tile_buffer));
        setup_centered(28, "PICODECK ED KEYBOARD", 3, COLOR_ORANGE);
        setup_centered(62, "HID + USB NETWORK", 2, COLOR_MID_GRAY);

        setup_text(45, 115, "KEYBOARD", 2, COLOR_MID_GRAY);
        setup_text(275, 115, keyboard_ready ? "READY" : "WAITING", 2,
                   keyboard_ready ? COLOR_GREEN : COLOR_ORANGE);
        setup_text(45, 165, "USB NETWORK", 2, COLOR_MID_GRAY);
        setup_text(275, 165, network_ready ? "192.168.8.1" : "WAITING", 2,
                   network_ready ? COLOR_GREEN : COLOR_ORANGE);
        setup_text(45, 215, "WINDOWS", 2, COLOR_MID_GRAY);
        setup_text(275, 215, "192.168.8.2", 2, COLOR_WHITE);
        setup_text(45, 265, "EDDISCOVERY", 2, COLOR_MID_GRAY);
        setup_text(275, 265, eddiscovery_status ? eddiscovery_status : "STARTING",
                   2, eddiscovery_status && strcmp(eddiscovery_status, "CONNECTED") == 0
                          ? COLOR_GREEN : COLOR_ORANGE);

        int height = LCD_HEIGHT - setup_stripe_y;
        if (height > SETUP_STRIPE_HEIGHT) height = SETUP_STRIPE_HEIGHT;
        lcd_blit_rgb565_be(0, (uint16_t)setup_stripe_y, LCD_WIDTH,
                           (uint16_t)height, tile_buffer,
                           (size_t)LCD_WIDTH * (size_t)height * 2);
    }
}

void ui_init(void) {
    lcd_fill(COLOR_BLACK);
}

void ui_render_button(uint8_t index, bool active) {
    if (index >= g_button_count) {
        return;
    }

    const button_definition_t *button = &g_buttons[index];
    const uint16_t foreground = active ? COLOR_WHITE : COLOR_ORANGE;
    const uint8_t *icon = active ? button->icon_active : button->icon_normal;

    clear_tile(COLOR_BLACK);
    draw_border(foreground);
    draw_icon(icon, foreground);
    if (button->label_top[0] != '\0' && button->label_bottom[0] != '\0') {
        draw_text_line(button->label_top, 49, foreground);
        draw_text_line(button->label_bottom, 64, foreground);
    } else {
        const char *label = button->label_top[0] != '\0' ?
            button->label_top : button->label_bottom;
        draw_text_line(label, 57, foreground);
    }

    const uint16_t x = (uint16_t)((index % UI_COLUMNS) * UI_TILE_WIDTH);
    const uint16_t y = (uint16_t)((index / UI_COLUMNS) * UI_TILE_HEIGHT);
    lcd_blit_rgb565_be(x, y, UI_TILE_WIDTH, UI_TILE_HEIGHT,
                       tile_buffer, sizeof(tile_buffer));
}

void ui_render_all(void) {
    for (uint8_t i = 0; i < g_button_count; ++i) {
        ui_render_button(i, false);
    }
}

int ui_button_at(uint16_t x, uint16_t y) {
    if (x >= LCD_WIDTH || y >= LCD_HEIGHT) {
        return -1;
    }
    const int index = (int)(y / UI_TILE_HEIGHT) * UI_COLUMNS +
                      (int)(x / UI_TILE_WIDTH);
    return index < (int)g_button_count ? index : -1;
}
