cmake_minimum_required(VERSION 3.12)
set(FETCHCONTENT_QUIET FALSE)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# If an SDK path has not been configured in the environemnt, use CMake's
# FetchContent to pull down the SDK sources from GitHub.
if(NOT DEFINED ENV{PICO_SDK_PATH})
    set(ENV{PICO_SDK_FETCH_FROM_GIT} "1")
    set(ENV{PICO_SDK_FETCH_FROM_GIT_TAG} "2.2.0")
    set(ENV{PICOTOOL_FORCE_FETCH_FROM_GIT} "1")
    set(ENV{PICOTOOL_FETCH_FROM_GIT_PATH} "${CMAKE_BINARY_DIR}/picotool-build")
endif()

include(pico_sdk_import.cmake)

project(picorx
LANGUAGES
    C
    CXX
VERSION
    0.0.1
)

pico_sdk_init()

set(CMAKE_C_STANDARD 11)

add_compile_options(-Wall -Werror -fdata-sections -ffunction-sections)
add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>)
add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>)

file(GLOB_RECURSE U8G2_SRCS
    CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_LIST_DIR}/external/u8g2/csrc/*.c"
)

file(GLOB_RECURSE U8G2_HDRS
    CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_LIST_DIR}/external/u8g2/csrc/*.c"
)

add_library(u8g2)
target_sources(u8g2 PRIVATE ${U8G2_SRCS} ${U8G2_HDRS})
target_compile_options(u8g2 PRIVATE -Wno-unused-function -Wno-unused-variable)
target_include_directories(u8g2 PUBLIC ${CMAKE_CURRENT_LIST_DIR}/external/u8g2/csrc/)

set(PICORX_SRCS
    ${CMAKE_CURRENT_LIST_DIR}/ssd1306.c
    ${CMAKE_CURRENT_LIST_DIR}/picorx.cpp
    ${CMAKE_CURRENT_LIST_DIR}/nco.cpp
    ${CMAKE_CURRENT_LIST_DIR}/rx.cpp
    ${CMAKE_CURRENT_LIST_DIR}/rx_dsp.cpp
    ${CMAKE_CURRENT_LIST_DIR}/fft.cpp
    ${CMAKE_CURRENT_LIST_DIR}/fft_filter.cpp
    ${CMAKE_CURRENT_LIST_DIR}/cic_corrections.cpp
    ${CMAKE_CURRENT_LIST_DIR}/ui.cpp
    ${CMAKE_CURRENT_LIST_DIR}/utils.cpp
    ${CMAKE_CURRENT_LIST_DIR}/ili934x.cpp
    ${CMAKE_CURRENT_LIST_DIR}/waterfall.cpp
    ${CMAKE_CURRENT_LIST_DIR}/button.cpp
    ${CMAKE_CURRENT_LIST_DIR}/cat.cpp
    ${CMAKE_CURRENT_LIST_DIR}/noise_canceler.cpp
    ${CMAKE_CURRENT_LIST_DIR}/pwm_audio_sink.cpp
    ${CMAKE_CURRENT_LIST_DIR}/usb_descriptors.c
    ${CMAKE_CURRENT_LIST_DIR}/usb_audio_device.c
    ${CMAKE_CURRENT_LIST_DIR}/ring_buffer_lib.c
)

set(PICORX_LIBS
    pico_stdlib
    pico_multicore
    hardware_clocks
    hardware_flash
    hardware_pio
    hardware_adc
    hardware_pwm
    hardware_dma
    hardware_i2c
    hardware_spi
    tinyusb_device
    tinyusb_board
    u8g2
)

if(BUTTON_ENCODER)
    add_definitions(-DBUTTON_ENCODER)
    list(APPEND PICORX_SRCS ${CMAKE_CURRENT_LIST_DIR}/button_encoder.cpp)
else()
    list(APPEND PICORX_SRCS ${CMAKE_CURRENT_LIST_DIR}/rotary_encoder.cpp)
endif()

if(PICO_BOARD STREQUAL "pico")

    #main application
    add_executable(picorx ${PICORX_SRCS})
    target_compile_features(picorx PUBLIC c_std_11)
    pico_generate_pio_header(picorx ${CMAKE_CURRENT_LIST_DIR}/nco.pio)
    pico_generate_pio_header(picorx ${CMAKE_CURRENT_LIST_DIR}/quadrature_encoder.pio)
    pico_add_extra_outputs(picorx)
    pico_enable_stdio_usb(picorx 1)
    pico_enable_stdio_uart(picorx 0)
    target_include_directories(picorx PRIVATE ${CMAKE_CURRENT_LIST_DIR})
    target_link_libraries(picorx PRIVATE ${PICORX_LIBS})
    target_compile_definitions(picorx PUBLIC PICO_XOSC_STARTUP_DELAY_MULTIPLIER=128)
    set_target_properties(picorx PROPERTIES SUFFIX ".elf")

    #battery check utility
    add_executable(battery_check)
    target_compile_features(battery_check PUBLIC c_std_11)
    target_sources(battery_check PRIVATE ${CMAKE_CURRENT_LIST_DIR}/battery_check.cpp)
    pico_add_extra_outputs(battery_check)
    pico_enable_stdio_usb(battery_check 1)
    pico_enable_stdio_uart(battery_check 0)
    target_link_libraries(battery_check pico_stdlib hardware_adc)
    set_target_properties(battery_check PROPERTIES SUFFIX ".elf")

elseif(PICO_BOARD STREQUAL "pico2")

    if(PICO_PLATFORM STREQUAL "rp2350-riscv")

        #main application
        add_executable(pico2rx-riscv ${PICORX_SRCS})
        target_compile_features(pico2rx-risc PUBLIC c_std_11)
        pico_generate_pio_header(pico2rx-riscv ${CMAKE_CURRENT_LIST_DIR}/nco.pio)
        pico_generate_pio_header(pico2rx-riscv ${CMAKE_CURRENT_LIST_DIR}/quadrature_encoder.pio)
        pico_add_extra_outputs(pico2rx-riscv)
        # enable usb output, disable uart output
        pico_enable_stdio_usb(pico2rx-riscv 1)
        pico_enable_stdio_uart(pico2rx-riscv 0)
        target_include_directories(pico2rx-riscv PRIVATE ${CMAKE_CURRENT_LIST_DIR})
        target_link_libraries(pico2rx-riscv PRIVATE ${PICORX_LIBS})
        target_compile_definitions(pico2rx-riscv PUBLIC PICO_XOSC_STARTUP_DELAY_MULTIPLIER=128)
        set_target_properties(pico2rx-riscv PROPERTIES SUFFIX ".elf")

        #battery check utility
        add_executable(battery_check_pico2-riscv)
        target_compile_features(battery_check_pico2-riscv PUBLIC c_std_11)
        target_sources(battery_check_pico2-riscv PRIVATE ${CMAKE_CURRENT_LIST_DIR}/battery_check.cpp)
        pico_add_extra_outputs(battery_check_pico2-riscv)
        pico_enable_stdio_usb(battery_check_pico2-riscv 1)
        pico_enable_stdio_uart(battery_check_pico2-riscv 0)
        target_link_libraries(battery_check_pico2-riscv pico_stdlib hardware_adc)
        set_target_properties(battery_check_pico2-riscv PROPERTIES SUFFIX ".elf")

    else()

        #main application
        add_executable(pico2rx ${PICORX_SRCS})
        target_compile_features(pico2rx PUBLIC c_std_11)
        pico_generate_pio_header(pico2rx ${CMAKE_CURRENT_LIST_DIR}/nco.pio)
        pico_generate_pio_header(pico2rx ${CMAKE_CURRENT_LIST_DIR}/quadrature_encoder.pio)
        pico_add_extra_outputs(pico2rx)
        pico_enable_stdio_usb(pico2rx 1)
        pico_enable_stdio_uart(pico2rx 0)
        target_include_directories(pico2rx PRIVATE ${CMAKE_CURRENT_LIST_DIR})
        target_link_libraries(pico2rx PRIVATE ${PICORX_LIBS})
        target_compile_definitions(pico2rx PUBLIC PICO_XOSC_STARTUP_DELAY_MULTIPLIER=128)
        set_target_properties(pico2rx PROPERTIES SUFFIX ".elf")

        #battery check utility
        add_executable(battery_check_pico2)
        target_compile_features(battery_check_pico2 PUBLIC c_std_11)
        target_sources(battery_check_pico2 PRIVATE battery_check.cpp)
        pico_add_extra_outputs(battery_check_pico2)
        pico_enable_stdio_usb(battery_check_pico2 1)
        pico_enable_stdio_uart(battery_check_pico2 0)
        target_link_libraries(battery_check_pico2 pico_stdlib hardware_adc)
        set_target_properties(battery_check_pico2 PROPERTIES SUFFIX ".elf")

    endif()
endif()
