# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.3.0)
set(toolchainVersion 13_2_Rel1)
set(picotoolVersion 2.3.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
# CMakeLists.txt for pico beacon firmware

set(FIRMWARE_VERSION "1.1, 19 July 2016")

cmake_minimum_required(VERSION 3.13)

set(PICO_BOARD "pico_w" CACHE STRING "Board Type")

# Pull in SDK (must be before project)
include(pico_sdk_import.cmake)

project(beacon C CXX ASM)

# Initialize the SDK
pico_sdk_init()

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(beacon)

target_sources(beacon PRIVATE pio_beacon.c wifi_scan.c dns_lookup.c)

target_include_directories(beacon PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        )

target_link_libraries(beacon PRIVATE
        pico_stdlib
        pico_cyw43_arch_lwip_threadsafe_background
        pico_lwip_mqtt
        pico_unique_id
        hardware_pio
        )

target_compile_definitions(beacon PRIVATE
        LWIP_DNS_SUPPORT_MDNS_QUERIES=1
        FIRMWARE_VERSION="${FIRMWARE_VERSION}"
        )

# target_compile_options(beacon PRIVATE -Wall)

pico_generate_pio_header(beacon ${CMAKE_CURRENT_LIST_DIR}/pio_beacon.pio)

pico_enable_stdio_uart(beacon 1)
pico_enable_stdio_usb(beacon 1)

pico_add_extra_outputs(beacon)

pico_set_program_name(beacon "beacon")
pico_set_program_version(beacon "${FIRMWARE_VERSION}")


