$ErrorActionPreference = "Stop"
$projectRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".."))
$sharedSetup = Join-Path (Split-Path $projectRoot -Parent) "tools\setup-toolchain.ps1"
if (-not (Test-Path -LiteralPath $sharedSetup)) {
    throw "Shared setup script not found: $sharedSetup"
}
& $sharedSetup

