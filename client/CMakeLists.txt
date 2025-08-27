cmake_minimum_required(VERSION 3.12)

include(cmake/pico_sdk_import.cmake)

project(pico_mqtt_test C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)

pico_sdk_init()

add_executable(front src/front.cpp)
target_include_directories(front PRIVATE include)
target_link_libraries(front PRIVATE pico_stdlib)
pico_enable_stdio_usb(front 0)
pico_enable_stdio_uart(front 1)
pico_add_extra_outputs(front)

add_executable(rear src/rear.cpp)
target_include_directories(rear PRIVATE include)
target_link_libraries(rear PRIVATE pico_stdlib)
pico_enable_stdio_usb(rear 0)
pico_enable_stdio_uart(rear 1)
pico_add_extra_outputs(rear)
