@echo off
setlocal
powershell -ExecutionPolicy Bypass -File "%~dp0package-release.ps1"