# == DO NEVER EDIT THE NEXT LINES for Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.0.0)
set(toolchainVersion 13_2_Rel1)
set(picotoolVersion 2.0.0)
include(${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
# ====================================================================================
cmake_minimum_required(VERSION 3.12)

include(pico_sdk_import.cmake)

set(CMAKE_C_STANDARD 11)

add_compile_options(-Wall -Werror -fdata-sections -ffunction-sections)
add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>)
add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>)

if(PICO_BOARD STREQUAL "pico")

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

elseif(PICO_BOARD STREQUAL "pico2")


  if(PICO_PLATFORM STREQUAL "rp2350-riscv")

    #main application
    project(pico2rx-riscv)
    pico_sdk_init()
    add_executable(pico2rx-riscv
        ssd1306.c
        picorx.cpp
        rx.cpp
        rx_dsp.cpp
        half_band_filter.cpp
        half_band_filter2.cpp
        ui.cpp
    )
    pico_generate_pio_header(pico2rx-riscv ${CMAKE_CURRENT_LIST_DIR}/nco.pio)
    pico_generate_pio_header(pico2rx-riscv ${CMAKE_CURRENT_LIST_DIR}/quadrature_encoder.pio)
    pico_add_extra_outputs(pico2rx-riscv)
    # enable usb output, disable uart output
    pico_enable_stdio_usb(pico2rx-riscv 1)
    pico_enable_stdio_uart(pico2rx-riscv 0)
    target_link_libraries(pico2rx-riscv pico_stdlib pico_multicore hardware_clocks hardware_flash hardware_pio hardware_adc hardware_pwm hardware_dma hardware_i2c)

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

    #main application
    project(pico2rx)
    pico_sdk_init()
    add_executable(pico2rx
        ssd1306.c
        picorx.cpp
        rx.cpp
        rx_dsp.cpp
        half_band_filter.cpp
        half_band_filter2.cpp
        ui.cpp
    )
    pico_generate_pio_header(pico2rx ${CMAKE_CURRENT_LIST_DIR}/nco.pio)
    pico_generate_pio_header(pico2rx ${CMAKE_CURRENT_LIST_DIR}/quadrature_encoder.pio)
    pico_add_extra_outputs(pico2rx)
    # enable usb output, disable uart output
    pico_enable_stdio_usb(pico2rx 1)
    pico_enable_stdio_uart(pico2rx 0)
    target_link_libraries(pico2rx pico_stdlib pico_multicore hardware_clocks hardware_flash hardware_pio hardware_adc hardware_pwm hardware_dma hardware_i2c)

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
