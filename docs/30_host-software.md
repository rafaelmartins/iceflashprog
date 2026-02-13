# Host software

The `iceflashprog` command-line tool communicates with the iceflashprog device over USB HID to read, write, erase, and verify flash memory on iCE40 FPGA development boards. It is written in Go and uses the [usbhid](https://pkg.go.dev/rafaelmartins.com/p/usbhid) library for cross-platform USB HID access.

## Installation

### Prebuilt binaries

Prebuilt binaries are available from the [latest release (v0.1.0)](https://github.com/rafaelmartins/iceflashprog/releases/tag/v0.1.0):

| Platform | Architecture | Download |
|----------|--------------|----------|
| Linux | `amd64` | [`iceflashprog-linux-amd64-0.1.0.tar.xz`](https://github.com/rafaelmartins/iceflashprog/releases/download/v0.1.0/iceflashprog-linux-amd64-0.1.0.tar.xz) |
| Linux | `arm64` | [`iceflashprog-linux-arm64-0.1.0.tar.xz`](https://github.com/rafaelmartins/iceflashprog/releases/download/v0.1.0/iceflashprog-linux-arm64-0.1.0.tar.xz) |
| macOS | `arm64` | [`iceflashprog-darwin-arm64-0.1.0.tar.xz`](https://github.com/rafaelmartins/iceflashprog/releases/download/v0.1.0/iceflashprog-darwin-arm64-0.1.0.tar.xz) |
| Windows | `amd64` | [`iceflashprog-windows-amd64-0.1.0.zip`](https://github.com/rafaelmartins/iceflashprog/releases/download/v0.1.0/iceflashprog-windows-amd64-0.1.0.zip) |

The Linux archives also include the udev rules file.

### Building from source

A Go compiler 1.24 or later is required:

```bash
go install rafaelmartins.com/p/iceflashprog@latest
```

Or from a local clone:

```bash
go build -o iceflashprog
```

### Linux udev rules

Linux users need to install a udev rules file to allow non-root access to the device. The rules file is included with the prebuilt binary archives.

To install manually:

```bash
sudo cp 60-iceflashprog.rules /etc/udev/rules.d/
sudo udevadm control --reload-rules
sudo udevadm trigger
```

The rule grants access to members of the `plugdev` group and to locally logged-in users (via `uaccess`).

## Usage

### Detect flash memory

Verify that the device is connected and the flash memory is accessible:

```bash
iceflashprog -d
```

Expected output:

```
Manufacturer: 0x1c
Device ID: 0x7015
```

The manufacturer ID and device ID values depend on the specific flash chip on the FPGA board.

### Write bitstream to flash

Write a bitstream binary file to the flash memory. This erases the affected 64 KB blocks before writing and the firmware verifies each 256-byte page after writing:

```bash
iceflashprog bitstream.bin
```

To skip the erase step (useful if the flash was already erased):

```bash
iceflashprog -n bitstream.bin
```

### Verify flash contents

Compare a local file against the contents of the flash memory:

```bash
iceflashprog -c bitstream.bin
```

### Read flash memory to file

Read the entire flash memory (2 MB) to a local file:

```bash
iceflashprog -r output.bin
```

### Erase entire flash

Erase the entire flash chip:

```bash
iceflashprog -e
```

### Multiple devices

When multiple iceflashprog devices are connected, select a specific device by its serial number:

```bash
iceflashprog -s "SERIAL_NUMBER" bitstream.bin
```

If multiple devices are detected and no serial number is provided, the tool reports the available serial numbers as an error.

### Show version

```bash
iceflashprog -V
```

## Command-line reference

| Flag | Description |
|------|-------------|
| `-c` | Compare file content against flash memory |
| `-d` | Detect flash memory and exit |
| `-e` | Erase whole flash memory and exit |
| `-n` | Do not erase flash before writing |
| `-r` | Read flash memory to file |
| `-s` | Device serial number (for multiple devices) |
| `-V` | Show version and exit |
