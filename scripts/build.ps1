param(
  [string]$Environment = "lilygo_t_dongle_s3"
)

$ErrorActionPreference = "Stop"

if (Get-Command platformio -ErrorAction SilentlyContinue) {
  & platformio run -e $Environment
  exit $LASTEXITCODE
}

if (Get-Command pio -ErrorAction SilentlyContinue) {
  & pio run -e $Environment
  exit $LASTEXITCODE
}

if (Get-Command py -ErrorAction SilentlyContinue) {
  & py -m platformio run -e $Environment
  exit $LASTEXITCODE
}

& python -m platformio run -e $Environment
exit $LASTEXITCODE
