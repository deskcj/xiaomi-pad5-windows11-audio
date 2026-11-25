param([Parameter(Mandatory=$true)][string]$RuntimeDirectory,[Parameter(Mandatory=$true)][string]$ApoPackageDirectory,
    [Parameter(Mandatory=$true)][string]$OutputDirectory,[Parameter(Mandatory=$true)][ValidatePattern('^1\.0\.0-lab[0-9]+$')][string]$RuntimeVersion)
$ErrorActionPreference='Stop'
$audioRoot=[IO.Path]::GetFullPath($RuntimeDirectory).TrimEnd('\')+'\'
$audioOut=[IO.Path]::GetFullPath($OutputDirectory)
if(Test-Path -LiteralPath $audioOut){throw 'Retain previous bundles; select a new output directory'}
$manifest=Get-Content -LiteralPath (Join-Path $audioRoot 'Manifest.json') -Raw | ConvertFrom-Json
foreach($item in $manifest.Files.PSObject.Properties){
    $path=[IO.Path]::GetFullPath((Join-Path $audioRoot $item.Name))
    if(-not $path.StartsWith($audioRoot,[StringComparison]::OrdinalIgnoreCase)){throw 'Manifest path escapes runtime'}
    if((Get-FileHash -LiteralPath $path).Hash.ToLowerInvariant() -ne $item.Value){throw ('Runtime hash mismatch: '+$item.Name)}
}
foreach($name in 'NabuDolbyService.exe','NabuDolbyBroker.exe','NabuDolbyBridge.dll'){
    if((Get-AuthenticodeSignature -LiteralPath (Join-Path $audioRoot $name)).Status -ne 'Valid'){throw ('Invalid signature: '+$name)}
}
$component=[IO.Path]::GetFullPath($ApoPackageDirectory)
if((Get-FileHash -LiteralPath (Join-Path $component 'NabuDolbyBridge.dll')).Hash.ToLowerInvariant() -ne $manifest.BridgeSha256){throw 'Component and runtime use different APO binaries'}
New-Item -ItemType Directory -Path (Join-Path $audioOut 'runtime') | Out-Null
foreach($item in $manifest.Files.PSObject.Properties){
    $dest=Join-Path (Join-Path $audioOut 'runtime') $item.Name
    New-Item -ItemType Directory -Path (Split-Path -Parent $dest) -Force | Out-Null
    Copy-Item -LiteralPath (Join-Path $audioRoot $item.Name) -Destination $dest
}
# A unique smoke bank keeps this runtime-only installer separate from audio
# registration. The signed APO component is retained for a controlled graph trial.
$bank='Global\NabuDolbyBank-ServiceSmoke-'+[Guid]::NewGuid().ToString()
$ini="[Runtime]`nSourceFingerprint=$($manifest.SourceFingerprint)`nBridgeSha256=$($manifest.BridgeSha256)`nBankName=$bank`n"
[IO.File]::WriteAllText((Join-Path $audioOut 'runtime\Runtime.ini'),$ini,[Text.UTF8Encoding]::new($false))
$manifest.Files.'Runtime.ini'=(Get-FileHash -LiteralPath (Join-Path $audioOut 'runtime\Runtime.ini')).Hash.ToLowerInvariant()
if(-not $manifest.PSObject.Properties['RuntimeVersion']){$manifest | Add-Member -NotePropertyName RuntimeVersion -NotePropertyValue $RuntimeVersion}else{$manifest.RuntimeVersion=$RuntimeVersion}
$manifest.BankName=$bank
$manifest | ConvertTo-Json -Depth 9 | Set-Content -LiteralPath (Join-Path $audioOut 'runtime\Manifest.json') -Encoding UTF8
New-Item -ItemType Directory -Path (Join-Path $audioOut 'APO') | Out-Null
foreach($name in 'NabuDolbyApo.inf','NabuDolbyBridge.dll','nabudolbyapo.cat'){Copy-Item -LiteralPath (Join-Path $component $name) -Destination (Join-Path $audioOut 'APO')}
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'Install-Runtime.ps1') -Destination $audioOut
$note=@'
Experimental ARM64 original Dolby stereo DAP runtime and signed APO candidate.
Install-Runtime.ps1 installs only the portable LocalService backend, in Manual
start mode. It does not install the APO, bind Windows effects, configure automatic
startup or establish Atmos object rendering. Those integration steps remain open.
Keep the package and installation state on SSD. Use elevated native PowerShell:
  .\Install-Runtime.ps1 -StateDirectory H:\your-new-state-folder
Rollback restores the previous owned service configuration and retains both
runtime directories:
  .\Install-Runtime.ps1 -StateDirectory H:\your-new-state-folder -Restore
No endpoint, amplifier settings, driver binding or Windows reboot is touched.
'@
$note | Set-Content -LiteralPath (Join-Path $audioOut 'README.txt') -Encoding UTF8
$hashes=[ordered]@{}
Get-ChildItem -LiteralPath $audioOut -Recurse -File | ForEach-Object {$hashes[$_.FullName.Substring($audioOut.Length+1)]=(Get-FileHash -LiteralPath $_.FullName).Hash.ToLowerInvariant()}
$hashes | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $audioOut 'SHA256.json') -Encoding UTF8
$zip=$audioOut+'.zip';if(Test-Path -LiteralPath $zip){throw 'Keep the previous ZIP'}
Add-Type -AssemblyName System.IO.Compression.FileSystem
[IO.Compression.ZipFile]::CreateFromDirectory($audioOut,$zip)
[ordered]@{Bundle=$audioOut;Zip=$zip;Sha256=(Get-FileHash -LiteralPath $zip).Hash.ToLowerInvariant();Bytes=(Get-Item -LiteralPath $zip).Length;RuntimeOnlyInstaller=$true;AutomaticStartupTested=$false;DriverInstalled=$false} | ConvertTo-Json
