@echo off
setlocal
platformio run -e lilygo_t_dongle_s3
if not errorlevel 1 exit /b 0
pio run -e lilygo_t_dongle_s3
if not errorlevel 1 exit /b 0
py -m platformio run -e lilygo_t_dongle_s3
if not errorlevel 1 exit /b 0
python -m platformio run -e lilygo_t_dongle_s3
