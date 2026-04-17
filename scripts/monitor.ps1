param(
  [Parameter(Mandatory = $true)]
  [string]$Port,
  [int]$Baud = 115200
)

$ErrorActionPreference = "Stop"

if (Get-Command platformio -ErrorAction SilentlyContinue) {
  & platformio device monitor --port $Port --baud $Baud
  exit $LASTEXITCODE
}

if (Get-Command pio -ErrorAction SilentlyContinue) {
  & pio device monitor --port $Port --baud $Baud
  exit $LASTEXITCODE
}

if (Get-Command py -ErrorAction SilentlyContinue) {
  & py -m platformio device monitor --port $Port --baud $Baud
  exit $LASTEXITCODE
}

& python -m platformio device monitor --port $Port --baud $Baud
exit $LASTEXITCODE
