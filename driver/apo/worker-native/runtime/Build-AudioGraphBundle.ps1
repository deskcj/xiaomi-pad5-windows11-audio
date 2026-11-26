param([Parameter(Mandatory=$true)][string]$RuntimeBundleDirectory,
      [Parameter(Mandatory=$true)][string]$ExtensionPackageDirectory,
      [Parameter(Mandatory=$true)][string]$OutputDirectory)
$ErrorActionPreference='Stop'
$audioBundle=[IO.Path]::GetFullPath($RuntimeBundleDirectory)
$audioOut=[IO.Path]::GetFullPath($OutputDirectory)
if(Test-Path -LiteralPath $audioOut){throw 'Preserve prior graph bundles; use a new directory'}
if(Test-Path -LiteralPath ($audioOut+'.zip')){throw 'Preserve prior ZIP packages'}
$audioRuntime=Get-Content -LiteralPath (Join-Path $audioBundle 'runtime\Manifest.json') -Raw | ConvertFrom-Json
if(-not $audioRuntime.AudioGraphBank -or $audioRuntime.ApoVersion -ne '2.0.0.21'){throw 'Use the qualified .21 audio-graph runtime'}
& (Join-Path $audioBundle 'Install-Runtime.ps1') -StateDirectory (Join-Path $audioOut 'unused') -ValidateOnly | Out-Null
if((Get-FileHash -LiteralPath (Join-Path $audioBundle 'APO\NabuDolbyBridge.dll')).Hash.ToLowerInvariant() -ne $audioRuntime.BridgeSha256){throw 'APO/backend DLL mismatch'}
$audioExtension=[IO.Path]::GetFullPath($ExtensionPackageDirectory)
if((Get-FileHash -LiteralPath (Join-Path $audioExtension 'NabuDolbyBound.inf')).Hash.ToLowerInvariant() -ne '622e4c38ed18ac51dc4ef063e9c9a009a132d228a4779ba86830dc6dd8a7fbe1'){throw 'Extension differs from qualified bound SFX .36'}
New-Item -ItemType Directory -Path $audioOut | Out-Null
foreach($audioName in @('runtime','APO','Install-Runtime.ps1')){Copy-Item -LiteralPath (Join-Path $audioBundle $audioName) -Destination $audioOut -Recurse}
New-Item -ItemType Directory -Path (Join-Path $audioOut 'graph') | Out-Null
Get-ChildItem -LiteralPath (Join-Path $PSScriptRoot 'graph') -File | Where-Object Name -ne 'Install-Dolby.ps1' | ForEach-Object {Copy-Item -LiteralPath $_.FullName -Destination (Join-Path $audioOut 'graph')}
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'graph\Install-Dolby.ps1') -Destination $audioOut
New-Item -ItemType Directory -Path (Join-Path $audioOut 'extension') | Out-Null
foreach($audioName in @('NabuDolbyBound.inf','nabudolbybound.cat')){Copy-Item -LiteralPath (Join-Path $audioExtension $audioName) -Destination (Join-Path $audioOut 'extension')}
$audioNote=@'
Experimental original Dolby stereo DAP add-on for Xiaomi Pad 5 Windows ARM64.
Requires the qualified .20 amplifier/.4 APO/Qualcomm 1.0.2100.0 baseline.
Includes the matching signed APO, bound speaker SFX extension and standalone
LocalService backend. Installs SCM Automatic without changing kernel drivers,
calibration, gain, protection or firmware. Atmos objects and MiSound are absent.
Use elevated native PowerShell and keep package/rollback on SSD:
  .\Install-Dolby.ps1 -StateDirectory H:\your-new-dolby-state
Read-only validation:
  .\Install-Dolby.ps1 -StateDirectory H:\another-new-state -ValidateOnly
Restore captured original drivers, settings and service configuration:
  .\Install-Dolby.ps1 -StateDirectory H:\your-new-dolby-state -Restore
Installation only establishes bindings. Cold boot and audio quality require
separate qualification. Every failure and exact rollback are saved on SSD.
Never describe this add-on as a complete audio-driver or finished Atmos package.
'@
$audioNote | Set-Content -LiteralPath (Join-Path $audioOut 'README.txt') -Encoding UTF8
$audioFiles=[ordered]@{}
Get-ChildItem -LiteralPath $audioOut -Recurse -File | ForEach-Object {
    $audioFiles[$_.FullName.Substring($audioOut.Length+1).Replace('\','/')]=(Get-FileHash -LiteralPath $_.FullName).Hash.ToLowerInvariant()
}
$audioManifest=[ordered]@{Schema=1;PackageKind='ExperimentalDolbyGraphAddon';ApoVersion=$audioRuntime.ApoVersion;RuntimeVersion=$audioRuntime.RuntimeVersion;SourceFingerprint=$audioRuntime.SourceFingerprint;SignerThumbprint='F38EDD5C105095C8342556158CE66253F045965D';BaselineApoDllSha256='dc0c29a144090f83b4532c215332f1a0c96aa4c99ca9db1ccdbc1bb5c117889c';BaselineExtensionInfSha256='2a23915a596e36ea08a493bf9b0155b594fd2f4cb069077d8ea83c6abe58700e';BaselineKernelSha256='14016bfc988faca26577ad9aa0317a0d1a0d4e68313a460a22898352d4722f34';Files=$audioFiles}
$audioManifest | ConvertTo-Json -Depth 9 | Set-Content -LiteralPath (Join-Path $audioOut 'GraphPackage.json') -Encoding UTF8
& (Join-Path $audioOut 'graph\Verify-GraphPackage.ps1') -Root $audioOut | Out-Null
Add-Type -AssemblyName System.IO.Compression.FileSystem
[IO.Compression.ZipFile]::CreateFromDirectory($audioOut,$audioOut+'.zip')
[ordered]@{Package=$audioOut;Zip=$audioOut+'.zip';Sha256=(Get-FileHash -LiteralPath ($audioOut+'.zip')).Hash.ToLowerInvariant();Bytes=(Get-Item -LiteralPath ($audioOut+'.zip')).Length;KernelIncluded=$false;GraphInstallerIncluded=$true;PlaybackQualified=$false} | ConvertTo-Json
