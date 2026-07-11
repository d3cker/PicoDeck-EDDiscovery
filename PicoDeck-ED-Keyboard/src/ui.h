#pragma once

#include <stdbool.h>
#include <stdint.h>

void ui_init(void);
void ui_render_all(void);
void ui_render_startup(bool keyboard_ready, bool network_ready,
                       const char *eddiscovery_status);
void ui_render_button(uint8_t index, bool active);
int ui_button_at(uint16_t x, uint16_t y);
