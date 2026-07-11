#include <stdio.h>

#include "bsp/board_api.h"
#include "board.h"
#include "dashboard.h"
#include "edd_state.h"
#include "hardware/gpio.h"
#include "lcd.h"
#include "net_usb.h"
#include "pico/stdlib.h"
#include "tusb.h"
#include "websocket_client.h"

static edd_state_t s_edd;

typedef struct {
    bool raw_level;
    bool stable_level;
    uint64_t changed_ms;
} debounced_input_t;

static void on_websocket_message(const char *json, size_t length) {
    (void)edd_state_handle_json(&s_edd, json, length);
}

static void init_user_key(void) {
    gpio_init(PIN_USER_KEY);
    gpio_set_dir(PIN_USER_KEY, GPIO_IN);
    gpio_pull_up(PIN_USER_KEY);
}

static void debounced_input_init(debounced_input_t *input, uint pin,
                                 uint64_t now_ms) {
    input->raw_level = gpio_get(pin);
    input->stable_level = input->raw_level;
    input->changed_ms = now_ms;
}

static bool debounced_input_pressed(debounced_input_t *input, uint pin,
                                    uint64_t now_ms) {
    bool level = gpio_get(pin);
    if (level != input->raw_level) {
        input->raw_level = level;
        input->changed_ms = now_ms;
    } else if (input->stable_level != input->raw_level &&
               now_ms - input->changed_ms >= 35) {
        input->stable_level = input->raw_level;
        return !input->stable_level;
    }
    return false;
}

int main(void) {
    board_init();
    stdio_init_all();
    init_user_key();

    lcd_init();
    /* Visible proof that the LCD initialized before USB/network can fail. */
    lcd_fill(COLOR_ORANGE);
    sleep_ms(350);
    lcd_fill(COLOR_BLACK);
    edd_state_init(&s_edd);
    websocket_client_init(on_websocket_message);
    dashboard_init(&s_edd);
    dashboard_set_connection(false, WS_OFFLINE);
    for (int stripe = 0; stripe < 14; ++stripe) dashboard_task();

    net_usb_prepare_identity();
    tusb_rhport_init_t usb_init = {
        .role = TUSB_ROLE_DEVICE,
        .speed = TUSB_SPEED_AUTO,
    };
    tusb_init(BOARD_TUD_RHPORT, &usb_init);
    if (board_init_after_tusb) board_init_after_tusb();

    bool network_ok = net_usb_init();
    printf("PicoDeck ED Display v0.1.6\n");
    printf("USB network: %s\n", network_ok ? "ready" : "failed");

    uint64_t input_now = time_us_64() / 1000u;
    debounced_input_t user_key;
    debounced_input_t touch;
    debounced_input_init(&user_key, PIN_USER_KEY, input_now);
    debounced_input_init(&touch, PIN_TOUCH_IRQ, input_now);

    while (true) {
        tud_task();
        net_usb_task();
        bool ready = network_ok && net_usb_ready();
        websocket_client_task(ready);

        uint64_t now = time_us_64() / 1000u;
        bool key_pressed = debounced_input_pressed(&user_key, PIN_USER_KEY, now);
        bool screen_touched = debounced_input_pressed(&touch, PIN_TOUCH_IRQ, now);
        if (key_pressed || screen_touched) dashboard_next_page();

        dashboard_set_connection(ready, websocket_client_state());
        dashboard_task();
        sleep_us(250);
    }
}
