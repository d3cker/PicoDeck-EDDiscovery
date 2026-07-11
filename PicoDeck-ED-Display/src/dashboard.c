#include "dashboard.h"

#include <stdio.h>
#include <string.h>

#include "board.h"
#include "font5x7.h"
#include "lcd.h"
#include "net_usb.h"
#include "pico/time.h"

#define STRIPE_HEIGHT 24
#define PAGE_COUNT 4
#define SCROLL_REDRAW_MS 250u
#define SCROLL_HOLD_MS 900u
#define SCROLL_PIXEL_MS 35u

static const edd_state_t *s_data;
static uint8_t s_pixels[LCD_WIDTH * STRIPE_HEIGHT * 2];
static int s_stripe_y = -1;
static unsigned s_page;
static uint32_t s_generation;
static bool s_usb_network;
static websocket_state_t s_websocket;
static bool s_last_usb;
static websocket_state_t s_last_websocket;
static uint64_t s_render_ms;
static uint64_t s_next_scroll_redraw_ms;

static void pixel(int x, int y, uint16_t color) {
    if (x < 0 || x >= LCD_WIDTH || y < s_stripe_y ||
        y >= s_stripe_y + STRIPE_HEIGHT || y >= LCD_HEIGHT) return;
    size_t offset = ((size_t)(y - s_stripe_y) * LCD_WIDTH + (size_t)x) * 2;
    s_pixels[offset] = (uint8_t)(color >> 8);
    s_pixels[offset + 1] = (uint8_t)color;
}

static void rect(int x, int y, int width, int height, uint16_t color) {
    int left = x < 0 ? 0 : x;
    int right = x + width > LCD_WIDTH ? LCD_WIDTH : x + width;
    int top = y < s_stripe_y ? s_stripe_y : y;
    int bottom = y + height > s_stripe_y + STRIPE_HEIGHT
                     ? s_stripe_y + STRIPE_HEIGHT : y + height;
    if (bottom > LCD_HEIGHT) bottom = LCD_HEIGHT;
    for (int py = top; py < bottom; ++py)
        for (int px = left; px < right; ++px) pixel(px, py, color);
}

static void clipped_rect(int x, int y, int width, int height,
                         int clip_x, int clip_width, uint16_t color) {
    int left = x > clip_x ? x : clip_x;
    int right = x + width < clip_x + clip_width
                    ? x + width : clip_x + clip_width;
    if (right > left) rect(left, y, right - left, height, color);
}

static void outline(int x, int y, int width, int height, uint16_t color) {
    rect(x, y, width, 1, color);
    rect(x, y + height - 1, width, 1, color);
    rect(x, y, 1, height, color);
    rect(x + width - 1, y, 1, height, color);
}

static void text_at(int x, int y, const char *text, int scale,
                    uint16_t color) {
    if (!text) return;
    for (const char *c = text; *c; ++c) {
        const uint8_t *glyph = font5x7_glyph(*c);
        for (int column = 0; column < 5; ++column)
            for (int row = 0; row < 7; ++row)
                if (glyph[column] & (1u << row))
                    rect(x + column * scale, y + row * scale, scale, scale,
                         color);
        x += 6 * scale;
        if (x >= LCD_WIDTH) break;
    }
}

static void text_at_clipped(int x, int y, const char *text, int scale,
                            uint16_t color, int clip_x, int clip_width) {
    if (!text || clip_width <= 0) return;
    for (const char *c = text; *c; ++c) {
        const uint8_t *glyph = font5x7_glyph(*c);
        for (int column = 0; column < 5; ++column)
            for (int row = 0; row < 7; ++row)
                if (glyph[column] & (1u << row))
                    clipped_rect(x + column * scale, y + row * scale,
                                 scale, scale, clip_x, clip_width, color);
        x += 6 * scale;
        if (x >= clip_x + clip_width) break;
    }
}

static int text_width(const char *text, int scale) {
    size_t length = text ? strlen(text) : 0;
    return length ? ((int)length * 6 - 1) * scale : 0;
}

static void centered(int y, const char *text, int scale, uint16_t color) {
    int x = (LCD_WIDTH - text_width(text, scale)) / 2;
    if (x < 4) x = 4;
    text_at(x, y, text, scale, color);
}

static const char *value_or_dash(const char *value) {
    return value && value[0] ? value : "-";
}

static int scroll_offset(const char *text, int scale, int width) {
    int overflow = text_width(text, scale) - width;
    if (overflow <= 0) return 0;
    uint64_t travel_ms = (uint64_t)overflow * SCROLL_PIXEL_MS;
    uint64_t cycle_ms = 2u * (SCROLL_HOLD_MS + travel_ms);
    uint64_t phase = s_render_ms % cycle_ms;
    if (phase < SCROLL_HOLD_MS) return 0;
    phase -= SCROLL_HOLD_MS;
    if (phase < travel_ms) return (int)(phase / SCROLL_PIXEL_MS);
    phase -= travel_ms;
    if (phase < SCROLL_HOLD_MS) return overflow;
    phase -= SCROLL_HOLD_MS;
    return overflow - (int)(phase / SCROLL_PIXEL_MS);
}

static void value_area(int x, int y, int width, const char *value, int scale,
                       uint16_t color, bool center_if_short) {
    const char *shown = value_or_dash(value);
    int content_width = text_width(shown, scale);
    int draw_x = x;
    if (content_width <= width && center_if_short)
        draw_x += (width - content_width) / 2;
    else if (content_width > width)
        draw_x -= scroll_offset(shown, scale, width);
    text_at_clipped(draw_x, y, shown, scale, color, x, width);
}

static void label_value(int x, int y, int width, const char *label,
                        const char *value, uint16_t color) {
    text_at(x, y, label, 2, COLOR_MID_GRAY);
    value_area(x, y + 18, width, value, 2, color, false);
}

static void header(const char *page) {
    rect(0, 0, LCD_WIDTH, 38, COLOR_DARK_GRAY);
    text_at(12, 10, "ED DISCOVERY", 2, COLOR_ORANGE);
    text_at(190, 10, page, 2, COLOR_WHITE);
    const char *status;
    uint16_t status_color;
    if (!s_usb_network) {
        status = "USB OFF";
        status_color = COLOR_RED;
    } else if (s_websocket == WS_OPEN) {
        status = "ONLINE";
        status_color = COLOR_GREEN;
    } else {
        status = "RECONNECT";
        status_color = COLOR_ORANGE;
    }
    text_at(474 - text_width(status, 2), 10, status, 2, status_color);
    rect(0, 37, LCD_WIDTH, 2, COLOR_ORANGE);
}

static void pill(int x, int y, int width, const char *name, bool active,
                 bool warning) {
    uint16_t color = active ? (warning ? COLOR_RED : COLOR_GREEN) : COLOR_MID_GRAY;
    outline(x, y, width, 27, color);
    int name_x = x + (width - text_width(name, 2)) / 2;
    text_at(name_x, y + 7, name, 2, color);
}

static void draw_overview(void) {
    header("ROUTE");
    text_at(14, 52, "DESTINATION", 2, COLOR_MID_GRAY);
    value_area(14, 72, 452, s_data->route_target, 3, COLOR_WHITE, false);
    char jumps[16];
    if (s_data->remaining_jumps >= 0) snprintf(jumps, sizeof(jumps), "%d", s_data->remaining_jumps);
    else strcpy(jumps, "-");
    centered(110, jumps, 10, COLOR_ORANGE);
    centered(188, "JUMPS REMAINING", 2, COLOR_MID_GRAY);
    label_value(14, 222, 210, "CURRENT", s_data->system, COLOR_CYAN);
    label_value(250, 222, 216, "NEXT", s_data->next_system, COLOR_CYAN);
    pill(4, 282, 116, "SHIELDS", s_data->shields_up, false);
    pill(124, 282, 116, "MASS LOCK", s_data->mass_locked, true);
    pill(244, 282, 116, "LOW FUEL", s_data->low_fuel, true);
    pill(364, 282, 116, "DANGER", s_data->danger || s_data->interdicted, true);
}

static void draw_ship(void) {
    header("SHIP");
    value_area(12, 55, 456, s_data->ship, 3, COLOR_WHITE, true);
    label_value(20, 105, 140, "FUEL", s_data->fuel, COLOR_ORANGE);
    label_value(180, 105, 130, "TANK", s_data->tank_size, COLOR_WHITE);
    label_value(330, 105, 136, "RANGE", s_data->jump_range, COLOR_CYAN);
    label_value(20, 170, 140, "CARGO", s_data->cargo, COLOR_WHITE);
    label_value(180, 170, 130, "MODE", s_data->mode, COLOR_WHITE);
    label_value(330, 170, 136, "LEGAL", s_data->legal_state, COLOR_WHITE);
    char pips[40];
    snprintf(pips, sizeof(pips), "SYS %d  ENG %d  WEP %d",
             s_data->pips[0], s_data->pips[1], s_data->pips[2]);
    centered(242, pips, 3, COLOR_ORANGE);
    pill(50, 285, 160, "OVERHEAT", s_data->overheat, true);
    pill(270, 285, 160, "INTERDICT", s_data->interdicted, true);
}

static void draw_system(void) {
    header("SYSTEM");
    value_area(10, 57, 460, s_data->system, 4, COLOR_ORANGE, true);
    label_value(20, 115, 210, "BODY", s_data->body, COLOR_WHITE);
    label_value(250, 115, 216, "STATION", s_data->station, COLOR_WHITE);
    label_value(20, 180, 210, "ALLEGIANCE", s_data->allegiance, COLOR_CYAN);
    label_value(250, 180, 216, "ECONOMY", s_data->economy, COLOR_CYAN);
    label_value(20, 245, 210, "SECURITY", s_data->security, COLOR_WHITE);
    label_value(250, 245, 216, "SOL DIST", s_data->sol_dist, COLOR_WHITE);
}

static void draw_info(void) {
    header("INFO");
    text_at(20, 52, "COMMANDER", 2, COLOR_MID_GRAY);
    value_area(20, 72, 446, s_data->commander, 3, COLOR_ORANGE, false);
    label_value(20, 125, 210, "GAME MODE", s_data->game_mode, COLOR_CYAN);
    label_value(250, 125, 216, "FLIGHT STATE", s_data->mode, COLOR_CYAN);
    label_value(20, 190, 210, "CREDITS", s_data->credits, COLOR_WHITE);
    label_value(250, 190, 216, "SOL DIST", s_data->sol_dist, COLOR_WHITE);
    text_at(20, 255, "CURRENT SYSTEM", 2, COLOR_MID_GRAY);
    value_area(20, 275, 446, s_data->system, 2, COLOR_WHITE, false);
}

static void draw_diagnostics(void) {
    header("SETUP");
    centered(62, "USB NETWORK", 3, s_usb_network ? COLOR_GREEN : COLOR_RED);
    label_value(30, 110, 185, "PICO", net_usb_device_ip(), COLOR_WHITE);
    label_value(265, 110, 185, "WINDOWS", net_usb_host_ip(), COLOR_WHITE);
    label_value(30, 180, 420, "EDDISCOVERY", websocket_client_state_name(),
                s_websocket == WS_OPEN ? COLOR_GREEN : COLOR_ORANGE);
    text_at(30, 238, "WEB SERVER PORT 6502", 2, COLOR_MID_GRAY);
    text_at(30, 265, "CONNECT NATIVE PICO USB TO PC", 2, COLOR_WHITE);
    text_at(30, 290, "ENABLE EDDISCOVERY WEB SERVER", 2, COLOR_WHITE);
}

static void render_stripe(void) {
    memset(s_pixels, 0, sizeof(s_pixels));
    if (!s_usb_network) draw_diagnostics();
    else if (s_page == 0) draw_overview();
    else if (s_page == 1) draw_ship();
    else if (s_page == 2) draw_system();
    else draw_info();
}

static bool too_wide(const char *value, int scale, int width) {
    return text_width(value_or_dash(value), scale) > width;
}

static bool page_needs_scroll(void) {
    if (!s_usb_network) return false;
    if (s_page == 0)
        return too_wide(s_data->route_target, 3, 452) ||
               too_wide(s_data->system, 2, 210) ||
               too_wide(s_data->next_system, 2, 216);
    if (s_page == 1)
        return too_wide(s_data->ship, 3, 456) ||
               too_wide(s_data->fuel, 2, 140) ||
               too_wide(s_data->tank_size, 2, 130) ||
               too_wide(s_data->jump_range, 2, 136) ||
               too_wide(s_data->cargo, 2, 140) ||
               too_wide(s_data->mode, 2, 130) ||
               too_wide(s_data->legal_state, 2, 136);
    if (s_page == 2)
        return too_wide(s_data->system, 4, 460) ||
               too_wide(s_data->body, 2, 210) ||
               too_wide(s_data->station, 2, 216) ||
               too_wide(s_data->allegiance, 2, 210) ||
               too_wide(s_data->economy, 2, 216) ||
               too_wide(s_data->security, 2, 210) ||
               too_wide(s_data->sol_dist, 2, 216);
    return too_wide(s_data->commander, 3, 446) ||
           too_wide(s_data->game_mode, 2, 210) ||
           too_wide(s_data->mode, 2, 216) ||
           too_wide(s_data->credits, 2, 210) ||
           too_wide(s_data->sol_dist, 2, 216) ||
           too_wide(s_data->system, 2, 446);
}

void dashboard_init(const edd_state_t *state) {
    s_data = state;
    s_stripe_y = 0;
    s_generation = state->generation;
    s_websocket = WS_OFFLINE;
    s_last_websocket = (websocket_state_t)99;
}

void dashboard_set_connection(bool usb_network, websocket_state_t websocket) {
    s_usb_network = usb_network;
    s_websocket = websocket;
}

void dashboard_next_page(void) {
    s_page = (s_page + 1) % PAGE_COUNT;
    dashboard_request_redraw();
}

void dashboard_request_redraw(void) {
    s_stripe_y = 0;
    s_render_ms = time_us_64() / 1000u;
}

void dashboard_task(void) {
    if (!s_data) return;
    uint64_t now = time_us_64() / 1000u;
    if (s_data->generation != s_generation || s_usb_network != s_last_usb ||
        s_websocket != s_last_websocket) {
        s_generation = s_data->generation;
        s_last_usb = s_usb_network;
        s_last_websocket = s_websocket;
        dashboard_request_redraw();
    }
    if (s_stripe_y < 0 && page_needs_scroll() &&
        now >= s_next_scroll_redraw_ms) {
        dashboard_request_redraw();
    }
    if (s_stripe_y < 0) return;
    render_stripe();
    int height = LCD_HEIGHT - s_stripe_y;
    if (height > STRIPE_HEIGHT) height = STRIPE_HEIGHT;
    lcd_blit_rgb565_be(0, (uint16_t)s_stripe_y, LCD_WIDTH, (uint16_t)height,
                       s_pixels, (size_t)LCD_WIDTH * height * 2);
    s_stripe_y += height;
    if (s_stripe_y >= LCD_HEIGHT) {
        s_stripe_y = -1;
        s_next_scroll_redraw_ms = now + SCROLL_REDRAW_MS;
    }
}
