#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

void edd_buttons_init(void);
uint32_t edd_buttons_handle_json(const char *json, size_t length);
bool edd_button_active(uint8_t index);

