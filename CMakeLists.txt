cmake_minimum_required(VERSION 3.12)

include(pico_sdk_import.cmake)

set(CMAKE_C_STANDARD 11)

project(picorx)

pico_sdk_init()

add_executable(picorx
    ssd1306.c
    picorx.cpp
    rx.cpp
    rx_dsp.cpp
    half_band_filter.cpp
    half_band_filter2.cpp
    ui.cpp
)

pico_generate_pio_header(picorx ${CMAKE_CURRENT_LIST_DIR}/nco.pio)
pico_generate_pio_header(picorx ${CMAKE_CURRENT_LIST_DIR}/quadrature_encoder.pio)


pico_add_extra_outputs(picorx)

# enable usb output, disable uart output
pico_enable_stdio_usb(picorx 1)
pico_enable_stdio_uart(picorx 0)


target_link_libraries(picorx pico_stdlib pico_multicore hardware_flash hardware_pio hardware_adc hardware_pwm hardware_dma hardware_i2c)
