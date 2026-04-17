@echo off
setlocal
if "%~1"=="" (
  echo Usage: flash-prebuilt.cmd COM18
  exit /b 1
)
py -m esptool --chip esp32s3 --port %~1 --baud 921600 write_flash -z ^
  0x0000 release\lilygo_t_dongle_s3\bootloader.bin ^
  0x8000 release\lilygo_t_dongle_s3\partitions.bin ^
  0x10000 release\lilygo_t_dongle_s3\firmware.bin
if not errorlevel 1 exit /b 0
python -m esptool --chip esp32s3 --port %~1 --baud 921600 write_flash -z ^
  0x0000 release\lilygo_t_dongle_s3\bootloader.bin ^
  0x8000 release\lilygo_t_dongle_s3\partitions.bin ^
  0x10000 release\lilygo_t_dongle_s3\firmware.bin
