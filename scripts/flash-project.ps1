param(
  [string]$Environment = "lilygo_t_dongle_s3",
  [Parameter(Mandatory = $true)]
  [string]$Port
)

$ErrorActionPreference = "Stop"

if (Get-Command platformio -ErrorAction SilentlyContinue) {
  & platformio run -e $Environment -t upload --upload-port $Port
  exit $LASTEXITCODE
}

if (Get-Command pio -ErrorAction SilentlyContinue) {
  & pio run -e $Environment -t upload --upload-port $Port
  exit $LASTEXITCODE
}

if (Get-Command py -ErrorAction SilentlyContinue) {
  & py -m platformio run -e $Environment -t upload --upload-port $Port
  exit $LASTEXITCODE
}

& python -m platformio run -e $Environment -t upload --upload-port $Port
exit $LASTEXITCODE
