#pragma once

#include <stdbool.h>
#include <stdint.h>

void net_usb_prepare_identity(void);
bool net_usb_init(void);
void net_usb_task(void);
bool net_usb_ready(void);
const char *net_usb_device_ip(void);
const char *net_usb_host_ip(void);

