#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
    WS_OFFLINE = 0,
    WS_CONNECTING,
    WS_HANDSHAKE,
    WS_OPEN,
    WS_ERROR,
} websocket_state_t;

typedef void (*websocket_message_cb_t)(const char *message, size_t length);
typedef bool (*websocket_opened_cb_t)(void);

typedef struct {
    const char *host;
    uint16_t port;
    const char *path;
    const char *protocol;
    const char *key;
    const char *ping_payload;
    uint32_t ping_interval_ms;
    uint32_t idle_timeout_ms;
    uint32_t connect_timeout_ms;
} websocket_client_config_t;

void websocket_client_init(const websocket_client_config_t *config,
                           websocket_message_cb_t message_cb,
                           websocket_opened_cb_t opened_cb);
void websocket_client_task(bool network_ready);
bool websocket_client_send_text(const char *message);
void websocket_client_request_reconnect(void);
websocket_state_t websocket_client_state(void);
const char *websocket_client_state_name(void);
uint32_t websocket_client_messages(void);
