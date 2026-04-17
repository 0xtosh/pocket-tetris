param(
  [Parameter(Mandatory = $true)]
  [string]$Port,
  [string]$Python = ""
)

$ErrorActionPreference = "Stop"
$releaseDir = Join-Path $PSScriptRoot "..\release\lilygo_t_dongle_s3"
$bootloader = Join-Path $releaseDir "bootloader.bin"
$partitions = Join-Path $releaseDir "partitions.bin"
$firmware = Join-Path $releaseDir "firmware.bin"

if ([string]::IsNullOrWhiteSpace($Python)) {
  if (Get-Command py -ErrorAction SilentlyContinue) {
    $Python = "py"
  } else {
    $Python = "python"
  }
}

if ($Python -eq "py") {
  & py -m esptool --chip esp32s3 --port $Port --baud 921600 write_flash -z `
    0x0000 $bootloader `
    0x8000 $partitions `
    0x10000 $firmware
  exit $LASTEXITCODE
}

& $Python -m esptool --chip esp32s3 --port $Port --baud 921600 write_flash -z `
  0x0000 $bootloader `
  0x8000 $partitions `
  0x10000 $firmware

exit $LASTEXITCODE

