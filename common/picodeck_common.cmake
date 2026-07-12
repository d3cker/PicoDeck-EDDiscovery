set(PICODECK_COMMON_DIR "${CMAKE_CURRENT_LIST_DIR}")
set(PICODECK_TINYUSB_NETWORKING
    "${PICO_SDK_PATH}/lib/tinyusb/lib/networking")

function(picodeck_add_common target websocket_rx_capacity websocket_chunk_size)
    target_sources(${target} PRIVATE
        "${PICODECK_COMMON_DIR}/net_usb.c"
        "${PICODECK_COMMON_DIR}/websocket_client.c"
        "${PICODECK_COMMON_DIR}/eddjson_client.c"
        "${PICODECK_TINYUSB_NETWORKING}/dhserver.c"
    )

    target_include_directories(${target} PRIVATE
        "${PICODECK_COMMON_DIR}"
        "${PICODECK_TINYUSB_NETWORKING}"
    )

    target_compile_definitions(${target} PRIVATE
        WEBSOCKET_CLIENT_RX_CAPACITY=${websocket_rx_capacity}
        WEBSOCKET_CLIENT_SEND_CHUNK_SIZE=${websocket_chunk_size}
    )

    target_link_libraries(${target} PRIVATE
        pico_rand
        pico_unique_id
        pico_lwip_nosys
        tinyusb_device
        tinyusb_board
    )
endfunction()
