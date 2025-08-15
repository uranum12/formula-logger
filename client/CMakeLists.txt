cmake_minimum_required(VERSION 3.12)

include(cmake/pico_sdk_import.cmake)

project(pico_mqtt_test C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)

pico_sdk_init()

add_executable(main src/bme280.c src/main.cpp)
target_include_directories(main PUBLIC include)
target_link_libraries(
    main PRIVATE pico_stdlib hardware_gpio hardware_spi
                 pico_cyw43_arch_lwip_threadsafe_background pico_lwip_mqtt)
pico_enable_stdio_usb(main 0)
pico_enable_stdio_uart(main 1)
pico_add_extra_outputs(main)
