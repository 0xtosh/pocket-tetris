# Pocket Tetris

Pocket Tetris turns the LilyGO T-Dongle S3 into a tiny arcade cabinet with a built-in screen, a captive-portal browser controller, a QR-code idle screen, and a full-color 10x20 Tetris playfield.

The board boots into WiFi info mode, shows a QR code on the display, and hosts its own controller page. Join the board's network, tap into the portal, and start stacking blocks.

## What It Does

- Runs directly on the LilyGO T-Dongle S3 with the built-in 80x160 ST7735 display
- Uses a proven `esp_lcd` ST7735 init path for reliable display bring-up
- Shows a QR code on the device screen before gameplay starts
- Hosts a browser gamepad with live score, high score, level, lines, and state
- Uses captive-portal style redirects to help phones open the controller page quickly
- Tracks boot-session high score and highest level reached
- Uses 10 color themes across levels 0 through 9
- Includes ready-to-flash firmware binaries in `release/lilygo_t_dongle_s3/`

## Hardware

- Board: LilyGO T-Dongle S3
- MCU: ESP32-S3
- Display: 80x160 ST7735
- Flashing: USB Serial/JTAG over the board USB connection

## Network

- SSID: `games`
- Password: `gamesgames`
- Controller page: `http://192.168.4.1/`
- WebSocket port: `81`
- UDP port: `10000`

## How To Play

1. Power the board.
2. Scan the QR code or join WiFi `games` with password `gamesgames`.
3. If the captive portal does not pop up automatically, open `http://192.168.4.1/`.
4. Press `Restart` on the controller page to begin a new round.
5. Use the browser controls to move, rotate, soft drop, and hard drop pieces.
6. Use the board button to switch between the QR info screen and the game screen.

## Controls

- Left arrow: move left
- Right arrow: move right
- Down arrow: soft drop
- `Rotate L`: rotate counter-clockwise
- `Rotate R`: rotate clockwise
- `Hard Drop`: drop immediately
- `Restart`: start a new run instantly

## Install Dependencies

### Option 1: VS Code + PlatformIO

1. Install VS Code.
2. Install the PlatformIO IDE extension.
3. Open this repository folder.
4. PlatformIO will install board packages and libraries on the first build.

### Option 2: PlatformIO Core on the command line

Install the base tools once:

```powershell
py -m pip install -U platformio intelhex esptool
```

If your system uses `python` instead of `py`, the wrapper scripts will try that automatically.

## Build, Compile, Link, Flash

The project includes both PowerShell and `.cmd` wrappers so Windows users can run everything from Explorer, PowerShell, or `cmd.exe`.

### Build wrappers

- `scripts/compile.ps1`
- `scripts/compile.cmd`
- `scripts/link.ps1`
- `scripts/link.cmd`
- `scripts/build.ps1`
- `scripts/build.cmd`

These wrappers all drive the PlatformIO firmware pipeline. `compile` and `link` are convenience wrappers for users who want those stages called out explicitly, while `build` is the standard full build entry point.

### Flash wrappers

- `scripts/flash-project.ps1 -Port COM18`
- `scripts/flash-project.cmd COM18`
- `scripts/flash-prebuilt.ps1 -Port COM18`
- `scripts/flash-prebuilt.cmd COM18`

Use `flash-project` if you want to flash from the current source build. Use `flash-prebuilt` if you only want to flash the included release binaries.

### One-command wrapper

- `scripts/all-in-one.ps1 -Port COM18`
- `scripts/all-in-one.cmd COM18`

This wrapper builds the firmware, refreshes the release folder, and flashes the board in one go.

### Other helpers

- `scripts/package-release.ps1`
- `scripts/package-release.cmd`
- `scripts/monitor.ps1 -Port COM18`
- `scripts/monitor.cmd COM18`

## Direct Commands

If you want the raw PlatformIO commands instead of the wrappers:

```powershell
py -m platformio run -e lilygo_t_dongle_s3
py -m platformio run -e lilygo_t_dongle_s3 -t upload --upload-port COM18
```

## Included Firmware

The repository already includes a prebuilt firmware set in `release/lilygo_t_dongle_s3/`:

- `bootloader.bin`
- `partitions.bin`
- `firmware.bin`
- `BOOT_DESCRIPTION.txt`

Those files can be flashed directly with the prebuilt flash wrappers.

## Repository Layout

- `src/`: firmware source
- `boards/dongles3.json`: local PlatformIO board definition
- `scripts/`: build, package, flash, monitor, and wrapper scripts
- `docs/`: setup and flashing notes
- `release/lilygo_t_dongle_s3/`: tracked prebuilt firmware bundle

## Notes For GitHub Packaging

- The repository docs and helper scripts use repository-relative paths only.
- The release notes avoid machine-local paths.
- `.vscode/` and `.pio/` are ignored so editor and build cache paths do not get packaged.

## Extra Docs

- `docs/SETUP_GUIDE.md`
- `docs/FLASHING.md`
