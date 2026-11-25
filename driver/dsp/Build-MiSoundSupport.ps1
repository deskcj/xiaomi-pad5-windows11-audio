param([string]$OutputDirectory=(Join-Path $PSScriptRoot 'ARM64\Lab'))
$ErrorActionPreference='Stop'
$compiler=(Get-Command cl.exe -ErrorAction Stop).Source
$librarian=(Get-Command lib.exe -ErrorAction Stop).Source
if(-not $env:INCLUDE -or -not $env:LIB){throw 'Run from an ARM64 EWDK build environment.'}
$out=[IO.Path]::GetFullPath($OutputDirectory)
if(Test-Path -LiteralPath $out){throw 'Choose a new output folder to retain previous builds.'}
New-Item -ItemType Directory -Path $out | Out-Null
$objects=@()
foreach($stem in @('MiSoundProtocol','MiSoundSpeakerFilters')){
    $object=Join-Path $out ($stem+'.obj')
    $log=Join-Path $out ($stem+'.log')
    & $compiler /nologo /c /std:c11 /MT /W4 /WX /O2 /GS /guard:cf ('/Fo'+$object) (Join-Path $PSScriptRoot ($stem+'.c')) *> $log
    if($LASTEXITCODE -ne 0){throw "Compile failed: $log"}
    if([BitConverter]::ToUInt16([IO.File]::ReadAllBytes($object),0) -ne 0xAA64){throw 'Object is not ARM64; select the correct EWDK target.'}
    $objects+=$object
}
$library=Join-Path $out 'NabuMiSoundSupport.lib'
& $librarian /nologo /machine:arm64 ('/out:'+$library) @objects *> (Join-Path $out 'Archive.log')
if($LASTEXITCODE -ne 0){throw 'Archive failed; inspect Archive.log.'}
$hashes=[ordered]@{}
Get-ChildItem -LiteralPath $PSScriptRoot -File | Where-Object Extension -in @('.c','.h','.ps1') | Sort-Object Name | ForEach-Object {$hashes[$_.Name]=(Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash.ToLowerInvariant()}
[ordered]@{Architecture='ARM64';SourceSha256=$hashes;LibrarySha256=(Get-FileHash -LiteralPath $library).Hash.ToLowerInvariant();Installed=$false;DspProcessingImplemented=$false} | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $out 'Build.json') -Encoding utf8
Write-Output $library
