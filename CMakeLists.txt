cmake_minimum_required(VERSION 3.12)
set(PICO_SDK_PATH $ENV{PICO_SDK_PATH})

include(pico_sdk_import.cmake)

project(hello)

pico_sdk_init()

add_executable(hello
    hello.c
)

pico_generate_pio_header(hello ${CMAKE_CURRENT_LIST_DIR}/hello.pio)

pico_add_extra_outputs(hello)

target_link_libraries(hello pico_stdlib hardware_pio)
