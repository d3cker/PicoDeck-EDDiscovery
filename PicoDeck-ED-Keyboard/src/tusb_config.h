#pragma once

#include "lwipopts.h"

#ifndef CFG_TUSB_MCU
#error CFG_TUSB_MCU must be defined
#endif
#ifndef CFG_TUSB_OS
#define CFG_TUSB_OS OPT_OS_NONE
#endif
#ifndef BOARD_TUD_RHPORT
#define BOARD_TUD_RHPORT 0
#endif
#define CFG_TUSB_DEBUG 0
#define CFG_TUD_ENABLED 1
#define CFG_TUD_MAX_SPEED OPT_MODE_DEFAULT_SPEED
#define CFG_TUSB_MEM_SECTION
#define CFG_TUSB_MEM_ALIGN __attribute__((aligned(4)))
#define CFG_TUD_ENDPOINT0_SIZE 64

#define CFG_TUD_CDC 0
#define CFG_TUD_MSC 0
#define CFG_TUD_HID 1
#define CFG_TUD_HID_EP_BUFSIZE 16
#define CFG_TUD_MIDI 0
#define CFG_TUD_VENDOR 0
#define CFG_TUD_ECM_RNDIS 0
#define CFG_TUD_NCM 1

#define CFG_TUD_NCM_IN_NTB_MAX_SIZE 2048
#define CFG_TUD_NCM_OUT_NTB_MAX_SIZE 2048
#define CFG_TUD_NCM_OUT_NTB_N 1
#define CFG_TUD_NCM_IN_NTB_N 1
#define CFG_TUD_NCM_IN_MAX_DATAGRAMS_PER_NTB 2
#define CFG_TUD_NCM_OUT_MAX_DATAGRAMS_PER_NTB 2
