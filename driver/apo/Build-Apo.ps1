param([string]$OutputDirectory=(Join-Path $PSScriptRoot 'ARM64\Lab'))
$ErrorActionPreference='Stop'
$compiler=(Get-Command cl.exe -ErrorAction Stop).Source
$linker=(Get-Command link.exe -ErrorAction Stop).Source
if(-not $env:INCLUDE -or -not $env:LIB){throw 'Run from an EWDK build environment targeting ARM64.'}
$out=[IO.Path]::GetFullPath($OutputDirectory)
if(Test-Path -LiteralPath $out){throw 'Choose a new output directory to preserve previous builds.'}
New-Item -ItemType Directory -Path $out | Out-Null
$sourceFiles=Get-ChildItem -LiteralPath $PSScriptRoot -File | Where-Object Extension -in @('.cpp','.h') | Sort-Object Name
$identityBytes=[IO.MemoryStream]::new()
foreach($sourceFile in $sourceFiles){
    $canonical=$sourceFile.Name+"`n"+[IO.File]::ReadAllText($sourceFile.FullName).Replace("`r`n","`n")+[char]0
    $bytes=[Text.Encoding]::UTF8.GetBytes($canonical)
    $identityBytes.Write($bytes,0,$bytes.Length)
}
$sha=[Security.Cryptography.SHA256]::Create()
try{$sourceFingerprint=[BitConverter]::ToString($sha.ComputeHash($identityBytes.ToArray())).Replace('-','').ToLowerInvariant()}finally{$sha.Dispose();$identityBytes.Dispose()}
$versionMatch=[regex]::Match([IO.File]::ReadAllText((Join-Path $PSScriptRoot 'NabuDolbyApo.inf')),'(?m)^DriverVer=[^\r\n]*,([0-9]+\.[0-9]+\.[0-9]+\.[0-9]+)\s*$')
if(-not $versionMatch.Success){throw 'Missing INF candidate version'}
$candidateVersion=$versionMatch.Groups[1].Value+'-lab'
$identityHeader="#pragma once`ninline constexpr wchar_t NabuApoBuildTag[]=L`"$candidateVersion`";`ninline constexpr wchar_t NabuApoSourceFingerprint[]=L`"$sourceFingerprint`";`n"
[IO.File]::WriteAllText((Join-Path $out 'ApoBuildIdentity.h'),$identityHeader,[Text.UTF8Encoding]::new($false))
$objects=@()
foreach($stem in @('NabuDolbyApo','DolbySharedTransport','DolbyBank','ApoDiagnostics','DigitalHarness')){
    $object=Join-Path $out ($stem+'.obj')
    $log=Join-Path $out ($stem+'.log')
    & $compiler /nologo /c /std:c++17 /MT /W4 /WX /wd4505 /O2 /GS /guard:cf /GR- /D_WIN32_WINNT=0x0A00 ('/I'+$out) ('/Fo'+$object) (Join-Path $PSScriptRoot ($stem+'.cpp')) *> $log
    if($LASTEXITCODE -ne 0){throw "Compile failed: $log"}
    $objects+=$object
}
$dll=Join-Path $out 'NabuDolbyBridge.dll'
$linkLog=Join-Path $out 'Link.log'
& $linker /nologo /machine:arm64 /guard:cf /dynamicbase /nxcompat /manifest:no /dll '/export:DllGetClassObject,PRIVATE' '/export:DllCanUnloadNow,PRIVATE' ('/out:'+$dll) @objects kernel32.lib ole32.lib advapi32.lib *> $linkLog
if($LASTEXITCODE -ne 0){throw "Link failed: $linkLog"}
$hashes=[ordered]@{}
Get-ChildItem -LiteralPath $PSScriptRoot -File | Where-Object Extension -in @('.cpp','.h','.ps1') | ForEach-Object {$hashes[$_.Name]=(Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash.ToLowerInvariant()}
[ordered]@{
    CandidateVersion=$candidateVersion;Architecture='ARM64';Registered=$false
    SourceFingerprint=$sourceFingerprint
    Signed=$false;SystemEffectsProven=$false
    DllSha256=(Get-FileHash -LiteralPath $dll -Algorithm SHA256).Hash.ToLowerInvariant()
    SourceSha256=$hashes
} | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $out 'Build.json') -Encoding utf8
Write-Output $dll
