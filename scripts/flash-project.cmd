@echo off
setlocal
if "%~1"=="" (
  echo Usage: flash-project.cmd COM18
  exit /b 1
)
platformio run -e lilygo_t_dongle_s3 -t upload --upload-port %~1
if not errorlevel 1 exit /b 0
pio run -e lilygo_t_dongle_s3 -t upload --upload-port %~1
if not errorlevel 1 exit /b 0
py -m platformio run -e lilygo_t_dongle_s3 -t upload --upload-port %~1
if not errorlevel 1 exit /b 0
python -m platformio run -e lilygo_t_dongle_s3 -t upload --upload-port %~1
