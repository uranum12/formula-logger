cmake_minimum_required(VERSION 3.12)

include(cmake/pico_sdk_import.cmake)

project(pico_mqtt_test C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)

pico_sdk_init()

add_library(cjson libs/cjson/cJSON.c)
target_include_directories(cjson PUBLIC libs/cjson)

add_library(uart_tx src/uart_tx.c)
target_include_directories(uart_tx PUBLIC include)
pico_generate_pio_header(uart_tx ${CMAKE_CURRENT_LIST_DIR}/src/uart_tx.pio)
target_link_libraries(uart_tx PUBLIC pico_stdlib hardware_pio)

add_library(shift_out src/shift_out.c)
target_include_directories(shift_out PUBLIC include)
pico_generate_pio_header(shift_out ${CMAKE_CURRENT_LIST_DIR}/src/shift_out.pio)
target_link_libraries(shift_out PUBLIC pico_stdlib hardware_gpio hardware_pio)

add_executable(front src/bme280.c src/bno055.c src/front.cpp src/mcp3204.c
                     src/meter.cpp)
target_include_directories(front PRIVATE include)
target_link_libraries(
    front
    PRIVATE pico_stdlib
            pico_multicore
            hardware_uart
            hardware_spi
            hardware_i2c
            cjson
            shift_out)
pico_enable_stdio_usb(front 0)
pico_enable_stdio_uart(front 1)
pico_add_extra_outputs(front)

add_executable(rear src/mcp3204.c src/mcp3208.c src/rear.cpp)
target_include_directories(rear PRIVATE include)
target_link_libraries(rear PRIVATE pico_stdlib pico_multicore hardware_uart
                                   hardware_spi cjson)
pico_enable_stdio_usb(rear 0)
pico_enable_stdio_uart(rear 1)
pico_add_extra_outputs(rear)
