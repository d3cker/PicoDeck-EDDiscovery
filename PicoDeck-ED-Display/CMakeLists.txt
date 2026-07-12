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

project(picodeck_ed_display C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

include(${CMAKE_CURRENT_LIST_DIR}/../common/picodeck_common.cmake)

add_executable(picodeck_ed_display
    src/main.c
    src/lcd.c
    src/font5x7.c
    src/edd_state.c
    src/dashboard.c
    src/usb_descriptors.c
)

picodeck_add_common(picodeck_ed_display 32768 256)

target_include_directories(picodeck_ed_display PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/src
    ${CMAKE_CURRENT_LIST_DIR}/third_party/jsmn
)

target_compile_options(picodeck_ed_display PRIVATE
    -Wall
    -Wextra
    -Werror=return-type
    -Wno-sign-compare
)

target_link_libraries(picodeck_ed_display PRIVATE
    pico_stdlib
    hardware_spi
    hardware_pwm
)

pico_enable_stdio_usb(picodeck_ed_display 0)
pico_enable_stdio_uart(picodeck_ed_display 1)
pico_set_program_name(picodeck_ed_display "PicoDeck ED Display")
pico_set_program_version(picodeck_ed_display "0.1.6")
pico_add_extra_outputs(picodeck_ed_display)
