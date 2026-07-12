#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "websocket_client.h"

extern const char EDDJSON_REQUEST_STATUS[];
extern const char EDDJSON_REQUEST_INDICATOR[];
extern const char EDDJSON_REQUEST_RECENT_JOURNAL[];

typedef struct {
    const char *host;
    uint16_t port;
    const char *websocket_key;
    const char *ping_payload;
    const char *const *initial_requests;
    size_t initial_request_count;
    const char *periodic_request;
    uint32_t periodic_interval_ms;
} eddjson_client_config_t;

void eddjson_client_init(const eddjson_client_config_t *config,
                         websocket_message_cb_t message_cb);
void eddjson_client_task(bool network_ready);
websocket_state_t eddjson_client_state(void);
const char *eddjson_client_state_name(void);
uint32_t eddjson_client_messages(void);
