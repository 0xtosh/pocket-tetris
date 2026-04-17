# Flashing Guide

## Flash From Source

Build-and-upload from the current source tree:

```powershell
./scripts/flash-project.ps1 -Port COM18
```

Batch wrapper:

```bat
scripts\flash-project.cmd COM18
```

Raw command:

```powershell
py -m platformio run -e lilygo_t_dongle_s3 -t upload --upload-port COM18
```

## Flash Prebuilt Firmware Only

If you just want to program the included binaries and skip compiling:

```powershell
./scripts/flash-prebuilt.ps1 -Port COM18
```

Batch wrapper:

```bat
scripts\flash-prebuilt.cmd COM18
```

This uses the offsets documented in `release/lilygo_t_dongle_s3/BOOT_DESCRIPTION.txt`.

## One Command For Build + Package + Flash

```powershell
./scripts/all-in-one.ps1 -Port COM18
```

Batch wrapper:

```bat
scripts\all-in-one.cmd COM18
```

## Serial Monitor

```powershell
./scripts/monitor.ps1 -Port COM18
```

## If The Board Does Not Enter Flash Mode

1. Disconnect USB.
2. Hold the board boot button if your board revision requires it.
3. Reconnect USB.
4. Retry the flash command.

## Flash Layout

- `0x0000` -> `bootloader.bin`
- `0x8000` -> `partitions.bin`
- `0x10000` -> `firmware.bin`
