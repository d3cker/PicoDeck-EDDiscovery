#include "bsp/board_api.h"
#include "tusb.h"

#include <string.h>

enum {
    STRID_LANGID = 0,
    STRID_MANUFACTURER,
    STRID_PRODUCT,
    STRID_SERIAL,
    STRID_NETWORK,
    STRID_MAC,
};

enum {
    ITF_NUM_HID = 0,
    ITF_NUM_NCM,
    ITF_NUM_NCM_DATA,
    ITF_NUM_TOTAL,
};

#define USB_PID 0x4024
#define EPNUM_HID 0x81
#define EPNUM_NET_NOTIF 0x82
#define EPNUM_NET_OUT 0x03
#define EPNUM_NET_IN 0x83

static const tusb_desc_device_t device_descriptor = {
    .bLength = sizeof(tusb_desc_device_t),
    .bDescriptorType = TUSB_DESC_DEVICE,
    .bcdUSB = 0x0201,
    .bDeviceClass = TUSB_CLASS_MISC,
    .bDeviceSubClass = MISC_SUBCLASS_COMMON,
    .bDeviceProtocol = MISC_PROTOCOL_IAD,
    .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor = 0xCAFE,
    .idProduct = USB_PID,
    .bcdDevice = 0x0102,
    .iManufacturer = STRID_MANUFACTURER,
    .iProduct = STRID_PRODUCT,
    .iSerialNumber = STRID_SERIAL,
    .bNumConfigurations = 1,
};

const uint8_t *tud_descriptor_device_cb(void) {
    return (const uint8_t *)&device_descriptor;
}

static const uint8_t hid_report_descriptor[] = {
    TUD_HID_REPORT_DESC_KEYBOARD()
};

const uint8_t *tud_hid_descriptor_report_cb(uint8_t instance) {
    (void)instance;
    return hid_report_descriptor;
}

#define CONFIG_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_HID_DESC_LEN + \
                          TUD_CDC_NCM_DESC_LEN)

static const uint8_t configuration_descriptor[] = {
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN,
                          TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 250),
    TUD_HID_DESCRIPTOR(ITF_NUM_HID, 0, HID_ITF_PROTOCOL_KEYBOARD,
                       sizeof(hid_report_descriptor), EPNUM_HID, 16, 1),
    TUD_CDC_NCM_DESCRIPTOR(ITF_NUM_NCM, STRID_NETWORK, STRID_MAC,
                           EPNUM_NET_NOTIF, 64, EPNUM_NET_OUT, EPNUM_NET_IN,
                           CFG_TUD_NET_ENDPOINT_SIZE, CFG_TUD_NET_MTU),
};

const uint8_t *tud_descriptor_configuration_cb(uint8_t index) {
    return index == 0 ? configuration_descriptor : NULL;
}

#define BOS_TOTAL_LEN (TUD_BOS_DESC_LEN + TUD_BOS_MICROSOFT_OS_DESC_LEN)
#define MS_OS_20_DESC_LEN 0xB2

static const uint8_t bos_descriptor[] = {
    TUD_BOS_DESCRIPTOR(BOS_TOTAL_LEN, 1),
    TUD_BOS_MS_OS_20_DESCRIPTOR(MS_OS_20_DESC_LEN, 1),
};

const uint8_t *tud_descriptor_bos_cb(void) { return bos_descriptor; }

static const uint8_t ms_os_20_descriptor[] = {
    U16_TO_U8S_LE(0x000A), U16_TO_U8S_LE(MS_OS_20_SET_HEADER_DESCRIPTOR),
    U32_TO_U8S_LE(0x06030000), U16_TO_U8S_LE(MS_OS_20_DESC_LEN),
    U16_TO_U8S_LE(0x0008), U16_TO_U8S_LE(MS_OS_20_SUBSET_HEADER_CONFIGURATION),
    0, 0, U16_TO_U8S_LE(MS_OS_20_DESC_LEN - 0x0A),
    U16_TO_U8S_LE(0x0008), U16_TO_U8S_LE(MS_OS_20_SUBSET_HEADER_FUNCTION),
    ITF_NUM_NCM, 0, U16_TO_U8S_LE(MS_OS_20_DESC_LEN - 0x0A - 0x08),
    U16_TO_U8S_LE(0x0014), U16_TO_U8S_LE(MS_OS_20_FEATURE_COMPATBLE_ID),
    'W', 'I', 'N', 'N', 'C', 'M', 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    U16_TO_U8S_LE(MS_OS_20_DESC_LEN - 0x0A - 0x08 - 0x08 - 0x14),
    U16_TO_U8S_LE(MS_OS_20_FEATURE_REG_PROPERTY),
    U16_TO_U8S_LE(0x0007), U16_TO_U8S_LE(0x002A),
    'D',0,'e',0,'v',0,'i',0,'c',0,'e',0,'I',0,'n',0,'t',0,'e',0,
    'r',0,'f',0,'a',0,'c',0,'e',0,'G',0,'U',0,'I',0,'D',0,'s',0,0,0,
    U16_TO_U8S_LE(0x0050),
    '{',0,'6',0,'B',0,'4',0,'3',0,'1',0,'D',0,'2',0,'E',0,'-',0,
    '4',0,'D',0,'3',0,'5',0,'-',0,'4',0,'A',0,'1',0,'F',0,'-',0,
    '9',0,'4',0,'B',0,'2',0,'-',0,'8',0,'5',0,'3',0,'A',0,'C',0,
    '9',0,'E',0,'1',0,'0',0,'7',0,'8',0,'C',0,'}',0,0,0,0,0,
};

TU_VERIFY_STATIC(sizeof(ms_os_20_descriptor) == MS_OS_20_DESC_LEN,
                 "Incorrect MS OS descriptor size");

bool tud_vendor_control_xfer_cb(uint8_t rhport, uint8_t stage,
                                const tusb_control_request_t *request) {
    if (stage != CONTROL_STAGE_SETUP) return true;
    if (request->bmRequestType_bit.type == TUSB_REQ_TYPE_VENDOR &&
        request->bRequest == 1 && request->wIndex == 7) {
        return tud_control_xfer(rhport, request,
                                (void *)(uintptr_t)ms_os_20_descriptor,
                                sizeof(ms_os_20_descriptor));
    }
    return false;
}

static const char *string_descriptors[] = {
    (const char[]){0x09, 0x04},
    "d3cker",
    "PicoDeck ED Keyboard",
    NULL,
    "PicoDeck Keyboard Network",
};

static uint16_t string_descriptor_buffer[33];

const uint16_t *tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
    (void)langid;
    unsigned count = 0;
    if (index == STRID_LANGID) {
        memcpy(&string_descriptor_buffer[1], string_descriptors[0], 2);
        count = 1;
    } else if (index == STRID_SERIAL) {
        count = board_usb_get_serial(string_descriptor_buffer + 1, 32);
    } else if (index == STRID_MAC) {
        for (unsigned i = 0; i < sizeof(tud_network_mac_address); ++i) {
            string_descriptor_buffer[1 + count++] =
                "0123456789ABCDEF"[tud_network_mac_address[i] >> 4];
            string_descriptor_buffer[1 + count++] =
                "0123456789ABCDEF"[tud_network_mac_address[i] & 0x0f];
        }
    } else {
        if (index >= sizeof(string_descriptors) / sizeof(string_descriptors[0]) ||
            !string_descriptors[index]) return NULL;
        const char *text = string_descriptors[index];
        count = strlen(text);
        if (count > 32) count = 32;
        for (unsigned i = 0; i < count; ++i)
            string_descriptor_buffer[1 + i] = (uint8_t)text[i];
    }
    string_descriptor_buffer[0] =
        (uint16_t)((TUSB_DESC_STRING << 8) | (2 * count + 2));
    return string_descriptor_buffer;
}

uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id,
                               hid_report_type_t report_type, uint8_t *buffer,
                               uint16_t requested_length) {
    (void)instance; (void)report_id; (void)report_type;
    (void)buffer; (void)requested_length;
    return 0;
}

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id,
                           hid_report_type_t report_type,
                           const uint8_t *buffer, uint16_t buffer_size) {
    (void)instance; (void)report_id; (void)report_type;
    (void)buffer; (void)buffer_size;
}

