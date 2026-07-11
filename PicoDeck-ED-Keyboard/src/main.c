#include <stdbool.h>
#include <stdint.h>

#include "bsp/board_api.h"
#include "pico/bootrom.h"
#include "pico/stdlib.h"
#include "tusb.h"

#include "buttons.h"
#include "edd_buttons.h"
#include "lcd.h"
#include "net_usb.h"
#include "touch.h"
#include "ui.h"
#include "websocket_client.h"

#define HID_PRESS_TIME_MS 60u
#define TOUCH_STABLE_POLLS 3u
#define RELEASE_STABLE_POLLS 3u
#define BOOTSEL_HOLD_TIME_MS 6000u
#define STARTUP_MAX_TIME_MS 15000u
#define CONNECTED_HOLD_TIME_MS 800u

static uint32_t s_dirty_buttons;

static uint64_t now_ms(void) { return time_us_64() / 1000u; }

static void on_websocket_message(const char *json, size_t length) {
    s_dirty_buttons |= edd_buttons_handle_json(json, length);
}

static bool send_key_down(uint8_t usage) {
    if (!tud_mounted() || !tud_hid_ready()) return false;
    const uint8_t keys[6] = {usage, 0, 0, 0, 0, 0};
    return tud_hid_keyboard_report(0, 0, keys);
}

static bool send_key_up(void) {
    if (!tud_mounted() || !tud_hid_ready()) return false;
    return tud_hid_keyboard_report(0, 0, NULL);
}

static void render_keyboard(void) {
    for (uint8_t i = 0; i < g_button_count; ++i)
        ui_render_button(i, edd_button_active(i));
    s_dirty_buttons = 0;
}

static void render_one_dirty_button(int touched_button) {
    if (!s_dirty_buttons) return;
    for (uint8_t i = 0; i < g_button_count; ++i) {
        uint32_t bit = 1u << i;
        if (!(s_dirty_buttons & bit)) continue;
        s_dirty_buttons &= ~bit;
        ui_render_button(i, edd_button_active(i) || touched_button == i);
        return;
    }
}

int main(void) {
    board_init();
    touch_init();
    lcd_init();
    ui_init();
    edd_buttons_init();
    websocket_client_init(on_websocket_message);
    ui_render_startup(false, false, "STARTING");

    net_usb_prepare_identity();
    tusb_rhport_init_t usb_init = {
        .role = TUSB_ROLE_DEVICE,
        .speed = TUSB_SPEED_AUTO,
    };
    tusb_init(BOARD_TUD_RHPORT, &usb_init);
    if (board_init_after_tusb) board_init_after_tusb();
    bool network_ok = net_usb_init();

    bool keyboard_visible = false;
    uint64_t startup_at = now_ms();
    uint64_t connected_at = 0;
    bool last_hid_ready = false;
    bool last_network_ready = false;
    websocket_state_t last_websocket = (websocket_state_t)99;

    int candidate_button = -1;
    uint8_t candidate_count = 0;
    uint8_t release_count = 0;
    int active_button = -1;
    bool armed = true;
    bool press_pending = false;
    bool key_is_down = false;
    absolute_time_t key_release_at = nil_time;
    absolute_time_t bootsel_hold_at = nil_time;

    while (true) {
        tud_task();
        net_usb_task();
        bool network_ready = network_ok && net_usb_ready();
        websocket_client_task(network_ready);
        websocket_state_t websocket = websocket_client_state();
        bool hid_ready = tud_mounted();
        uint64_t now = now_ms();

        if (!keyboard_visible) {
            if (hid_ready != last_hid_ready ||
                network_ready != last_network_ready ||
                websocket != last_websocket) {
                last_hid_ready = hid_ready;
                last_network_ready = network_ready;
                last_websocket = websocket;
                ui_render_startup(hid_ready, network_ready,
                                  websocket_client_state_name());
            }
            if (hid_ready && websocket == WS_OPEN) {
                if (!connected_at) connected_at = now;
            } else {
                connected_at = 0;
            }
            if ((connected_at && now - connected_at >= CONNECTED_HOLD_TIME_MS) ||
                now - startup_at >= STARTUP_MAX_TIME_MS) {
                render_keyboard();
                keyboard_visible = true;
            } else {
                sleep_ms(1);
                continue;
            }
        }

        uint16_t touch_x = 0;
        uint16_t touch_y = 0;
        bool touching = touch_poll(&touch_x, &touch_y);
        int touched_button = touching ? ui_button_at(touch_x, touch_y) : -1;

        if (armed && touched_button >= 0) {
            if (touched_button == candidate_button) {
                if (candidate_count < 255) ++candidate_count;
            } else {
                candidate_button = touched_button;
                candidate_count = 1;
            }
            if (candidate_count >= TOUCH_STABLE_POLLS) {
                active_button = candidate_button;
                armed = false;
                press_pending = true;
                release_count = 0;
                bootsel_hold_at = active_button == (int)g_button_count - 1
                    ? make_timeout_time_ms(BOOTSEL_HOLD_TIME_MS) : nil_time;
                ui_render_button((uint8_t)active_button, true);
            }
        } else if (armed) {
            candidate_button = -1;
            candidate_count = 0;
        }

        if (!armed) {
            if (active_button == (int)g_button_count - 1 && touching &&
                touched_button == active_button && !is_nil_time(bootsel_hold_at) &&
                absolute_time_diff_us(get_absolute_time(), bootsel_hold_at) <= 0)
                reset_usb_boot(0, 0);

            if (touching) release_count = 0;
            else if (release_count < 255) ++release_count;

            if (press_pending && send_key_down(g_buttons[active_button].hid_usage)) {
                press_pending = false;
                key_is_down = true;
                key_release_at = make_timeout_time_ms(HID_PRESS_TIME_MS);
            }
            if (key_is_down &&
                absolute_time_diff_us(get_absolute_time(), key_release_at) <= 0 &&
                send_key_up()) key_is_down = false;

            if (!key_is_down && !press_pending &&
                release_count >= RELEASE_STABLE_POLLS) {
                ui_render_button((uint8_t)active_button,
                                 edd_button_active((uint8_t)active_button));
                active_button = -1;
                candidate_button = -1;
                candidate_count = 0;
                armed = true;
                bootsel_hold_at = nil_time;
            }
            if (press_pending && release_count >= RELEASE_STABLE_POLLS) {
                press_pending = false;
                ui_render_button((uint8_t)active_button,
                                 edd_button_active((uint8_t)active_button));
                active_button = -1;
                armed = true;
                bootsel_hold_at = nil_time;
            }
        }

        render_one_dirty_button(active_button);
        sleep_ms(1);
    }
}

