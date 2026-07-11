param(
    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Release"
)

$ErrorActionPreference = "Stop"
$projectRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".."))
$candidates = @(
    (Join-Path $projectRoot ".toolchain"),
    (Join-Path (Split-Path $projectRoot -Parent) ".toolchain"),
    (Join-Path (Split-Path (Split-Path $projectRoot -Parent) -Parent) ".toolchain")
)
$toolchainRoot = $null
foreach ($candidate in $candidates) {
    if (Test-Path (Join-Path $candidate "sdk\pico-sdk-2.2.0\pico_sdk_init.cmake")) {
        $toolchainRoot = $candidate
        break
    }
}
if (-not $toolchainRoot) { $toolchainRoot = $candidates[1] }
$sdk = Join-Path $toolchainRoot "sdk\pico-sdk-2.2.0"
$toolchain = Join-Path $toolchainRoot "gcc"
$cmake = Join-Path $toolchainRoot "cmake328\cmake-3.28.6-windows-x86_64\bin\cmake.exe"
$ninja = Join-Path $toolchainRoot "ninja112\ninja.exe"
$pioasm = Join-Path $toolchainRoot "tools\pioasm"
$picotool = Join-Path $toolchainRoot "picotool\picotool"
$python = Join-Path $toolchainRoot "python312\python.exe"
$build = Join-Path $projectRoot "build-rp2040"
$dist = Join-Path $projectRoot "dist"
$cacheStamp = Join-Path $build ".picodeck-toolchain-root"
$toolchainIdentity = [System.IO.Path]::GetFullPath($toolchainRoot)

$required = @(
    $sdk,
    (Join-Path $toolchain "bin\arm-none-eabi-gcc.exe"),
    $cmake,
    $ninja,
    (Join-Path $pioasm "pioasm.exe"),
    (Join-Path $picotool "picotool.exe"),
    $python
)
foreach ($path in $required) {
    if (-not (Test-Path -LiteralPath $path)) {
        throw "Missing build dependency: $path"
    }
}

$env:PICO_SDK_PATH = $sdk
$env:PICO_TOOLCHAIN_PATH = $toolchain
$env:Path = (Join-Path $toolchain "bin") + ";" + $env:Path

if (Test-Path -LiteralPath $build) {
    $cacheMatches = (Test-Path -LiteralPath $cacheStamp) -and
        ((Get-Content -Raw -LiteralPath $cacheStamp) -eq $toolchainIdentity)
    if (-not $cacheMatches) {
        $expectedBuild = [System.IO.Path]::GetFullPath((Join-Path $projectRoot "build-rp2040"))
        $resolvedBuild = [System.IO.Path]::GetFullPath($build)
        if ($resolvedBuild -ne $expectedBuild) {
            throw "Refusing to clean unexpected build directory: $resolvedBuild"
        }
        Write-Output "Toolchain location changed; cleaning stale CMake cache in $resolvedBuild"
        Remove-Item -LiteralPath $resolvedBuild -Recurse -Force
    }
}

New-Item -ItemType Directory -Force $build, $dist | Out-Null

& $cmake `
    -S $projectRoot `
    -B $build `
    -G Ninja `
    "-DCMAKE_BUILD_TYPE=$Configuration" `
    "-DCMAKE_MAKE_PROGRAM=$ninja" `
    "-DPICO_SDK_PATH=$sdk" `
    "-DPICO_TOOLCHAIN_PATH=$toolchain" `
    "-Dpioasm_DIR=$pioasm" `
    "-Dpicotool_DIR=$picotool" `
    "-DPython3_EXECUTABLE=$python" `
    -DPICO_BOARD=pico
if ($LASTEXITCODE -ne 0) {
    throw "CMake configuration failed with exit code $LASTEXITCODE"
}
Set-Content -LiteralPath $cacheStamp -Value $toolchainIdentity -NoNewline -Encoding ASCII

& $cmake --build $build --config $Configuration
if ($LASTEXITCODE -ne 0) {
    throw "Build failed with exit code $LASTEXITCODE"
}

$uf2 = Join-Path $build "picodeck.uf2"
if (-not (Test-Path -LiteralPath $uf2)) {
    throw "Build completed but picodeck.uf2 was not produced"
}

$destination = Join-Path $dist "PicoDeck-ED-Keyboard.uf2"
Copy-Item -LiteralPath $uf2 -Destination $destination -Force
$repositoryRoot = Split-Path $projectRoot -Parent
Copy-Item -LiteralPath (Join-Path $repositoryRoot "LICENSE") `
    -Destination (Join-Path $dist "LICENSE") -Force
Copy-Item -LiteralPath (Join-Path $repositoryRoot "THIRD_PARTY_NOTICES.md") `
    -Destination (Join-Path $dist "THIRD_PARTY_NOTICES.md") -Force
Copy-Item -LiteralPath (Join-Path $projectRoot "assets\UPSTREAM_LICENSE.md") `
    -Destination (Join-Path $dist "ICON_UPSTREAM_LICENSE.md") -Force
Write-Output ""
Write-Output "Firmware ready: $destination"
