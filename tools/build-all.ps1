param(
    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Release"
)

$ErrorActionPreference = "Stop"
$repositoryRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".."))
$projects = @(
    @{ Name = "PicoDeck-ED-Display"; Firmware = "PicoDeck-ED-Display.uf2" },
    @{ Name = "PicoDeck-ED-Keyboard"; Firmware = "PicoDeck-ED-Keyboard.uf2" }
)

foreach ($project in $projects) {
    $projectRoot = Join-Path $repositoryRoot $project.Name
    $buildScript = Join-Path $projectRoot "tools\build.ps1"
    Write-Output ""
    Write-Output "Building $($project.Name) [$Configuration]"
    & $buildScript -Configuration $Configuration
    if ($LASTEXITCODE -ne 0) {
        throw "$($project.Name) build failed with exit code $LASTEXITCODE"
    }
}

$dist = Join-Path $repositoryRoot "dist"
New-Item -ItemType Directory -Force -Path $dist | Out-Null
foreach ($project in $projects) {
    $source = Join-Path (Join-Path $repositoryRoot $project.Name) `
                        (Join-Path "dist" $project.Firmware)
    if (-not (Test-Path -LiteralPath $source)) {
        throw "Expected firmware was not produced: $source"
    }
    Copy-Item -LiteralPath $source -Destination (Join-Path $dist $project.Firmware) -Force
}

$releaseFiles = @(
    @{ Source = (Join-Path $repositoryRoot "LICENSE"); Name = "LICENSE" },
    @{ Source = (Join-Path $repositoryRoot "THIRD_PARTY_NOTICES.md"); Name = "THIRD_PARTY_NOTICES.md" },
    @{ Source = (Join-Path $repositoryRoot "PicoDeck-ED-Keyboard\assets\UPSTREAM_LICENSE.md"); Name = "ICON_UPSTREAM_LICENSE.md" }
)
foreach ($file in $releaseFiles) {
    if (-not (Test-Path -LiteralPath $file.Source)) {
        throw "Expected release file was not found: $($file.Source)"
    }
    Copy-Item -LiteralPath $file.Source -Destination (Join-Path $dist $file.Name) -Force
}

Write-Output ""
Write-Output "Both firmware images and release notices are ready in $dist"
