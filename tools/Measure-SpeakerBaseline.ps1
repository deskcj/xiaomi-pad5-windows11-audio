#Requires -Version 7.0
[CmdletBinding()]
param([switch]$PlayQuietTones)

$ErrorActionPreference = 'Stop'
if (-not $IsWindows -or -not [Environment]::Is64BitProcess) {
    throw 'Run this tool in 64-bit PowerShell on Windows.'
}
if (-not $PlayQuietTones) {
    throw 'Pass -PlayQuietTones to play two quiet stereo test tones through the built-in speakers.'
}
if (-not ('NabuAudio.QuietSpeakerCheck' -as [type])) {
    Add-Type -Path (Join-Path $PSScriptRoot 'QuietSpeakerCheck.cs')
}

[NabuAudio.QuietSpeakerCheck]::Run()
