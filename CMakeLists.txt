# == DO NEVER EDIT THE NEXT LINES for Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
   set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(PICO_SDK_PATH ${USERHOME}/.pico-sdk/sdk/1.5.1)
set(PICO_TOOLCHAIN_PATH ${USERHOME}/.pico-sdk/toolchain/13_2_Rel1)
if(WIN32)
    set(pico-sdk-tools_DIR ${USERHOME}/.pico-sdk/tools/1.5.1)
    include(${pico-sdk-tools_DIR}/pico-sdk-tools-config.cmake)
    include(${pico-sdk-tools_DIR}/pico-sdk-tools-config-version.cmake)
endif()
# ====================================================================================
cmake_minimum_required(VERSION 3.12)

include(pico_sdk_import.cmake)

set(CMAKE_C_STANDARD 11)

#main application
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
target_link_libraries(picorx pico_stdlib pico_multicore hardware_clocks hardware_flash hardware_pio hardware_adc hardware_pwm hardware_dma hardware_i2c)

#battery check utility
project(battery_check)
add_executable(battery_check
    battery_check.cpp
)
pico_add_extra_outputs(battery_check)
pico_enable_stdio_usb(battery_check 1)
pico_enable_stdio_uart(battery_check 0)
target_link_libraries(battery_check pico_stdlib hardware_adc)
