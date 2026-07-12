# Shared firmware components

This directory is compiled into both PicoDeck firmware targets. It owns the
transport and USB-network behavior that must remain consistent across devices;
it does not contain display, touch, HID, or game-state presentation logic.

## Layers

- `net_usb`: configurable TinyUSB NCM netif, lwIP frame exchange, and one-client
  DHCP server
- `websocket_client`: reusable RFC 6455 client over the lwIP raw TCP API
- `eddjson_client`: EDDDiscovery's `EDDJSON` subprotocol and request scheduling
- `lwipopts.h`: shared lwIP configuration
- `picodeck_common.cmake`: common source and dependency integration

`picodeck_add_common(target, rx_capacity, send_chunk_size)` compiles the shared
sources directly into a firmware target. Compiling per target is intentional:
Display retains its 32 KiB receive buffers while Keyboard retains its smaller
8 KiB buffers.

Each application creates a `net_usb_config_t` and an
`eddjson_client_config_t`. This keeps device-specific subnets, interface names,
WebSocket identity, and EDDJSON request sets out of the shared implementation.
The application receives complete EDDJSON messages through a callback and owns
all parsing and state updates.

USB descriptors are not shared. Display is an NCM-only USB device; Keyboard is
a composite HID plus NCM device and therefore requires a different descriptor
tree and TinyUSB configuration.
