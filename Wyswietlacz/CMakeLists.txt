# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.1)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.1.1)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
cmake_minimum_required(VERSION 3.13)


# Languages standards
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Generate compile_commands.json for VS Code
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(PICO_BOARD pico_w CACHE STRING "Board type")


# Import and initialize Pico SDK
include(pico_sdk_import.cmake)
project(Wyswietlacz C CXX ASM)
pico_sdk_init()


# SSD1680 (e-ink)
add_library(ssd1680 STATIC ssd1680.c)

target_include_directories(ssd1680 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(ssd1680 
    pico_stdlib
    hardware_spi
    )


# Main program
add_executable(Wyswietlacz
    Wyswietlacz.c
)

# Name and version of the project
pico_set_program_name(Wyswietlacz "Wyswietlacz")
pico_set_program_version(Wyswietlacz "0.1")

# turn off UART/USB stdio
pico_enable_stdio_uart(Wyswietlacz 0)
pico_enable_stdio_usb(Wyswietlacz 0)

# Link all the libraries
target_link_libraries(Wyswietlacz
    pico_stdlib
    hardware_spi
    hardware_gpio
    pico_cyw43_arch_lwip_threadsafe_background
    ssd1680
)

target_include_directories(Wyswietlacz PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/.. # lwipopts
)

# .uf2, .bin, .hex
pico_add_extra_outputs(Wyswietlacz)
