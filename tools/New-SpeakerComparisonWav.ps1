#Requires -Version 7.0
[CmdletBinding()]
param(
    [string]$Path = (Join-Path $PSScriptRoot '..\assets\quiet-speaker-comparison.wav')
)

$ErrorActionPreference = 'Stop'
$rate = 48000
$frequency = 440
$peak = 197 # Signed 16-bit PCM, approximately -44.4 dBFS.
$fade = 2400
$sections = @(
    @{ Seconds = 0.75; Left = $false; Right = $false }
    @{ Seconds = 1.25; Left = $true;  Right = $false }
    @{ Seconds = 0.50; Left = $false; Right = $false }
    @{ Seconds = 1.25; Left = $false; Right = $true }
    @{ Seconds = 0.50; Left = $false; Right = $false }
    @{ Seconds = 1.25; Left = $true;  Right = $true }
    @{ Seconds = 0.50; Left = $false; Right = $false }
)
$dataBytes = [int](4 * $rate * ($sections.Seconds | Measure-Object -Sum).Sum)
$writer = [IO.BinaryWriter]::new([IO.File]::Open($Path, [IO.FileMode]::CreateNew))
try {
    $writer.Write([Text.Encoding]::ASCII.GetBytes('RIFF'))
    $writer.Write([int](36 + $dataBytes))
    $writer.Write([Text.Encoding]::ASCII.GetBytes('WAVEfmt '))
    $writer.Write([int]16)
    $writer.Write([uint16]1)
    $writer.Write([uint16]2)
    $writer.Write([int]$rate)
    $writer.Write([int](4 * $rate))
    $writer.Write([uint16]4)
    $writer.Write([uint16]16)
    $writer.Write([Text.Encoding]::ASCII.GetBytes('data'))
    $writer.Write([int]$dataBytes)
    foreach ($section in $sections) {
        $frames = [int]($rate * $section.Seconds)
        if (-not ($section.Left -or $section.Right)) {
            $writer.Write([byte[]]::new(4 * $frames))
            continue
        }
        for ($n = 0; $n -lt $frames; $n++) {
            $ramp = [Math]::Min(1.0, [Math]::Min($n, $frames - 1 - $n) / $fade)
            $value = [int16][Math]::Round($peak * $ramp * [Math]::Sin(2 * [Math]::PI * $frequency * $n / $rate))
            $writer.Write([int16]($value * [int]$section.Left))
            $writer.Write([int16]($value * [int]$section.Right))
        }
    }
}
finally {
    $writer.Dispose()
}
Get-Item -LiteralPath $Path
