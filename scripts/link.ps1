param(
  [string]$Environment = "lilygo_t_dongle_s3"
)

$ErrorActionPreference = "Stop"
& (Join-Path $PSScriptRoot "build.ps1") -Environment $Environment
if ($LASTEXITCODE -ne 0) {
  exit $LASTEXITCODE
}

$elfPath = Join-Path $PSScriptRoot "..\.pio\build\$Environment\firmware.elf"
if (-not (Test-Path $elfPath)) {
  Write-Error "Expected linked artifact not found: .pio/build/$Environment/firmware.elf"
}

exit 0