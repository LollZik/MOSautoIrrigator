cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


set(PICO_BOARD pico_w CACHE STRING "Board type")

set(PICO_SDK_PATH "$ENV{HOME}/pico-sdk")
include(${PICO_SDK_PATH}/external/pico_sdk_import.cmake)


project(Wyswietlacz C CXX ASM)

pico_sdk_init()

add_executable(Wyswietlacz Wyswietlacz.c ssd1680.c)

pico_set_program_name(Wyswietlacz "Wyswietlacz")
pico_set_program_version(Wyswietlacz "0.1")


pico_enable_stdio_uart(Wyswietlacz 0)
pico_enable_stdio_usb(Wyswietlacz 0)

include_directories(${PICO_SDK_PATH}/src)


target_link_libraries(Wyswietlacz pico_stdlib hardware_spi pico_cyw43_arch_none)

target_include_directories(Wyswietlacz PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
)