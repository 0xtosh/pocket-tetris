@echo off
setlocal
if "%~1"=="" (
  echo Usage: all-in-one.cmd COM18
  exit /b 1
)
powershell -ExecutionPolicy Bypass -File "%~dp0all-in-one.ps1" -Port %~1