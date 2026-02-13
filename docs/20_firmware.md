# Firmware

The iceflashprog firmware runs on an STM32F042 microcontroller (ARM Cortex-M0) on the [stm32f0-usbd-devboard](@@/p/stm32-usbd-devboards/stm32f0/). It is a bare-metal application with no HAL or RTOS -- all peripheral access uses direct CMSIS register operations. The firmware implements a USB HID interface for communicating with the host software and drives SPI flash operations using DMA. A single binary runs on all STM32F042K variants supported by the devboard.

## Building from source

### Prerequisites

- CMake 3.25 or later
- ARM GNU Toolchain (`arm-none-eabi`)
- Ninja (recommended) or Make

The following dependencies are fetched automatically via CMake FetchContent:

- [cmake-cmsis-stm32](@@/p/cmake-cmsis-stm32/) -- CMake build system for STM32 targets
- [usbd-fs-stm32](@@/p/usbd-fs-stm32/) -- USB Full-Speed device library for STM32

### Configure and build

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -S firmware -G Ninja
cmake --build build
```

### Output artifacts

| File | Description |
|------|-------------|
| `iceflashprog.elf` | ELF binary with debug symbols |
| `iceflashprog.bin` | Raw binary image |
| `iceflashprog.hex` | Intel HEX format |
| `iceflashprog.dfu` | DFU image with suffix (for `dfu-util`) |
| `iceflashprog.map` | Linker map file |

### Memory layout

The linker script uses a 16 KB flash region to ensure compatibility with all STM32F042K variants, including the smallest STM32F042K4.

| Region | Start address | Size |
|--------|---------------|------|
| Flash | `0x08000000` | 16 KB |
| RAM | `0x20000000` | 6 KB |

## Flashing

### Using USB DFU

A prebuilt DFU binary is available from the [latest release (v0.1.0)](https://github.com/rafaelmartins/iceflashprog/releases/tag/v0.1.0) as [`iceflashprog-firmware-0.1.0.zip`](https://github.com/rafaelmartins/iceflashprog/releases/download/v0.1.0/iceflashprog-firmware-0.1.0.zip). Extract the archive and flash the `iceflashprog.dfu` file. See the [Hardware Build Manual](@@/hardware/build-manual/) for detailed DFU flashing instructions.

> [!NOTE]
> If the microcontroller is empty (fresh from the factory), it boots directly into the DFU bootloader and is ready to flash without any pin jumper.

### Using ST-Link

A dedicated CMake target is available for ST-Link flashing:

```bash
cmake --build build --target iceflashprog-stlink-write
```

See the [Hardware Build Manual](@@/hardware/build-manual/) for detailed ST-Link instructions.

## Architecture overview

### Clock configuration

By default, the firmware runs the system clock at 48 MHz from the internal HSI48 oscillator, which also serves as the USB clock source. This results in an SPI clock of 12 MHz (HSI48 / 4).

An alternative `SPI_MAX_FREQ` compile-time option reconfigures the PLL to produce a 36 MHz system clock (HSI48 / 4 * 3), yielding an SPI clock of 18 MHz (36 MHz / 2). The HSI48 oscillator remains active in both modes and is used as the USB clock source.

### Peripheral map

| Peripheral | Function |
|------------|----------|
| GPIOA PA4 | SPI1 chip select (software-controlled) |
| GPIOA PA5 | SPI1 SCK |
| GPIOA PA6 | SPI1 MISO |
| GPIOA PA7 | SPI1 MOSI |
| GPIOA PA15 | LED output |
| GPIOB PB0 | CRST (FPGA reset, active low) |
| SPI1 | SPI master, DMA-driven flash communication |
| DMA1 Ch2 | SPI1 RX |
| DMA1 Ch3 | SPI1 TX |
| TIM3 | SPI flash status register polling (1 ms period) |
| IWDG | Independent watchdog (~1 s default, ~15 s during chip erase) |

### Main loop

The firmware runs a cooperative polling loop. On each iteration, `spi_flash_task()` is called first to process any pending SPI flash operations (including DMA completion and status register polling via TIM3). If no flash work is pending, `usbd_task()` processes USB events. The watchdog is reloaded on every USB SOF frame (1 ms interval) as long as no write-in-progress flag is set, ensuring the device resets if the host stops communicating.

### Main source files

| File | Purpose |
|------|---------|
| `main.c` | USB HID report handling, clock initialization, main loop |
| `descriptors.c` | USB device, configuration, HID report, and string descriptors |
| `spi.c` | SPI1 peripheral driver with DMA transfers |
| `spi_flash.c` | SPI flash command layer (read, write, erase, JEDEC ID, power management) |
| `watchdog.c` | Independent watchdog initialization and reload management |

## USB HID protocol

The device uses a vendor-specific USB HID protocol. This is not a standard HID device (keyboard, mouse, etc.) -- it requires custom host software.

### Device identity

| Field | Value |
|-------|-------|
| USB version | 2.0 Full-Speed |
| Device class | HID (interface-level) |
| VID | `0x16c0` |
| PID | `0x05df` |
| Manufacturer | `rgm.io` |
| Product | `iceflashprog` |
| Serial number | STM32 internal unique ID |
| Max power | 100 mA (bus-powered) |
| HID version | 1.11 |

### Endpoints

| Direction | Type | Max packet size | Interval |
|-----------|------|-----------------|----------|
| IN (EP1) | Interrupt | 64 bytes | 1 ms |
| OUT (EP1) | Interrupt | 64 bytes | 1 ms |

### HID reports

The device defines two report IDs with different purposes:

| Report ID | Direction | Size (bytes) | Purpose |
|-----------|-----------|--------------|---------|
| 1 | Input (device to host) | 256 | Flash page data |
| 1 | Output (host to device) | 259 | Flash page write (3-byte address + 256-byte data) |
| 2 | Input (device to host) | 4 | Command response (1 status + 3 data) |
| 2 | Output (host to device) | 4 | Command request (1 command ID + 3 data) |

Reports larger than the 64-byte endpoint size are transferred in multiple USB transactions.

### Commands (report ID 2)

| Command ID | Name | Request data | Response data |
|------------|------|-------------|---------------|
| 1 | Power Up | (unused) | (none) |
| 2 | Power Down | (unused) | (none) |
| 3 | JEDEC ID | (unused) | manufacturer ID (1 byte) + device ID (2 bytes) |
| 4 | Read | 3-byte address | Flash page returned via report ID 1 |
| 5 | Erase Sector | 3-byte address | (none) |
| 6 | Erase Block | 3-byte address | (none) |
| 7 | Erase Chip | (unused) | (none) |

The Power Up command also de-asserts the FPGA reset line (CRST), and Power Down re-asserts it. The host must send Power Up before any flash operations.

### Flash page write (report ID 1)

To write a flash page, the host sends report ID 1 with 3 bytes of address followed by 256 bytes of data (259 bytes total). The firmware performs the write and then automatically reads back the page to verify correctness. The result is returned as a report ID 2 response with the appropriate status code.

### Status codes

| Value | Name | Description |
|-------|------|-------------|
| 0 | OK | Operation completed successfully |
| 1 | Unpowered | Flash chip is not powered (Power Up not sent) |
| 2 | Invalid Request | Malformed request |
| 3 | Invalid Command ID | Unknown command ID |
| 4 | Invalid Flash Page Read | Flash page read returned unexpected length |
| 5 | Invalid Flash Page Write | Write verification failed |
| 6 | Locked | Device is busy with another operation |

### Vendor usage page

The HID report descriptor uses vendor usage page `0xFF00` with the following usage IDs:

| Usage ID | Name |
|----------|------|
| `0x0001` | iceflashprog (application collection) |
| `0x0002` | Flash Page |
| `0x0003` | Request |
| `0x0004` | Response |
| `0x0011` | Address |
| `0x0012` | Data |
| `0x0013` | Command ID |
| `0x0015` | Status |
