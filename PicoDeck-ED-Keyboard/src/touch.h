#pragma once

#include <stdbool.h>
#include <stdint.h>

void touch_init(void);
bool touch_poll(uint16_t *x, uint16_t *y);

