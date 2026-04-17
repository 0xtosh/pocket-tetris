@echo off
setlocal
if "%~1"=="" (
  echo Usage: monitor.cmd COM18
  exit /b 1
)
platformio device monitor --port %~1 --baud 115200
if not errorlevel 1 exit /b 0
pio device monitor --port %~1 --baud 115200
if not errorlevel 1 exit /b 0
py -m platformio device monitor --port %~1 --baud 115200
if not errorlevel 1 exit /b 0
python -m platformio device monitor --port %~1 --baud 115200
