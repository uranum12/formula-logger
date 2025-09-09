cmake_minimum_required(VERSION 3.12)

include(cmake/pico_sdk_import.cmake)

project(pico_mqtt_test C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)

pico_sdk_init()

add_library(cjson libs/cjson/cJSON.c)
target_include_directories(cjson PUBLIC libs/cjson)

add_executable(rpm_probe src/rpm_probe.cpp)
target_link_libraries(rpm_probe PRIVATE pico_stdlib hardware_uart cjson)
pico_enable_stdio_usb(rpm_probe 1)
pico_enable_stdio_uart(rpm_probe 0)
pico_add_extra_outputs(rpm_probe)
