param([Parameter(Mandatory=$true)][string]$OutputDirectory,
    [Parameter(Mandatory=$true)][ValidatePattern('^[0-9a-fA-F]{40}$')][string]$CertificateThumbprint,
    [Parameter(Mandatory=$true)][string]$KitBinDirectory)
$ErrorActionPreference='Stop'
$out=[IO.Path]::GetFullPath($OutputDirectory)
if(Test-Path -LiteralPath $out){throw 'Choose a new output directory to retain earlier packages.'}
$kit=[IO.Path]::GetFullPath($KitBinDirectory)
$signTool=Join-Path $kit 'arm64\signtool.exe'
$inf2Cat=Join-Path $kit 'x86\Inf2Cat.exe'
if(-not (Test-Path -LiteralPath $signTool) -or -not (Test-Path -LiteralPath $inf2Cat)){throw 'Provide the EWDK versioned Windows Kits bin directory.'}
New-Item -ItemType Directory -Path $out | Out-Null
& (Join-Path $PSScriptRoot 'Build-Apo.ps1') -OutputDirectory (Join-Path $out 'unsigned-build')
$build=Get-Content -LiteralPath (Join-Path $out 'unsigned-build\Build.json') -Raw | ConvertFrom-Json
$infText=[IO.File]::ReadAllText((Join-Path $PSScriptRoot 'NabuDolbyApo.inf'))
$versionMatch=[regex]::Match($infText,'(?m)^DriverVer=[^\r\n]*,([0-9]+\.[0-9]+\.[0-9]+\.[0-9]+)\s*$')
if(-not $versionMatch.Success -or $build.CandidateVersion -ne ($versionMatch.Groups[1].Value+'-lab')){throw 'INF version and compiled source tag disagree.'}
$packageVersion=$versionMatch.Groups[1].Value
$package=Join-Path $out 'package'
New-Item -ItemType Directory -Path $package | Out-Null
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'NabuDolbyApo.inf') -Destination $package
$unsigned=Join-Path $out 'unsigned-build\NabuDolbyBridge.dll'
$signed=Join-Path $package 'NabuDolbyBridge.dll'
Copy-Item -LiteralPath $unsigned -Destination $signed
& $signTool sign /fd SHA256 /ph /sha1 $CertificateThumbprint /s My $signed *> (Join-Path $out 'SignDll.log')
if($LASTEXITCODE -ne 0){throw 'Embedded DLL signing failed.'}
& $signTool verify /pa $signed *> (Join-Path $out 'VerifyDll.log')
if($LASTEXITCODE -ne 0){throw 'Embedded DLL signature verification failed.'}
# Require the PE certificate table: a catalog-only signature cannot pass this check.
$image=[IO.File]::ReadAllBytes($signed)
$pe=[BitConverter]::ToUInt32($image,0x3c)
if([BitConverter]::ToUInt16($image,$pe+24) -ne 0x20b){throw 'Expected a PE32+ APO.'}
$certificateOffset=[BitConverter]::ToUInt32($image,$pe+24+144)
$certificateBytes=[BitConverter]::ToUInt32($image,$pe+24+148)
if(-not $certificateOffset -or $certificateBytes -lt 8 -or [uint64]$certificateOffset+$certificateBytes -gt $image.Length){throw 'Missing or invalid embedded certificate table.'}
& $inf2Cat ('/driver:'+$package) /os:10_GE_ARM64 /uselocaltime *> (Join-Path $out 'Inf2Cat.log')
if($LASTEXITCODE -ne 0){throw 'Inf2Cat failed.'}
$catalog=Join-Path $package 'NabuDolbyApo.cat'
& $signTool sign /fd SHA256 /sha1 $CertificateThumbprint /s My $catalog *> (Join-Path $out 'SignCatalog.log')
if($LASTEXITCODE -ne 0){throw 'Catalog signing failed.'}
& $signTool verify /pa $catalog *> (Join-Path $out 'VerifyCatalog.log')
if($LASTEXITCODE -ne 0){throw 'Catalog signature verification failed.'}
foreach($file in @('NabuDolbyApo.inf','NabuDolbyBridge.dll')){
    & $signTool verify /pa /c $catalog (Join-Path $package $file) *> (Join-Path $out ($file+'.Member.log'))
    if($LASTEXITCODE -ne 0){throw "Catalog member verification failed: $file"}
}
$files=[ordered]@{}
Get-ChildItem -LiteralPath $package -File | ForEach-Object {$files[$_.Name]=(Get-FileHash -LiteralPath $_.FullName).Hash.ToLowerInvariant()}
[ordered]@{
    Architecture='ARM64';PackageVersion=$packageVersion;EmbeddedDllSignatureVerified=$true
    EmbeddedCertificateBytes=$certificateBytes;CatalogAndMembersVerified=$true
    UnsignedDllSha256=(Get-FileHash -LiteralPath $unsigned).Hash.ToLowerInvariant()
    Files=$files;Installed=$false;SystemEffectsProven=$false
} | ConvertTo-Json -Depth 3 | Set-Content -LiteralPath (Join-Path $out 'Package.json') -Encoding utf8
