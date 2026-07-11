$ErrorActionPreference = "Stop"

$repositoryRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".."))
$downloads = Join-Path $repositoryRoot ".downloads"
$toolchainRoot = Join-Path $repositoryRoot ".toolchain"
New-Item -ItemType Directory -Force -Path $downloads, $toolchainRoot | Out-Null

$packages = @(
    @{ Name="pico-sdk-2.2.0.tar.gz"; Url="https://github.com/raspberrypi/pico-sdk/releases/download/2.2.0/pico-sdk-2.2.0.tar.gz"; Sha256="2678FE2B176CF64A7F71CD91749FDF9134C8CF7FF84B7199DFE5EA0D6DBA6FA4" },
    @{ Name="tinyusb-86ad6e56.zip"; Url="https://github.com/hathach/tinyusb/archive/86ad6e56c1700e85f1c5678607a762cfe3aa2f47.zip"; Sha256="3011C90C128988012B553E5D2F0A90BC0B64046591C964BC1F9F6659EDCD7E4B" },
    @{ Name="lwip-77dcd25a.zip"; Url="https://github.com/lwip-tcpip/lwip/archive/77dcd25a72509eb83f72b033d219b1d40cd8eb95.zip"; Sha256="E09BF3518F1D23204CDD38203E1BDBBB5C634A1DCE8DDE487B7EACA3F2407245" },
    @{ Name="arm-gnu-toolchain-14.3.rel1.zip"; Url="https://armkeil.blob.core.windows.net/developer/Files/downloads/gnu/14.3.rel1/binrel/arm-gnu-toolchain-14.3.rel1-mingw-w64-x86_64-arm-none-eabi.zip"; Sha256="864C0C8815857D68A1BBBA2E5E2782255BB922845C71C97636004A3D74F60986" },
    @{ Name="cmake-3.28.6-windows-x86_64.zip"; Url="https://github.com/Kitware/CMake/releases/download/v3.28.6/cmake-3.28.6-windows-x86_64.zip"; Sha256="A8F2E684EAD94A64FD3517A38857A5B3F7F8D68D15C49CA1143D18797EAF9CAC" },
    @{ Name="ninja-win-1.12.1.zip"; Url="https://github.com/ninja-build/ninja/releases/download/v1.12.1/ninja-win.zip"; Sha256="F550FEC705B6D6FF58F2DB3C374C2277A37691678D6ABA463ADCBB129108467A" },
    @{ Name="pico-sdk-tools-2.2.0-x64-win.zip"; Url="https://github.com/raspberrypi/pico-sdk-tools/releases/download/v2.2.0-3/pico-sdk-tools-2.2.0-x64-win.zip"; Sha256="985D394C90B0B86B3B53EFB5B64580240AF6948B187EA484312DBA5A9A10D95A" },
    @{ Name="picotool-2.2.0-a4-x64-win.zip"; Url="https://github.com/raspberrypi/pico-sdk-tools/releases/download/v2.2.0-3/picotool-2.2.0-a4-x64-win.zip"; Sha256="A14D1088DEAAED9BB84591E4AA9FA109E60C2A8B4218B9D10556CB1209FFF622" },
    @{ Name="python-3.12.10-embed-amd64.zip"; Url="https://www.python.org/ftp/python/3.12.10/python-3.12.10-embed-amd64.zip"; Sha256="4ACBED6DD1C744B0376E3B1CF57CE906F9DC9E95E68824584C8099A63025A3C3" }
)

foreach ($package in $packages) {
    $destination = Join-Path $downloads $package.Name
    if (-not (Test-Path -LiteralPath $destination)) {
        Write-Output "Downloading $($package.Name)"
        & curl.exe -L --fail --output $destination $package.Url
        if ($LASTEXITCODE -ne 0) { throw "Download failed: $($package.Url)" }
    }
    $actual = (Get-FileHash -LiteralPath $destination -Algorithm SHA256).Hash
    if ($actual -ne $package.Sha256) {
        throw "SHA-256 mismatch for $($package.Name). Delete it and run setup again."
    }
}

function Expand-Package([string]$Archive, [string]$Destination) {
    New-Item -ItemType Directory -Force -Path $Destination | Out-Null
    & tar.exe -xf (Join-Path $downloads $Archive) -C $Destination
    if ($LASTEXITCODE -ne 0) { throw "Extraction failed: $Archive" }
}

if (-not (Test-Path (Join-Path $toolchainRoot "gcc\bin\arm-none-eabi-gcc.exe"))) {
    Expand-Package "arm-gnu-toolchain-14.3.rel1.zip" (Join-Path $toolchainRoot "gcc")
}
if (-not (Test-Path (Join-Path $toolchainRoot "cmake328\cmake-3.28.6-windows-x86_64\bin\cmake.exe"))) {
    Expand-Package "cmake-3.28.6-windows-x86_64.zip" (Join-Path $toolchainRoot "cmake328")
}
if (-not (Test-Path (Join-Path $toolchainRoot "ninja112\ninja.exe"))) {
    Expand-Package "ninja-win-1.12.1.zip" (Join-Path $toolchainRoot "ninja112")
}
if (-not (Test-Path (Join-Path $toolchainRoot "tools\pioasm\pioasm.exe"))) {
    Expand-Package "pico-sdk-tools-2.2.0-x64-win.zip" (Join-Path $toolchainRoot "tools")
}
if (-not (Test-Path (Join-Path $toolchainRoot "picotool\picotool\picotool.exe"))) {
    Expand-Package "picotool-2.2.0-a4-x64-win.zip" (Join-Path $toolchainRoot "picotool")
}
if (-not (Test-Path (Join-Path $toolchainRoot "python312\python.exe"))) {
    Expand-Package "python-3.12.10-embed-amd64.zip" (Join-Path $toolchainRoot "python312")
}

$sdkRoot = Join-Path $toolchainRoot "sdk\pico-sdk-2.2.0"
if (-not (Test-Path (Join-Path $sdkRoot "pico_sdk_init.cmake"))) {
    Expand-Package "pico-sdk-2.2.0.tar.gz" (Join-Path $toolchainRoot "sdk")
}

$tinyUsbTarget = Join-Path $sdkRoot "lib\tinyusb"
if (-not (Test-Path (Join-Path $tinyUsbTarget "src\tusb.c"))) {
    Expand-Package "tinyusb-86ad6e56.zip" (Join-Path $sdkRoot "lib")
    $tinyUsbSource = Join-Path $sdkRoot "lib\tinyusb-86ad6e56c1700e85f1c5678607a762cfe3aa2f47"
    New-Item -ItemType Directory -Force -Path $tinyUsbTarget | Out-Null
    Copy-Item -Path (Join-Path $tinyUsbSource "*") -Destination $tinyUsbTarget -Recurse -Force
}

$lwipTarget = Join-Path $sdkRoot "lib\lwip"
if (-not (Test-Path (Join-Path $lwipTarget "src\include\lwip\init.h"))) {
    Expand-Package "lwip-77dcd25a.zip" (Join-Path $sdkRoot "lib")
    $lwipSource = Join-Path $sdkRoot "lib\lwip-77dcd25a72509eb83f72b033d219b1d40cd8eb95"
    New-Item -ItemType Directory -Force -Path $lwipTarget | Out-Null
    Copy-Item -Path (Join-Path $lwipSource "*") -Destination $lwipTarget -Recurse -Force
}

Write-Output ""
Write-Output "Shared toolchain ready in $toolchainRoot"

