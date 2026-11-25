param([ValidateSet('QuietStereoProbe','QuietContinuityProbe','ApoSettingsProbe','ApoClassActivationProbe','ApoSecurityProbe','ApoFactoryContractProbe')][string]$Probe='QuietStereoProbe',
    [Parameter(Mandatory=$true)][string]$OutputDirectory)
$ErrorActionPreference='Stop'
$compiler=(Get-Command cl.exe -ErrorAction Stop).Source
$linker=(Get-Command link.exe -ErrorAction Stop).Source
if(-not $env:INCLUDE -or -not $env:LIB){throw 'Use an ARM64 EWDK build environment.'}
$out=[IO.Path]::GetFullPath($OutputDirectory)
if(Test-Path -LiteralPath $out){throw 'Choose a new directory to preserve earlier measurements.'}
New-Item -ItemType Directory -Path $out | Out-Null
$source=Join-Path $PSScriptRoot ($Probe+'.cpp')
$object=Join-Path $out ($Probe+'.obj')
$exe=Join-Path $out ($Probe+'.exe')
& $compiler /nologo /c /std:c++17 /O2 /MT /W4 /WX /GS /guard:cf ('/Fo'+$object) $source *> (Join-Path $out 'Compile.log')
if($LASTEXITCODE -ne 0){throw 'See Compile.log'}
& $linker /nologo /machine:arm64 /guard:cf /dynamicbase /nxcompat /manifest:no ('/out:'+$exe) $object kernel32.lib ole32.lib uuid.lib advapi32.lib *> (Join-Path $out 'Link.log')
if($LASTEXITCODE -ne 0){throw 'See Link.log'}
[ordered]@{
    Architecture='ARM64';Probe=$Probe;SourceSha256=(Get-FileHash -LiteralPath $source).Hash.ToLowerInvariant()
    ExeSha256=(Get-FileHash -LiteralPath $exe).Hash.ToLowerInvariant()
    SourcePeak=$(if($Probe -in 'QuietStereoProbe','QuietContinuityProbe'){0.001}else{0})
    OwnedStreamGain=$(if($Probe -in 'QuietStereoProbe','QuietContinuityProbe'){0.01}else{$null})
    SourcePeakTimesStreamGain=$(if($Probe -in 'QuietStereoProbe','QuietContinuityProbe'){0.00001}else{0})
    AudioStartedDuringBuild=$false;EndpointVolumeWritten=$false;AcousticStereoProven=$false
} | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $out 'Build.json') -Encoding utf8
