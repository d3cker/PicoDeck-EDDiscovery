#include "eddjson_client.h"

#include <string.h>

#include "pico/time.h"

const char EDDJSON_REQUEST_STATUS[] =
    "{\"requesttype\":\"status\",\"entry\":-1}";
const char EDDJSON_REQUEST_INDICATOR[] =
    "{\"requesttype\":\"indicator\"}";
const char EDDJSON_REQUEST_RECENT_JOURNAL[] =
    "{\"requesttype\":\"journal\",\"start\":-1,\"length\":50}";

static eddjson_client_config_t s_config;
static uint64_t s_last_periodic_request_ms;

static uint64_t now_ms(void) { return time_us_64() / 1000u; }

static bool on_websocket_opened(void) {
    if (s_config.initial_request_count && !s_config.initial_requests)
        return false;
    for (size_t i = 0; i < s_config.initial_request_count; ++i) {
        const char *request = s_config.initial_requests[i];
        if (request && !websocket_client_send_text(request)) return false;
    }
    s_last_periodic_request_ms = now_ms();
    return true;
}

void eddjson_client_init(const eddjson_client_config_t *config,
                         websocket_message_cb_t message_cb) {
    memset(&s_config, 0, sizeof(s_config));
    if (config) s_config = *config;
    websocket_client_config_t websocket = {
        .host = s_config.host,
        .port = s_config.port,
        .path = "/",
        .protocol = "EDDJSON",
        .key = s_config.websocket_key,
        .ping_payload = s_config.ping_payload,
        .ping_interval_ms = 10000u,
        .idle_timeout_ms = 30000u,
        .connect_timeout_ms = 10000u,
    };
    websocket_client_init(&websocket, message_cb, on_websocket_opened);
}

void eddjson_client_task(bool network_ready) {
    websocket_client_task(network_ready);
    if (websocket_client_state() != WS_OPEN ||
        !s_config.periodic_request || !s_config.periodic_interval_ms) {
        return;
    }
    uint64_t now = now_ms();
    if (now - s_last_periodic_request_ms <= s_config.periodic_interval_ms)
        return;
    if (!websocket_client_send_text(s_config.periodic_request)) {
        websocket_client_request_reconnect();
        return;
    }
    s_last_periodic_request_ms = now;
}

websocket_state_t eddjson_client_state(void) {
    return websocket_client_state();
}

const char *eddjson_client_state_name(void) {
    return websocket_client_state_name();
}

uint32_t eddjson_client_messages(void) {
    return websocket_client_messages();
}
