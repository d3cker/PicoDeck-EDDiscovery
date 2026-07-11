cmake_minimum_required(VERSION 3.13)

set(PICO_BOARD pico CACHE STRING "Target board")

if(NOT DEFINED PICO_SDK_PATH)
    set(PICO_SDK_PATH "${CMAKE_CURRENT_LIST_DIR}/.toolchain/sdk/pico-sdk-2.2.0")
    if(NOT EXISTS "${PICO_SDK_PATH}")
        set(PICO_SDK_PATH "${CMAKE_CURRENT_LIST_DIR}/../.toolchain/sdk/pico-sdk-2.2.0")
    endif()
    if(NOT EXISTS "${PICO_SDK_PATH}")
        set(PICO_SDK_PATH "${CMAKE_CURRENT_LIST_DIR}/../../.toolchain/sdk/pico-sdk-2.2.0")
    endif()
endif()

include(${PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(picodeck C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

set(TINYUSB_NETWORKING ${PICO_SDK_PATH}/lib/tinyusb/lib/networking)

add_executable(picodeck
    src/main.c
    src/lcd.c
    src/touch.c
    src/font5x7.c
    src/buttons.c
    src/edd_buttons.c
    src/net_usb.c
    src/ui.c
    src/usb_descriptors.c
    src/websocket_client.c
    assets/icons.c
    ${TINYUSB_NETWORKING}/dhserver.c
)

target_include_directories(picodeck PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/src
    ${CMAKE_CURRENT_LIST_DIR}/assets
    ${TINYUSB_NETWORKING}
)

target_compile_options(picodeck PRIVATE
    -Wall
    -Wextra
    -Werror=return-type
)

target_link_libraries(picodeck PRIVATE
    pico_stdlib
    pico_rand
    pico_unique_id
    pico_lwip_nosys
    hardware_spi
    tinyusb_device
    tinyusb_board
)

pico_enable_stdio_usb(picodeck 0)
pico_enable_stdio_uart(picodeck 0)
pico_set_program_name(picodeck "PicoDeck ED Keyboard")
pico_set_program_version(picodeck "0.5.0")
pico_add_extra_outputs(picodeck)
