cmake_minimum_required(VERSION 3.12)
set(FETCHCONTENT_QUIET FALSE)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# If an SDK path has not been configured in the environemnt, use CMake's
# FetchContent to pull down the SDK sources from GitHub.
if(NOT DEFINED ENV{PICO_SDK_PATH})
    set(ENV{PICO_SDK_FETCH_FROM_GIT} "1")
    set(ENV{PICO_SDK_FETCH_FROM_GIT_TAG} "2.1.0")
    set(ENV{PICOTOOL_FORCE_FETCH_FROM_GIT} "1")
    set(ENV{PICOTOOL_FETCH_FROM_GIT_PATH} "${CMAKE_BINARY_DIR}/picotool-build")
endif()

# Ensure GNU size tool is defined
find_program(CMAKE_SIZE arm-none-eabi-size)
if (NOT CMAKE_SIZE)
    message(FATAL_ERROR "arm-none-eabi-size not found — is the ARM toolchain in your PATH?")
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

add_compile_options(-Wall -Werror -Wextra -Wshadow -Wstack-usage=2048
#add_compile_options(-Wall -Werror -Wextra -Wshadow
                    -fdata-sections -ffunction-sections -Wno-missing-field-initializers)
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
    ${CMAKE_CURRENT_LIST_DIR}/src/ssd1306.c
    ${CMAKE_CURRENT_LIST_DIR}/src/picorx.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/nco.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/xcvr.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/rx_dsp.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/fft.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/fft_filter.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/noise_reduction.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/cic_corrections.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/ui.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/settings.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/memory.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/autosave_memory.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/utils.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/ili934x.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/waterfall.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/button.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/quadrature_si5351.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/cat.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/pwm_audio_sink.cpp
    ${CMAKE_CURRENT_LIST_DIR}/usb_descriptors.c
    ${CMAKE_CURRENT_LIST_DIR}/usb_audio_device.c
    ${CMAKE_CURRENT_LIST_DIR}/src/ring_buffer_lib.c
    ${CMAKE_CURRENT_LIST_DIR}/src/rotary_encoder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/codecs/sstv_decoder.cpp
    ${CMAKE_CURRENT_LIST_DIR}/codecs/sstv_decoder_picorx.cpp
    ${CMAKE_CURRENT_LIST_DIR}/eibi/eibi.cpp
    ${CMAKE_CURRENT_LIST_DIR}/eibi/spotter.cpp
)

set(PICORX_LIBS
    pico_stdlib
    pico_multicore
    pico_rand
    hardware_clocks
    hardware_flash
    hardware_pio
    hardware_adc
    hardware_pwm
    hardware_dma
    hardware_i2c
    hardware_spi
    hardware_exception
    tinyusb_device
    tinyusb_board
    u8g2
)

if(BUTTON_ENCODER)
    add_definitions(-DBUTTON_ENCODER)
    list(APPEND PICORX_SRCS ${CMAKE_CURRENT_LIST_DIR}/src/button_encoder.cpp)
else()
    list(APPEND PICORX_SRCS ${CMAKE_CURRENT_LIST_DIR}/src/rotary_encoder.cpp)
endif()

if(PICO_BOARD STREQUAL "pico")

    list(APPEND PICORX_SRCS src/clocks_pico.cpp)
    list(APPEND PICORX_SRCS transmit/cordic.cpp)
    #main application
    add_executable(picorx ${PICORX_SRCS})
    target_compile_features(picorx PUBLIC c_std_11)
    pico_generate_pio_header(picorx ${CMAKE_CURRENT_LIST_DIR}/src/nco.pio)
    pico_generate_pio_header(picorx ${CMAKE_CURRENT_LIST_DIR}/src/quadrature_encoder.pio)
    pico_add_extra_outputs(picorx)
    pico_enable_stdio_usb(picorx 1)
    pico_enable_stdio_uart(picorx 0)
    target_include_directories(picorx PRIVATE ${CMAKE_CURRENT_LIST_DIR})
    target_link_libraries(picorx PRIVATE ${PICORX_LIBS})
    target_compile_definitions(picorx PUBLIC PICO_XOSC_STARTUP_DELAY_MULTIPLIER=128  PICO_STACK_SIZE=4096)
    set_target_properties(picorx PROPERTIES SUFFIX ".elf")
    add_custom_command(TARGET picorx POST_BUILD COMMAND ${CMAKE_SIZE} ${CMAKE_CURRENT_BINARY_DIR}/picorx.elf COMMENT "=== Memory / Code Size Report ===")

elseif(PICO_BOARD STREQUAL "pico2")

    add_compile_definitions(WITH_TX)

    if(PICO_PLATFORM STREQUAL "rp2350-riscv")

        list(APPEND PICORX_SRCS src/clocks_pico2.cpp)
        list(APPEND PICORX_SRCS transmit/adc.cpp)
        list(APPEND PICORX_SRCS transmit/tx_best_clock.cpp)
        list(APPEND PICORX_SRCS transmit/transmit_pico2_nco.cpp)
        list(APPEND PICORX_SRCS transmit/half_band_filter.cpp)
        list(APPEND PICORX_SRCS transmit/half_band_filter2.cpp)
        list(APPEND PICORX_SRCS transmit/cordic.cpp)
        list(APPEND PICORX_SRCS transmit/pwm.cpp)
        list(APPEND PICORX_SRCS transmit/iq_pwm.cpp)
        list(APPEND PICORX_SRCS transmit/modulator.cpp)
        list(APPEND PICORX_SRCS transmit/speech_processor.cpp)
        list(APPEND PICORX_SRCS transmit/cw_keyer.cpp)

        #main application
        add_executable(pico2rx-riscv ${PICORX_SRCS})
        target_compile_features(pico2rx-riscv PUBLIC c_std_11)
        pico_generate_pio_header(pico2rx-riscv ${CMAKE_CURRENT_LIST_DIR}/src/nco.pio)
        pico_generate_pio_header(pico2rx-riscv ${CMAKE_CURRENT_LIST_DIR}/src/quadrature_encoder.pio)
        pico_add_extra_outputs(pico2rx-riscv)
        # enable usb output, disable uart output
        pico_enable_stdio_usb(pico2rx-riscv 1)
        pico_enable_stdio_uart(pico2rx-riscv 0)
        target_include_directories(pico2rx-riscv PRIVATE ${CMAKE_CURRENT_LIST_DIR})
        target_link_libraries(pico2rx-riscv PRIVATE ${PICORX_LIBS})
        target_compile_definitions(pico2rx-riscv PUBLIC PICO_XOSC_STARTUP_DELAY_MULTIPLIER=128  PICO_STACK_SIZE=4096)
        set_target_properties(pico2rx-riscv PROPERTIES SUFFIX ".elf")
        add_custom_command(TARGET pico2rx-riscv POST_BUILD COMMAND ${CMAKE_SIZE} ${CMAKE_CURRENT_BINARY_DIR}/picorx-riscv.elf COMMENT "=== Memory / Code Size Report ===")

    else()

        list(APPEND PICORX_SRCS src/clocks_pico2.cpp)
        list(APPEND PICORX_SRCS src/clocks_pico2.cpp)
        list(APPEND PICORX_SRCS transmit/adc.cpp)
        list(APPEND PICORX_SRCS transmit/tx_best_clock.cpp)
        list(APPEND PICORX_SRCS transmit/transmit_pico2_nco.cpp)
        list(APPEND PICORX_SRCS transmit/half_band_filter.cpp)
        list(APPEND PICORX_SRCS transmit/half_band_filter2.cpp)
        list(APPEND PICORX_SRCS transmit/cordic.cpp)
        list(APPEND PICORX_SRCS transmit/pwm.cpp)
        list(APPEND PICORX_SRCS transmit/iq_pwm.cpp)
        list(APPEND PICORX_SRCS transmit/modulator.cpp)
        list(APPEND PICORX_SRCS transmit/speech_processor.cpp)
        list(APPEND PICORX_SRCS transmit/cw_keyer.cpp)

        #main application
        add_executable(pico2rx ${PICORX_SRCS})
        target_compile_features(pico2rx PUBLIC c_std_11)
        pico_generate_pio_header(pico2rx ${CMAKE_CURRENT_LIST_DIR}/src/nco.pio)
        pico_generate_pio_header(pico2rx ${CMAKE_CURRENT_LIST_DIR}/src/quadrature_encoder.pio)
        pico_add_extra_outputs(pico2rx)
        pico_enable_stdio_usb(pico2rx 1)
        pico_enable_stdio_uart(pico2rx 0)
        target_include_directories(pico2rx PRIVATE ${CMAKE_CURRENT_LIST_DIR})
        target_link_libraries(pico2rx PRIVATE ${PICORX_LIBS})
        target_compile_definitions(pico2rx PUBLIC PICO_XOSC_STARTUP_DELAY_MULTIPLIER=128  PICO_STACK_SIZE=4096)
        set_target_properties(pico2rx PROPERTIES SUFFIX ".elf")
        add_custom_command(TARGET pico2rx POST_BUILD COMMAND ${CMAKE_SIZE} ${CMAKE_CURRENT_BINARY_DIR}/pico2rx.elf COMMENT "=== Memory / Code Size Report ===")

    endif()
endif()
