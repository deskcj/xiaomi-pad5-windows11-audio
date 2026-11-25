param(
    [Parameter(Mandatory=$true)][string]$UnicornIncludeDirectory,
    [Parameter(Mandatory=$true)][string]$OutputDirectory
)
$ErrorActionPreference='Stop'
$compiler=(Get-Command cl.exe -ErrorAction Stop).Source
$linker=(Get-Command link.exe -ErrorAction Stop).Source
if(-not $env:INCLUDE -or -not $env:LIB){throw 'Run in an ARM64 EWDK build environment.'}
$headers=[IO.Path]::GetFullPath($UnicornIncludeDirectory)
if(-not(Test-Path -LiteralPath (Join-Path $headers 'unicorn\unicorn.h'))){throw 'Unicorn 2.1.4 headers are required.'}
$out=[IO.Path]::GetFullPath($OutputDirectory)
if(Test-Path -LiteralPath $out){throw 'Choose a new output directory to retain preceding builds.'}
New-Item -ItemType Directory -Path $out | Out-Null
$object=Join-Path $out 'NativeLevelScale.obj'
& $compiler /nologo /c /std:c++17 /MT /W4 /WX /wd4505 /O2 /GS /guard:cf /GR- /D_WIN32_WINNT=0x0A00 ('/I'+$headers) ('/Fo'+$object) (Join-Path $PSScriptRoot 'NativeLevelScale.cpp') *> (Join-Path $out 'Compile.log')
if($LASTEXITCODE -ne 0){throw 'Level-scale compilation failed; see Compile.log.'}
if([BitConverter]::ToUInt16([IO.File]::ReadAllBytes($object),0) -ne 0xAA64){throw 'Object is not ARM64.'}
$dll=Join-Path $out 'NativeLevelScale.dll'
& $linker /nologo /machine:arm64 /guard:cf /dynamicbase /nxcompat /manifest:no /dll ('/out:'+$dll) $object kernel32.lib *> (Join-Path $out 'Link.log')
if($LASTEXITCODE -ne 0){throw 'Level-scale linking failed; see Link.log.'}
$hashes=[ordered]@{}
foreach($name in @('NativeLevelScale.cpp','NativeLevelScale.h','Build-NativeLevelScale.ps1')){
    $hashes[$name]=(Get-FileHash -LiteralPath (Join-Path $PSScriptRoot $name) -Algorithm SHA256).Hash.ToLowerInvariant()
}
[ordered]@{Architecture='ARM64';Prototype='native-level-scale-v1';SourceSha256=$hashes;
    DllSha256=(Get-FileHash -LiteralPath $dll -Algorithm SHA256).Hash.ToLowerInvariant();
    Registered=$false;DriverInstalled=$false;AudioOpened=$false;SystemDolbyProven=$false
} | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $out 'Build.json') -Encoding utf8
Write-Output $dll
