param(
  [string]$Environment = "lilygo_t_dongle_s3"
)

$ErrorActionPreference = "Stop"
& (Join-Path $PSScriptRoot "build.ps1") -Environment $Environment
exit $LASTEXITCODE