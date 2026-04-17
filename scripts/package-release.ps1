param(
  [string]$Environment = "lilygo_t_dongle_s3"
)

$ErrorActionPreference = "Stop"
$root = Join-Path $PSScriptRoot ".."
$buildDir = Join-Path $root ".pio\build\$Environment"
$releaseDir = Join-Path $root "release\$Environment"

New-Item -ItemType Directory -Force -Path $releaseDir | Out-Null
Copy-Item (Join-Path $buildDir "bootloader.bin") (Join-Path $releaseDir "bootloader.bin") -Force
Copy-Item (Join-Path $buildDir "partitions.bin") (Join-Path $releaseDir "partitions.bin") -Force
Copy-Item (Join-Path $buildDir "firmware.bin") (Join-Path $releaseDir "firmware.bin") -Force

Write-Host "Release artifacts updated in release/$Environment"
