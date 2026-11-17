cmake_minimum_required(VERSION 3.12)

# Pull in SDK (must be before project)
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(dspicard C CXX ASM)

set(FAMILY rp2040)
set(BOARD pico_sdk)
set(TINYUSB_FAMILY_PROJECT_NAME_PREFIX "tinyusb_dev_")

set(TOP ${PICO_TINYUSB_PATH})

set(CMAKE_C_STANDARD 23)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if (PICO_SDK_VERSION_STRING VERSION_LESS "2.2.0")
    message(FATAL_ERROR "Raspberry Pi Pico SDK version 2.2.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

# Initialize the SDK
pico_sdk_init()

add_compile_options(-Wall
        -Wno-format          # int != int32_t as far as the compiler is concerned because gcc has int32_t as long int
        -Wno-unused-function # we have some for the docs that aren't called
        )
if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
    add_compile_options(-Wno-maybe-uninitialized)
endif()

include_directories(dspicard PUBLIC ${CMAKE_CURRENT_LIST_DIR})

set(SOURCES
	dspicard.c
	)

add_executable(dspicard
    ${HEADER_FILES} ${SOURCES}
    )

pico_generate_pio_header(dspicard ${PROJECT_SOURCE_DIR}/spi.pio)

# pull in common dependencies


# create map/bin/hex file etc.
pico_enable_stdio_usb(dspicard 1)
pico_enable_stdio_uart(dspicard 0)
pico_add_extra_outputs(dspicard)

target_link_libraries(dspicard hardware_pio hardware_exception hardware_i2c pico_stdlib pico_multicore pico_unique_id pico_util)