param(
  [string]$Environment = "lilygo_t_dongle_s3",
  [Parameter(Mandatory = $true)]
  [string]$Port
)

$ErrorActionPreference = "Stop"

& (Join-Path $PSScriptRoot "build.ps1") -Environment $Environment
if ($LASTEXITCODE -ne 0) {
  exit $LASTEXITCODE
}

& (Join-Path $PSScriptRoot "package-release.ps1") -Environment $Environment
if ($LASTEXITCODE -ne 0) {
  exit $LASTEXITCODE
}

& (Join-Path $PSScriptRoot "flash-project.ps1") -Environment $Environment -Port $Port
exit $LASTEXITCODE