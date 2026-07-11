param(
    [Parameter(Mandatory = $true)]
    [string]$SourceRoot,

    [string]$OutputFile = ""
)

$ErrorActionPreference = "Stop"
Add-Type -AssemblyName System.Drawing

if ([string]::IsNullOrWhiteSpace($OutputFile)) {
    $OutputFile = Join-Path $PSScriptRoot "..\assets\icons.c"
}

$icons = @(
    "landing-gear",
    "supercruise-decelerate",
    "hyperspace",
    "ship-lights",
    "silent-running",
    "night-vision",
    "hardpoints",
    "next-firegroup",
    "target",
    "target-next",
    "analysis-mode",
    "fss",
    "shield-power",
    "engine-power",
    "weapon-power",
    "balance-power",
    "cargo-scoop",
    "eject-cargo",
    "exploration",
    "comms-panel",
    "nav-panel",
    "trade",
    "galaxy-map",
    "system-map"
)

$width = 56
$sourceHeight = 56
$outputHeight = 44
$byteCount = [int][Math]::Ceiling(($width * $outputHeight * 2) / 8.0)
$themePath = Join-Path $SourceRoot "Sidewinder Orange"

function Convert-Icon([string]$Path) {
    if (-not (Test-Path -LiteralPath $Path)) {
        throw "Missing icon: $Path"
    }

    $source = [System.Drawing.Bitmap]::FromFile($Path)
    $scaled = New-Object System.Drawing.Bitmap(
        $width,
        $sourceHeight,
        [System.Drawing.Imaging.PixelFormat]::Format32bppArgb
    )
    $graphics = [System.Drawing.Graphics]::FromImage($scaled)
    $graphics.Clear([System.Drawing.Color]::Transparent)
    $graphics.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
    $graphics.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
    $graphics.DrawImage($source, 0, 0, $width, $sourceHeight)

    $bytes = New-Object byte[] $byteCount
    for ($y = 0; $y -lt $outputHeight; $y++) {
        for ($x = 0; $x -lt $width; $x++) {
            $pixel = $scaled.GetPixel($x, $y)
            # Four alpha levels retain thin diagonal lines without the jagged
            # edges produced by the old one-bit threshold.
            $coverage = [Math]::Min(3, [int][Math]::Round($pixel.A * 3.0 / 255.0))
            $pixelIndex = $y * $width + $x
            # Bit shift is intentional: a PowerShell [int] cast rounds a
            # fractional quotient and used to move pixels into adjacent bytes.
            $byteIndex = $pixelIndex -shr 2
            $shift = 6 - (($pixelIndex % 4) * 2)
            $bytes[$byteIndex] = $bytes[$byteIndex] -bor ($coverage -shl $shift)
        }
    }

    $graphics.Dispose()
    $scaled.Dispose()
    $source.Dispose()
    return $bytes
}

$lines = New-Object System.Collections.Generic.List[string]
$lines.Add("/* Generated from streamdeck-elite-icons commit d2f58e41. */")
$lines.Add("/* Images: CC BY-SA 3.0, Copyright (C) 2020 Keath Milligan. */")
$lines.Add('#include "icons.h"')
$lines.Add("")

foreach ($slug in $icons) {
    $identifier = $slug.Replace('-', '_')
    foreach ($state in @("normal", "active")) {
        $path = Join-Path $themePath ("{0}-{1}.png" -f $slug, $state)
        $bytes = Convert-Icon $path
        $lines.Add("const uint8_t icon_${identifier}_${state}[ICON_BYTES] = {")

        for ($offset = 0; $offset -lt $bytes.Length; $offset += 12) {
            $count = [Math]::Min(12, $bytes.Length - $offset)
            $formatted = for ($i = 0; $i -lt $count; $i++) {
                "0x{0:X2}" -f $bytes[$offset + $i]
            }
            $lines.Add("    " + ($formatted -join ", ") + ",")
        }

        $lines.Add("};")
        $lines.Add("")
    }
}

$resolvedOutput = [System.IO.Path]::GetFullPath($OutputFile)
$outputDirectory = [System.IO.Path]::GetDirectoryName($resolvedOutput)
New-Item -ItemType Directory -Force $outputDirectory | Out-Null
[System.IO.File]::WriteAllLines($resolvedOutput, $lines, [System.Text.Encoding]::ASCII)
Write-Output "Generated $resolvedOutput"
