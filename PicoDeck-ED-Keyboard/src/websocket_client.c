#include "websocket_client.h"

#include <string.h>

#include "board.h"
#include "lwip/ip_addr.h"
#include "lwip/tcp.h"
#include "pico/rand.h"
#include "pico/time.h"

#define RX_CAPACITY 8192u
#define INDICATOR_REFRESH_MS 10000u

static struct tcp_pcb *s_pcb;
static websocket_state_t s_state;
static websocket_message_cb_t s_message_cb;
static uint8_t s_rx[RX_CAPACITY + 1];
static size_t s_rx_length;
static uint8_t s_message[RX_CAPACITY + 1];
static size_t s_message_length;
static uint8_t s_fragment_opcode;
static uint64_t s_last_rx_ms;
static uint64_t s_last_ping_ms;
static uint64_t s_last_request_ms;
static uint64_t s_retry_at_ms;
static uint32_t s_retry_delay_ms;

static uint64_t now_ms(void) { return time_us_64() / 1000u; }

static void close_connection(bool abort_connection) {
    if (s_pcb) {
        tcp_arg(s_pcb, NULL);
        tcp_recv(s_pcb, NULL);
        tcp_sent(s_pcb, NULL);
        tcp_poll(s_pcb, NULL, 0);
        tcp_err(s_pcb, NULL);
        if (abort_connection || tcp_close(s_pcb) != ERR_OK) tcp_abort(s_pcb);
        s_pcb = NULL;
    }
    s_rx_length = 0;
    s_message_length = 0;
    s_fragment_opcode = 0;
}

static void schedule_retry(void) {
    close_connection(true);
    s_state = WS_ERROR;
    if (!s_retry_delay_ms) s_retry_delay_ms = 1000;
    s_retry_at_ms = now_ms() + s_retry_delay_ms;
    if (s_retry_delay_ms < 5000) s_retry_delay_ms += 1000;
}

static bool write_bytes(const void *data, size_t length) {
    if (!s_pcb || length > 0xffffu) return false;
    if (tcp_write(s_pcb, data, (u16_t)length, TCP_WRITE_FLAG_COPY) != ERR_OK)
        return false;
    return tcp_output(s_pcb) == ERR_OK;
}

static bool send_frame(uint8_t opcode, const void *payload, size_t length) {
    if (!s_pcb || length > 65535u) return false;
    uint8_t header[8];
    size_t header_length;
    header[0] = (uint8_t)(0x80u | (opcode & 0x0fu));
    if (length < 126) {
        header[1] = (uint8_t)(0x80u | length);
        header_length = 2;
    } else {
        header[1] = 0xfe;
        header[2] = (uint8_t)(length >> 8);
        header[3] = (uint8_t)length;
        header_length = 4;
    }
    uint32_t mask = get_rand_32();
    memcpy(header + header_length, &mask, sizeof(mask));
    header_length += sizeof(mask);
    if (tcp_write(s_pcb, header, (u16_t)header_length,
                  TCP_WRITE_FLAG_COPY | TCP_WRITE_FLAG_MORE) != ERR_OK)
        return false;

    const uint8_t *source = (const uint8_t *)payload;
    const uint8_t *mask_bytes = header + header_length - 4;
    uint8_t chunk[128];
    size_t offset = 0;
    while (offset < length) {
        size_t count = length - offset;
        if (count > sizeof(chunk)) count = sizeof(chunk);
        for (size_t i = 0; i < count; ++i)
            chunk[i] = source[offset + i] ^ mask_bytes[(offset + i) & 3u];
        uint8_t flags = TCP_WRITE_FLAG_COPY;
        if (offset + count < length) flags |= TCP_WRITE_FLAG_MORE;
        if (tcp_write(s_pcb, chunk, (u16_t)count, flags) != ERR_OK)
            return false;
        offset += count;
    }
    return tcp_output(s_pcb) == ERR_OK;
}

static bool request_indicator(void) {
    static const char request[] = "{\"requesttype\":\"indicator\"}";
    if (!send_frame(1, request, sizeof(request) - 1)) return false;
    s_last_request_ms = now_ms();
    return true;
}

static bool consume_handshake(void) {
    s_rx[s_rx_length] = 0;
    char *end = strstr((char *)s_rx, "\r\n\r\n");
    if (!end) return true;
    size_t header_length = (size_t)(end - (char *)s_rx) + 4;
    if (strncmp((char *)s_rx, "HTTP/1.1 101", 12) != 0 &&
        strncmp((char *)s_rx, "HTTP/1.0 101", 12) != 0) return false;
    memmove(s_rx, s_rx + header_length, s_rx_length - header_length);
    s_rx_length -= header_length;
    s_state = WS_OPEN;
    s_retry_delay_ms = 1000;
    s_last_rx_ms = s_last_ping_ms = now_ms();
    return request_indicator();
}

static bool consume_frames(void) {
    size_t used = 0;
    while (s_rx_length - used >= 2) {
        uint8_t *frame = s_rx + used;
        bool fin = (frame[0] & 0x80u) != 0;
        uint8_t opcode = frame[0] & 0x0fu;
        bool masked = (frame[1] & 0x80u) != 0;
        uint64_t length = frame[1] & 0x7fu;
        size_t header = 2;
        if (length == 126) {
            if (s_rx_length - used < 4) break;
            length = ((uint16_t)frame[2] << 8) | frame[3];
            header = 4;
        } else if (length == 127) {
            if (s_rx_length - used < 10) break;
            length = 0;
            for (size_t i = 0; i < 8; ++i)
                length = (length << 8) | frame[2 + i];
            header = 10;
        }
        if (masked) header += 4;
        if (length > RX_CAPACITY || header + length > RX_CAPACITY) return false;
        if (s_rx_length - used < header + (size_t)length) break;
        uint8_t *payload = frame + header;
        if (masked) {
            const uint8_t *mask = frame + header - 4;
            for (size_t i = 0; i < (size_t)length; ++i)
                payload[i] ^= mask[i & 3u];
        }
        bool control = (opcode & 0x08u) != 0;
        if (control && (!fin || length > 125)) return false;

        if (opcode == 0) {
            if (!s_fragment_opcode ||
                s_message_length + (size_t)length > RX_CAPACITY) return false;
            memcpy(s_message + s_message_length, payload, (size_t)length);
            s_message_length += (size_t)length;
            if (fin) {
                if (s_fragment_opcode == 1 && s_message_cb)
                    s_message_cb((const char *)s_message, s_message_length);
                s_message_length = 0;
                s_fragment_opcode = 0;
            }
        } else if (opcode == 1 || opcode == 2) {
            if (s_fragment_opcode) return false;
            if (fin) {
                if (opcode == 1 && s_message_cb)
                    s_message_cb((const char *)payload, (size_t)length);
            } else {
                memcpy(s_message, payload, (size_t)length);
                s_message_length = (size_t)length;
                s_fragment_opcode = opcode;
            }
        } else if (opcode == 8) {
            return false;
        } else if (opcode == 9) {
            if (!send_frame(10, payload, (size_t)length)) return false;
        } else if (opcode != 10) {
            return false;
        }
        used += header + (size_t)length;
    }
    if (used) {
        memmove(s_rx, s_rx + used, s_rx_length - used);
        s_rx_length -= used;
    }
    return true;
}

static err_t receive_cb(void *arg, struct tcp_pcb *pcb, struct pbuf *p,
                        err_t error) {
    (void)arg;
    if (!p) {
        schedule_retry();
        return ERR_ABRT;
    }
    if (error != ERR_OK || s_rx_length + p->tot_len > RX_CAPACITY) {
        pbuf_free(p);
        schedule_retry();
        return ERR_ABRT;
    }
    pbuf_copy_partial(p, s_rx + s_rx_length, p->tot_len, 0);
    s_rx_length += p->tot_len;
    tcp_recved(pcb, p->tot_len);
    pbuf_free(p);
    s_last_rx_ms = now_ms();
    bool ok = s_state == WS_HANDSHAKE ? consume_handshake() : consume_frames();
    if (ok && s_state == WS_OPEN) ok = consume_frames();
    if (!ok) {
        schedule_retry();
        return ERR_ABRT;
    }
    return ERR_OK;
}

static void error_cb(void *arg, err_t error) {
    (void)arg;
    (void)error;
    s_pcb = NULL;
    s_rx_length = s_message_length = 0;
    s_fragment_opcode = 0;
    s_state = WS_ERROR;
    if (!s_retry_delay_ms) s_retry_delay_ms = 1000;
    s_retry_at_ms = now_ms() + s_retry_delay_ms;
}

static err_t connected_cb(void *arg, struct tcp_pcb *pcb, err_t error) {
    (void)arg;
    (void)pcb;
    if (error != ERR_OK) {
        schedule_retry();
        return error;
    }
    s_state = WS_HANDSHAKE;
    static const char request[] =
        "GET / HTTP/1.1\r\n"
        "Host: 192.168.8.2:6502\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Key: UGljb0RlY2tLZXlib2FyZA==\r\n"
        "Sec-WebSocket-Version: 13\r\n"
        "Sec-WebSocket-Protocol: EDDJSON\r\n\r\n";
    if (!write_bytes(request, sizeof(request) - 1)) {
        schedule_retry();
        return ERR_ABRT;
    }
    return ERR_OK;
}

static void connect_now(void) {
    ip_addr_t address;
    IP4_ADDR(ip_2_ip4(&address), USB_NET_DEVICE_IP_A, USB_NET_DEVICE_IP_B,
             USB_NET_DEVICE_IP_C, USB_NET_HOST_IP_D);
    IP_SET_TYPE_VAL(address, IPADDR_TYPE_V4);
    s_pcb = tcp_new_ip_type(IPADDR_TYPE_V4);
    if (!s_pcb) {
        schedule_retry();
        return;
    }
    tcp_arg(s_pcb, NULL);
    tcp_recv(s_pcb, receive_cb);
    tcp_err(s_pcb, error_cb);
    s_state = WS_CONNECTING;
    s_last_rx_ms = now_ms();
    if (tcp_connect(s_pcb, &address, EDDISCOVERY_PORT, connected_cb) != ERR_OK)
        schedule_retry();
}

void websocket_client_init(websocket_message_cb_t message_cb) {
    s_message_cb = message_cb;
    s_state = WS_OFFLINE;
    s_retry_delay_ms = 1000;
}

void websocket_client_task(bool network_ready) {
    uint64_t now = now_ms();
    if (!network_ready) {
        if (s_pcb) close_connection(true);
        s_state = WS_OFFLINE;
        s_retry_at_ms = now + 500;
        return;
    }
    if (!s_pcb && (s_state == WS_OFFLINE || s_state == WS_ERROR) &&
        now >= s_retry_at_ms) {
        connect_now();
        return;
    }
    if (s_state == WS_OPEN) {
        if (now - s_last_rx_ms > 30000) {
            schedule_retry();
        } else if (now - s_last_request_ms > INDICATOR_REFRESH_MS) {
            if (!request_indicator()) schedule_retry();
        } else if (now - s_last_ping_ms > 10000) {
            static const char ping[] = "KB";
            if (!send_frame(9, ping, sizeof(ping) - 1)) schedule_retry();
            s_last_ping_ms = now;
        }
    } else if ((s_state == WS_CONNECTING || s_state == WS_HANDSHAKE) &&
               now - s_last_rx_ms > 10000) {
        schedule_retry();
    }
}

websocket_state_t websocket_client_state(void) { return s_state; }

const char *websocket_client_state_name(void) {
    static const char *names[] = {"USB OFFLINE", "CONNECTING", "HANDSHAKE",
                                  "CONNECTED", "RETRYING"};
    return names[(unsigned)s_state < 5 ? s_state : WS_ERROR];
}

