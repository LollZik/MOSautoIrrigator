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

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(PICO_BOARD pico_w CACHE STRING "Board type")

include(pico_sdk_import.cmake)

project(Konewka C CXX ASM)

pico_sdk_init()

add_executable(Konewka Konewka.c )

pico_set_program_name(Konewka "Konewka")
pico_set_program_version(Konewka "0.1")


# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(Konewka 0)
pico_enable_stdio_usb(Konewka 0)

# Add the standard library to the build
target_link_libraries(Konewka pico_stdlib hardware_adc hardware_spi)

# Link wifi library
if (PICO_CYW43_SUPPORTED)
    target_link_libraries(Konewka pico_cyw43_arch_lwip_threadsafe_background)
endif()

# Add the standard include files to the build
target_include_directories(Konewka PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/.. # lwipopts
)

pico_add_extra_outputs(Konewka)