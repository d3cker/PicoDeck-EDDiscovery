#!/usr/bin/env bash
set -euo pipefail

configuration="${1:-Release}"
if [[ "$configuration" != "Release" && "$configuration" != "Debug" ]]; then
    echo "Usage: $0 [Release|Debug]" >&2
    exit 2
fi

repository_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
toolchain_root="$repository_root/.toolchain-linux"
sdk_root="$toolchain_root/sdk/pico-sdk-2.2.0"
gcc_root="$toolchain_root/arm-gnu-toolchain-14.3.rel1-x86_64-arm-none-eabi"

required_commands=(cmake ninja python3 git)
for command_name in "${required_commands[@]}"; do
    if ! command -v "$command_name" >/dev/null 2>&1; then
        echo "Missing required command: $command_name" >&2
        exit 1
    fi
done

required_paths=(
    "$sdk_root/pico_sdk_init.cmake"
    "$sdk_root/lib/tinyusb/src/tusb.c"
    "$sdk_root/lib/lwip/src/include/lwip/init.h"
    "$gcc_root/bin/arm-none-eabi-gcc"
)
for path in "${required_paths[@]}"; do
    if [[ ! -e "$path" ]]; then
        echo "Missing build dependency: $path" >&2
        echo "Run tools/setup-toolchain-linux.sh first." >&2
        exit 1
    fi
done

export PICO_SDK_PATH="$sdk_root"
export PICO_TOOLCHAIN_PATH="$gcc_root"
export PICOTOOL_FETCH_FROM_GIT_PATH="$toolchain_root/picotool"
export PICOTOOL_GIT_BRANCH="2.2.0"
export PATH="$gcc_root/bin:$PATH"

build_project() {
    local project="$1"
    local build_directory="$2"

    echo
    echo "Building $project [$configuration]"
    cmake \
        -S "$repository_root/$project" \
        -B "$build_directory" \
        -G Ninja \
        -DCMAKE_BUILD_TYPE="$configuration" \
        -DPICO_BOARD=pico \
        -DPICO_SDK_PATH="$sdk_root" \
        -DPICO_TOOLCHAIN_PATH="$gcc_root" \
        -DPython3_EXECUTABLE="$(command -v python3)"
    cmake --build "$build_directory"
}

display_build="$repository_root/PicoDeck-ED-Display/build-linux-rp2040"
keyboard_build="$repository_root/PicoDeck-ED-Keyboard/build-linux-rp2040"
build_project "PicoDeck-ED-Display" "$display_build"
build_project "PicoDeck-ED-Keyboard" "$keyboard_build"

display_uf2="$display_build/picodeck_ed_display.uf2"
keyboard_uf2="$keyboard_build/picodeck.uf2"
for firmware in "$display_uf2" "$keyboard_uf2"; do
    if [[ ! -s "$firmware" ]]; then
        echo "Expected firmware was not produced: $firmware" >&2
        exit 1
    fi
done

dist="$repository_root/dist"
mkdir -p "$dist"
cp "$display_uf2" "$dist/PicoDeck-ED-Display.uf2"
cp "$keyboard_uf2" "$dist/PicoDeck-ED-Keyboard.uf2"
cp "$repository_root/LICENSE" "$dist/LICENSE"
cp "$repository_root/THIRD_PARTY_NOTICES.md" "$dist/THIRD_PARTY_NOTICES.md"
cp \
    "$repository_root/PicoDeck-ED-Keyboard/assets/UPSTREAM_LICENSE.md" \
    "$dist/ICON_UPSTREAM_LICENSE.md"

(
    cd "$dist"
    sha256sum PicoDeck-ED-Display.uf2 PicoDeck-ED-Keyboard.uf2 \
        > SHA256SUMS.txt
)

echo
echo "Both firmware images and release notices are ready in $dist"
