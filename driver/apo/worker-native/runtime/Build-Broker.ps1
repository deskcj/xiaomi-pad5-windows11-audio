param([Parameter(Mandatory=$true)][string]$OutputDirectory)
$ErrorActionPreference='Stop'
$compiler=(Get-Command cl.exe -ErrorAction Stop).Source
$linker=(Get-Command link.exe -ErrorAction Stop).Source
if(-not $env:INCLUDE -or -not $env:LIB){throw 'Use an ARM64 EWDK environment.'}
$out=[IO.Path]::GetFullPath($OutputDirectory)
if(Test-Path -LiteralPath $out){throw 'Choose a new output directory to retain previous builds.'}
New-Item -ItemType Directory -Path $out | Out-Null
$source=Join-Path $PSScriptRoot 'NabuDolbyBroker.cpp'
$object=Join-Path $out 'NabuDolbyBroker.obj'
& $compiler /nologo /c /std:c++17 /EHsc /MT /O2 /W4 /WX /GS /guard:cf ('/Fo'+$object) $source *> (Join-Path $out 'Compile.log')
if($LASTEXITCODE -ne 0){throw 'See Compile.log'}
$exe=Join-Path $out 'NabuDolbyBroker.exe'
& $linker /nologo /machine:arm64 /guard:cf /dynamicbase /nxcompat /manifest:no ('/out:'+$exe) $object kernel32.lib ole32.lib advapi32.lib *> (Join-Path $out 'Link.log')
if($LASTEXITCODE -ne 0){throw 'See Link.log'}
[ordered]@{Architecture='ARM64';ExeSha256=(Get-FileHash -LiteralPath $exe).Hash.ToLowerInvariant();SourceSha256=(Get-FileHash -LiteralPath $source).Hash.ToLowerInvariant();OpensAudio=$false;InstallsDrivers=$false;StartupRegistered=$false} | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $out 'Build.json') -Encoding utf8
