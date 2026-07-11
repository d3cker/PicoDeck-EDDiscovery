#include "net_usb.h"

#include <string.h>

#include "board.h"
#include "dhserver.h"
#include "lwip/etharp.h"
#include "lwip/init.h"
#include "lwip/ip.h"
#include "lwip/netif.h"
#include "lwip/pbuf.h"
#include "lwip/timeouts.h"
#include "pico/unique_id.h"
#include "tusb.h"

#define INIT_IP4(a, b, c, d) { PP_HTONL(LWIP_MAKEU32(a, b, c, d)) }

uint8_t tud_network_mac_address[6] = {0x02, 0x50, 0x44, 0x4b, 0x42, 0x01};

static struct netif s_netif;
static struct pbuf *s_received_frame;
static bool s_initialized;

static const ip4_addr_t s_ipaddr = INIT_IP4(USB_NET_DEVICE_IP_A,
    USB_NET_DEVICE_IP_B, USB_NET_DEVICE_IP_C, USB_NET_DEVICE_IP_D);
static const ip4_addr_t s_netmask = INIT_IP4(255, 255, 255, 0);
static const ip4_addr_t s_gateway = INIT_IP4(0, 0, 0, 0);
static dhcp_entry_t s_entries[] = {
    {{0}, INIT_IP4(USB_NET_DEVICE_IP_A, USB_NET_DEVICE_IP_B,
                   USB_NET_DEVICE_IP_C, USB_NET_HOST_IP_D), 24 * 60 * 60},
};
static const dhcp_config_t s_dhcp_config = {
    .router = INIT_IP4(0, 0, 0, 0),
    .port = 67,
    .dns = INIT_IP4(0, 0, 0, 0),
    .domain = "keyboard.usb",
    .num_entry = 1,
    .entries = s_entries,
};

void net_usb_prepare_identity(void) {
    pico_unique_board_id_t id;
    pico_get_unique_board_id(&id);
    tud_network_mac_address[0] = 0x02;
    for (size_t i = 0; i < 5; ++i)
        tud_network_mac_address[i + 1] = id.id[i + 3];
}

static err_t linkoutput_cb(struct netif *netif, struct pbuf *p) {
    (void)netif;
    if (!tud_ready()) return ERR_USE;
    if (!tud_network_can_xmit(p->tot_len)) return ERR_MEM;
    tud_network_xmit(p, 0);
    return ERR_OK;
}

static err_t netif_init_cb(struct netif *netif) {
    netif->mtu = CFG_TUD_NET_MTU;
    netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP |
                   NETIF_FLAG_LINK_UP | NETIF_FLAG_UP;
    netif->name[0] = 'K';
    netif->name[1] = 'B';
    netif->linkoutput = linkoutput_cb;
    netif->output = etharp_output;
    return ERR_OK;
}

bool net_usb_init(void) {
    lwip_init();
    s_netif.hwaddr_len = sizeof(tud_network_mac_address);
    memcpy(s_netif.hwaddr, tud_network_mac_address,
           sizeof(tud_network_mac_address));
    s_netif.hwaddr[5] ^= 1u;
    if (!netif_add(&s_netif, &s_ipaddr, &s_netmask, &s_gateway, NULL,
                   netif_init_cb, ip_input)) return false;
    netif_set_default(&s_netif);
    netif_set_up(&s_netif);
    netif_set_link_up(&s_netif);
    s_initialized = dhserv_init(&s_dhcp_config) == ERR_OK;
    return s_initialized;
}

void net_usb_task(void) {
    if (s_received_frame) {
        struct pbuf *frame = s_received_frame;
        s_received_frame = NULL;
        if (ethernet_input(frame, &s_netif) != ERR_OK) pbuf_free(frame);
        tud_network_recv_renew();
    }
    sys_check_timeouts();
}

bool net_usb_ready(void) {
    return s_initialized && tud_mounted() && netif_is_up(&s_netif) &&
           netif_is_link_up(&s_netif);
}

const char *net_usb_device_ip(void) { return "192.168.8.1"; }
const char *net_usb_host_ip(void) { return "192.168.8.2"; }

bool tud_network_recv_cb(const uint8_t *src, uint16_t size) {
    if (s_received_frame) return false;
    if (!size) return true;
    struct pbuf *p = pbuf_alloc(PBUF_RAW, size, PBUF_POOL);
    if (!p || pbuf_take(p, src, size) != ERR_OK) {
        if (p) pbuf_free(p);
        return false;
    }
    s_received_frame = p;
    return true;
}

uint16_t tud_network_xmit_cb(uint8_t *dst, void *ref, uint16_t arg) {
    (void)arg;
    struct pbuf *p = (struct pbuf *)ref;
    return pbuf_copy_partial(p, dst, p->tot_len, 0);
}

void tud_network_init_cb(void) {
    if (s_received_frame) {
        pbuf_free(s_received_frame);
        s_received_frame = NULL;
    }
}

