# Third-party notices

The repository-level [MIT License](LICENSE) applies to the original
PicoDeck-EDDiscovery source code and documentation. It does not replace the
licenses of the third-party material listed below.

## Elite Dangerous icon artwork

The generated icon masks in
`PicoDeck-ED-Keyboard/assets/icons.c`, including copies embedded in
`PicoDeck-ED-Keyboard.uf2`, are derived from the **Sidewinder Orange** theme in
[`streamdeck-elite-icons`](https://github.com/Ordo-Corona-Stellarum/streamdeck-elite-icons)
at commit `d2f58e41d86374dfc67efe15098d06f021d4cb79`.

- original artwork: Copyright (C) 2020 Keath Milligan
- license: [Creative Commons Attribution-ShareAlike 3.0 Unported](https://creativecommons.org/licenses/by-sa/3.0/)
- changes: the source PNG artwork was resized and converted into four-level
  alpha masks for rendering by the RP2040 firmware
- upstream notice in the source tree:
  `PicoDeck-ED-Keyboard/assets/UPSTREAM_LICENSE.md` (copied as
  `ICON_UPSTREAM_LICENSE.md` in Keyboard release bundles)

The generated masks remain available under CC BY-SA 3.0. When redistributing
the Keyboard source or a Keyboard UF2 containing these masks, preserve the
attribution, the indication of changes, and the CC BY-SA 3.0 license link or
license text. The rest of the PicoDeck-EDDiscovery code is not relicensed under
CC BY-SA merely because it is distributed alongside the icon assets.

The upstream repository separately licenses its software under GPLv3 or later.
PicoDeck-EDDiscovery does not copy or compile that upstream software; it uses
only the image artwork and the generated masks derived from it.

## jsmn

PicoDeck-ED-Display vendors the jsmn JSON parser, Copyright (c) 2010 Serge A.
Zaitsev, under the MIT License. Its complete license is stored in
`PicoDeck-ED-Display/third_party/jsmn/LICENSE`.

## Raspberry Pi Pico SDK

The firmware is built with the Raspberry Pi Pico SDK, Copyright 2020 Raspberry
Pi (Trading) Ltd., under its BSD 3-Clause license. SDK components may carry
their own notices; refer to the licenses supplied with the pinned SDK source.

## TinyUSB

The firmware is built with TinyUSB, Copyright (c) 2018 hathach (tinyusb.org),
under the MIT License. Refer to `lib/tinyusb/LICENSE` in the pinned Pico SDK
source installed by `tools/setup-toolchain.ps1`.

## lwIP

The firmware is built with lwIP, Copyright (c) 2001, 2002 Swedish Institute of
Computer Science, under its BSD-style 3-Clause license. Refer to
`lib/lwip/COPYING` in the pinned Pico SDK source installed by
`tools/setup-toolchain.ps1`.
