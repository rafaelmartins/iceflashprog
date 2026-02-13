# SPDX-FileCopyrightText: 2025 Rafael G. Martins <rafael@rafaelmartins.eng.br>
# SPDX-License-Identifier: GPL-2.0-only

cmake_minimum_required(VERSION 3.25)

include(FetchContent)

FetchContent_Declare(cmake_cmsis_stm32
    GIT_REPOSITORY https://github.com/rafaelmartins/cmake-cmsis-stm32.git
    GIT_TAG 5042adce80f41f937d09ea7db59f595f6718a7e8
)
FetchContent_Declare(usbd_fs_stm32
    GIT_REPOSITORY https://github.com/rafaelmartins/usbd-fs-stm32.git
    GIT_TAG 2cd61442a470691a20f88d944446cfcffc83eb1b
)
FetchContent_MakeAvailable(
    cmake_cmsis_stm32
    usbd_fs_stm32
)

project(iceflashprog C ASM)

add_executable(iceflashprog
    descriptors.c
    main.c
    spi.c
    spi_flash.c
    watchdog.c
)

target_compile_definitions(usbd-fs-stm32 INTERFACE
    USBD_EP1_IN_SIZE=64
    USBD_EP1_OUT_SIZE=64
    USBD_EP1_TYPE=INTERRUPT
)

target_compile_definitions(iceflashprog PRIVATE
    # SPI_MAX_FREQ
    # WATCHDOG_STOP_ON_HALT
)

target_link_libraries(iceflashprog PRIVATE
    usbd-fs-stm32
)

target_compile_options(iceflashprog PRIVATE
    -Wall
    -Wextra
    -Werror
)

set_property(TARGET iceflashprog
    PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE
)

cmsis_stm32_target(iceflashprog
    DEVICE STM32F042x6
    VERSION 20241122015203
    LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/STM32F042KxTx_FLASH.ld
    ADDITIONAL_OUTPUTS BIN MAP IHEX DFU
    SHOW_SIZE ON
    STLINK ON
    INSTALL ON
)
