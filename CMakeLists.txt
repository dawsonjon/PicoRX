# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico2 CACHE STRING "Board type")

cmake_minimum_required(VERSION 3.12)

include(pico_sdk_import.cmake)

set(CMAKE_C_STANDARD 11)

add_compile_options(-Wall -Werror -fdata-sections -ffunction-sections)
add_compile_options(-Wall -fdata-sections -ffunction-sections)
add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>)
add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>)


project(picorx)
pico_sdk_init()

file(GLOB U8G2_SRCS
     "external/u8g2/csrc/*.c"
)

include_directories(include external/u8g2/csrc/)

add_library(
    u8g2
    ${U8G2_SRCS}
)
target_compile_options(u8g2 PRIVATE -Wno-unused-function -Wno-unused-variable)

set(SRCS 
    ssd1306.c
    picorx.cpp
    nco.cpp
    rx.cpp
    rx_dsp.cpp
    fft.cpp
    fft_filter.cpp
    noise_reduction.cpp
    cic_corrections.cpp
    ui.cpp
    settings.cpp
    memory.cpp
    autosave_memory.cpp
    utils.cpp
    ili934x.cpp
    waterfall.cpp
    button.cpp
    quadrature_si5351.cpp
    cat.cpp
    pwm_audio_sink.cpp
    usb_descriptors.c
    usb_audio_device.c
    ring_buffer_lib.c
    codecs/decode_sstv.cpp
    codecs/cordic.cpp
    transmit/adc.cpp
    transmit/transmit_nco.cpp
    transmit/half_band_filter.cpp
    transmit/half_band_filter2.cpp
    transmit/pwm.cpp
    transmit/modulator.cpp
    transmit/signal_generator.cpp
    transmit/cw_keyer.cpp
)

if(BUTTON_ENCODER)
    add_definitions(-DBUTTON_ENCODER)
    list(APPEND SRCS button_encoder.cpp)
else()
    list(APPEND SRCS rotary_encoder.cpp)
endif()

if(PICO_BOARD STREQUAL "pico")

  list(APPEND SRCS clocks_pico.cpp)
  #main application
  add_executable(picorx ${SRCS})
  pico_generate_pio_header(picorx ${CMAKE_CURRENT_LIST_DIR}/nco.pio)

  pico_generate_pio_header(picorx ${CMAKE_CURRENT_LIST_DIR}/quadrature_encoder.pio)
  pico_add_extra_outputs(picorx)
  # enable usb output, disable uart output
  pico_enable_stdio_usb(picorx 1)
  pico_enable_stdio_uart(picorx 0)
  target_include_directories(picorx PRIVATE ${CMAKE_CURRENT_LIST_DIR})
  target_link_libraries(picorx PRIVATE pico_stdlib
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
                                       u8g2)
  target_compile_definitions(picorx PUBLIC PICO_XOSC_STARTUP_DELAY_MULTIPLIER=128)

  #battery check utility
  project(battery_check)
  add_executable(battery_check
      battery_check.cpp
  )
  pico_add_extra_outputs(battery_check)
  pico_enable_stdio_usb(battery_check 1)
  pico_enable_stdio_uart(battery_check 0)
  target_link_libraries(battery_check pico_stdlib hardware_adc)

elseif(PICO_BOARD STREQUAL "pico2")


  if(PICO_PLATFORM STREQUAL "rp2350-riscv")

    list(APPEND SRCS clocks_pico2.cpp)
    #main application
    add_executable(pico2rx-riscv ${SRCS})
     pico_generate_pio_header(pico2rx-riscv ${CMAKE_CURRENT_LIST_DIR}/nco.pio)
    pico_generate_pio_header(pico2rx-riscv ${CMAKE_CURRENT_LIST_DIR}/quadrature_encoder.pio)
    pico_add_extra_outputs(pico2rx-riscv)
    # enable usb output, disable uart output
    pico_enable_stdio_usb(pico2rx-riscv 1)
    pico_enable_stdio_uart(pico2rx-riscv 0)
    target_include_directories(pico2rx-riscv PRIVATE ${CMAKE_CURRENT_LIST_DIR})
    target_link_libraries(pico2rx-riscv PRIVATE pico_stdlib
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
                                         u8g2)
    target_compile_definitions(pico2rx-riscv PUBLIC PICO_XOSC_STARTUP_DELAY_MULTIPLIER=128)

    #battery check utility
    project(battery_check_pico2-riscv)
    add_executable(battery_check_pico2-riscv
        battery_check.cpp
    )
    pico_add_extra_outputs(battery_check_pico2-riscv)
    pico_enable_stdio_usb(battery_check_pico2-riscv 1)
    pico_enable_stdio_uart(battery_check_pico2-riscv 0)
    target_link_libraries(battery_check_pico2-riscv pico_stdlib hardware_adc)

  else()

    list(APPEND SRCS clocks_pico2.cpp)
    #main application
    add_executable(pico2rx ${SRCS})
    pico_generate_pio_header(pico2rx ${CMAKE_CURRENT_LIST_DIR}/nco.pio)
    pico_generate_pio_header(pico2rx ${CMAKE_CURRENT_LIST_DIR}/quadrature_encoder.pio)
    pico_add_extra_outputs(pico2rx)
    pico_enable_stdio_usb(pico2rx 1)
    pico_enable_stdio_uart(pico2rx 0)
    target_include_directories(pico2rx PRIVATE ${CMAKE_CURRENT_LIST_DIR})
    target_link_libraries(pico2rx PRIVATE pico_stdlib
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
                                         u8g2)
    target_compile_definitions(pico2rx PUBLIC PICO_XOSC_STARTUP_DELAY_MULTIPLIER=128)

    #battery check utility
    project(battery_check_pico2)
    add_executable(battery_check_pico2
        battery_check.cpp
    )
    pico_add_extra_outputs(battery_check_pico2)
    pico_enable_stdio_usb(battery_check_pico2 1)
    pico_enable_stdio_uart(battery_check_pico2 0)
    target_link_libraries(battery_check_pico2 pico_stdlib hardware_adc)

  endif()

endif()
