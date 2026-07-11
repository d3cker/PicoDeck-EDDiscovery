#pragma once

#include <stddef.h>
#include <stdint.h>

#include "icons.h"

typedef struct {
    const char *label_top;
    const char *label_bottom;
    uint8_t hid_usage;
    const uint8_t *icon_normal;
    const uint8_t *icon_active;
} button_definition_t;

extern const button_definition_t g_buttons[];
extern const size_t g_button_count;

