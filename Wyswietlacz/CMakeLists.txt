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


# Main program
add_executable(Wyswietlacz
    app_display.c
    bsp_display.c
    ssd1680.c
    text_renderer.c
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
    hardware_pwm
    pico_cyw43_arch_lwip_threadsafe_background
)

target_include_directories(Wyswietlacz PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/.. # lwipopts
)

# .uf2, .bin, .hex
pico_add_extra_outputs(Wyswietlacz)
