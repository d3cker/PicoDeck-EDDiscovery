#pragma once

#include <stdbool.h>
#include <stddef.h>

typedef enum {
    WS_OFFLINE = 0,
    WS_CONNECTING,
    WS_HANDSHAKE,
    WS_OPEN,
    WS_ERROR,
} websocket_state_t;

typedef void (*websocket_message_cb_t)(const char *json, size_t length);

void websocket_client_init(websocket_message_cb_t message_cb);
void websocket_client_task(bool network_ready);
websocket_state_t websocket_client_state(void);
const char *websocket_client_state_name(void);

