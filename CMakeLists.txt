cmake_minimum_required(VERSION 3.12)
set(PICO_SDK_PATH $ENV{PICO_SDK_PATH})

include(pico_sdk_import.cmake)

project(hello)

pico_sdk_init()

add_executable(hello
    receiver.cpp
    half_band_filter.cpp
)

pico_generate_pio_header(hello ${CMAKE_CURRENT_LIST_DIR}/hello.pio)

pico_add_extra_outputs(hello)

# enable usb output, disable uart output
pico_enable_stdio_usb(hello 1)
pico_enable_stdio_uart(hello 0)


target_link_libraries(hello pico_stdlib hardware_pio hardware_adc hardware_dma)
