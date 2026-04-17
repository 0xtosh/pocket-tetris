# Setup Guide

## Requirements

- Windows, macOS, or Linux
- LilyGO T-Dongle S3
- USB data cable
- VS Code with PlatformIO, or Python with PlatformIO Core

## Install Tooling

### VS Code route

1. Install VS Code.
2. Install the PlatformIO IDE extension.
3. Open this repository folder.
4. Build from the PlatformIO UI or with one of the scripts in `scripts/`.

### Command-line route

Install PlatformIO and the helper modules used by common flashing workflows:

```powershell
py -m pip install -U platformio intelhex esptool
```

If your environment uses `python` instead of `py`, the included wrapper scripts will try both.

## Dependencies

The project declares firmware dependencies in `platformio.ini`. PlatformIO installs them automatically during the first build.

Main dependency set:

- `links2004/WebSockets`
- Arduino ESP32 framework libraries such as `WiFi`, `WebServer`, and `DNSServer`

## Build Options

### Standard build

```powershell
./scripts/build.ps1
```

### Compile wrapper

```powershell
./scripts/compile.ps1
```

### Link wrapper

```powershell
./scripts/link.ps1
```

### Raw PlatformIO command

```powershell
py -m platformio run -e lilygo_t_dongle_s3
```

## Output Files

After a successful build, PlatformIO creates:

- `.pio/build/lilygo_t_dongle_s3/bootloader.bin`
- `.pio/build/lilygo_t_dongle_s3/partitions.bin`
- `.pio/build/lilygo_t_dongle_s3/firmware.bin`
- `.pio/build/lilygo_t_dongle_s3/firmware.elf`

To copy the current build into the tracked release folder:

```powershell
./scripts/package-release.ps1
```

## First Boot

1. Flash the firmware.
2. Power the board.
3. Join WiFi `games` using password `gamesgames`.
4. Let the captive portal open automatically, or browse manually to `http://192.168.4.1/`.

## Portal Overview

The controller page includes:

- current score and level
- boot-session high score and highest level reached
- lines cleared
- game state
- touch controls for rotate, movement, hard drop, and restart
