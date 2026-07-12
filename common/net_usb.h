#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint8_t device_ip[4];
    uint8_t host_ip[4];
    const char *dhcp_domain;
    char interface_name[2];
} net_usb_config_t;

void net_usb_prepare_identity(void);
bool net_usb_init(const net_usb_config_t *config);
void net_usb_task(void);
bool net_usb_ready(void);
const char *net_usb_device_ip(void);
const char *net_usb_host_ip(void);
