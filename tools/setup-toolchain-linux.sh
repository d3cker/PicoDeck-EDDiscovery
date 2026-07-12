#!/usr/bin/env bash
set -euo pipefail

repository_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
downloads="$repository_root/.downloads-linux"
toolchain_root="$repository_root/.toolchain-linux"
sdk_root="$toolchain_root/sdk/pico-sdk-2.2.0"

required_commands=(curl sha256sum tar unzip)
for command_name in "${required_commands[@]}"; do
    if ! command -v "$command_name" >/dev/null 2>&1; then
        echo "Missing required command: $command_name" >&2
        exit 1
    fi
done

mkdir -p "$downloads" "$toolchain_root" "$toolchain_root/sdk"

download() {
    local name="$1"
    local url="$2"
    local sha256="$3"
    local destination="$downloads/$name"

    if [[ ! -f "$destination" ]]; then
        echo "Downloading $name"
        curl --location --fail --retry 3 --output "$destination" "$url"
    fi

    if ! echo "$sha256  $destination" | sha256sum --check --status; then
        echo "SHA-256 mismatch for $destination" >&2
        echo "Delete the file and run setup again." >&2
        exit 1
    fi
}

download \
    "pico-sdk-2.2.0.tar.gz" \
    "https://github.com/raspberrypi/pico-sdk/releases/download/2.2.0/pico-sdk-2.2.0.tar.gz" \
    "2678fe2b176cf64a7f71cd91749fdf9134c8cf7ff84b7199dfe5ea0d6dba6fa4"
download \
    "tinyusb-86ad6e56.zip" \
    "https://github.com/hathach/tinyusb/archive/86ad6e56c1700e85f1c5678607a762cfe3aa2f47.zip" \
    "3011c90c128988012b553e5d2f0a90bc0b64046591c964bc1f9f6659edcd7e4b"
download \
    "lwip-77dcd25a.zip" \
    "https://github.com/lwip-tcpip/lwip/archive/77dcd25a72509eb83f72b033d219b1d40cd8eb95.zip" \
    "e09bf3518f1d23204cdd38203e1bdbbb5c634a1dce8dde487b7eaca3f2407245"
download \
    "arm-gnu-toolchain-14.3.rel1-x86_64-arm-none-eabi.tar.xz" \
    "https://armkeil.blob.core.windows.net/developer/Files/downloads/gnu/14.3.rel1/binrel/arm-gnu-toolchain-14.3.rel1-x86_64-arm-none-eabi.tar.xz" \
    "8f6903f8ceb084d9227b9ef991490413014d991874a1e34074443c2a72b14dbd"

gcc_root="$toolchain_root/arm-gnu-toolchain-14.3.rel1-x86_64-arm-none-eabi"
if [[ ! -x "$gcc_root/bin/arm-none-eabi-gcc" ]]; then
    tar -xJf \
        "$downloads/arm-gnu-toolchain-14.3.rel1-x86_64-arm-none-eabi.tar.xz" \
        -C "$toolchain_root"
fi

if [[ ! -f "$sdk_root/pico_sdk_init.cmake" ]]; then
    tar -xzf "$downloads/pico-sdk-2.2.0.tar.gz" -C "$toolchain_root/sdk"
fi

tinyusb_root="$sdk_root/lib/tinyusb"
if [[ ! -f "$tinyusb_root/src/tusb.c" ]]; then
    unzip -q "$downloads/tinyusb-86ad6e56.zip" -d "$sdk_root/lib"
    mv \
        "$sdk_root/lib/tinyusb-86ad6e56c1700e85f1c5678607a762cfe3aa2f47" \
        "$tinyusb_root"
fi

lwip_root="$sdk_root/lib/lwip"
if [[ ! -f "$lwip_root/src/include/lwip/init.h" ]]; then
    unzip -q "$downloads/lwip-77dcd25a.zip" -d "$sdk_root/lib"
    mv \
        "$sdk_root/lib/lwip-77dcd25a72509eb83f72b033d219b1d40cd8eb95" \
        "$lwip_root"
fi

echo
echo "Linux RP2040 toolchain ready in $toolchain_root"
