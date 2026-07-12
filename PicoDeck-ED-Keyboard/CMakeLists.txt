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

include(${CMAKE_CURRENT_LIST_DIR}/../common/picodeck_common.cmake)

add_executable(picodeck
    src/main.c
    src/lcd.c
    src/touch.c
    src/font5x7.c
    src/buttons.c
    src/edd_buttons.c
    src/ui.c
    src/usb_descriptors.c
    assets/icons.c
)

picodeck_add_common(picodeck 8192 128)

target_include_directories(picodeck PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/src
    ${CMAKE_CURRENT_LIST_DIR}/assets
)

target_compile_options(picodeck PRIVATE
    -Wall
    -Wextra
    -Werror=return-type
)

target_link_libraries(picodeck PRIVATE
    pico_stdlib
    hardware_spi
)

pico_enable_stdio_usb(picodeck 0)
pico_enable_stdio_uart(picodeck 0)
pico_set_program_name(picodeck "PicoDeck ED Keyboard")
pico_set_program_version(picodeck "0.5.0")
pico_add_extra_outputs(picodeck)
