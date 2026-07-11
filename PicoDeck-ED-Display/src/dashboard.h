#pragma once

#include <stdbool.h>

#include "edd_state.h"
#include "websocket_client.h"

void dashboard_init(const edd_state_t *state);
void dashboard_set_connection(bool usb_network, websocket_state_t websocket);
void dashboard_next_page(void);
void dashboard_request_redraw(void);
void dashboard_task(void);

