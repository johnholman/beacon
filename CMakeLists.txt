# CMakeLists.txt for pico beacon firmware

cmake_minimum_required(VERSION 3.12)

# Pull in SDK (must be before project)
include(pico_sdk_import.cmake)

# include(pico_extras_import_optional.cmake)

project(beacon C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# set(PICO_EXAMPLES_PATH ${PROJECT_SOURCE_DIR})

set(PICO_BOARD "pico_w")
set(WIFI_SSID $ENV{WIFI_SSID})
set(WIFI_PASSWORD $ENV{WIFI_PASSWORD})

# Initialize the SDK
pico_sdk_init()

add_compile_options(-Wall
        -Wno-format          # int != int32_t as far as the compiler is concerned because gcc has int32_t as long int
        -Wno-unused-function # we have some for the docs that aren't called
        )
if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
    add_compile_options(-Wno-maybe-uninitialized)
endif()


add_executable(beacon)

pico_generate_pio_header(beacon ${CMAKE_CURRENT_LIST_DIR}/pio_beacon.pio)

target_sources(beacon PRIVATE pio_beacon.c)

target_include_directories(beacon PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        )

target_link_libraries(beacon PRIVATE
        pico_stdlib
        pico_cyw43_arch_lwip_threadsafe_background
        pico_lwip_mqtt
        hardware_pio
        )

target_compile_definitions(beacon PRIVATE
        WIFI_SSID=\"${WIFI_SSID}\"
        WIFI_PASSWORD=\"${WIFI_PASSWORD}\"
        )

pico_add_extra_outputs(beacon)


