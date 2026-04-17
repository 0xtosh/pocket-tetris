@echo off
setlocal
powershell -ExecutionPolicy Bypass -File "%~dp0compile.ps1" %*