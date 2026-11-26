param([Parameter(Mandatory=$true)][string]$RuntimeDirectory,[Parameter(Mandatory=$true)][string]$ApoPackageDirectory,
    [Parameter(Mandatory=$true)][string]$OutputDirectory,[Parameter(Mandatory=$true)][ValidatePattern('^1\.0\.0-lab[0-9]+$')][string]$RuntimeVersion,
    [string]$LogDirectory,[switch]$AudioGraphBank,[switch]$WarmSpare)
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
# Graph-bank staging does not install or bind the included APO. It prepares
# matching resources for a later guarded Windows graph trial.
$bank=if($AudioGraphBank){'Global\NabuDolbyBank-9df8834a-209e-4cc1-bc73-bb4353adfa9f'}else{'Global\NabuDolbyBank-ServiceSmoke-'+[Guid]::NewGuid().ToString()}
$audioLogs=$null
if($LogDirectory){
    if(-not [IO.Path]::IsPathRooted($LogDirectory) -or $LogDirectory -match '[\r\n]'){throw 'Use an absolute local log directory'}
    $audioLogs=[IO.Path]::GetFullPath($LogDirectory).TrimEnd('\')
    if($audioLogs -notmatch '^[A-Za-z]:\\.+'){throw 'Use a dedicated local log directory'}
    if(-not (Test-Path -LiteralPath (Split-Path -Parent $audioLogs) -PathType Container)){throw 'Log parent directory must already exist'}
}
$ini="[Runtime]`nSourceFingerprint=$($manifest.SourceFingerprint)`nBridgeSha256=$($manifest.BridgeSha256)`nBankName=$bank`n"
if($audioLogs){$ini+="LogRoot=$audioLogs`n"}
if($WarmSpare){$ini+="WarmSpare=1`n"}
[IO.File]::WriteAllText((Join-Path $audioOut 'runtime\Runtime.ini'),$ini,[Text.UTF8Encoding]::new($false))
$manifest.Files.'Runtime.ini'=(Get-FileHash -LiteralPath (Join-Path $audioOut 'runtime\Runtime.ini')).Hash.ToLowerInvariant()
if(-not $manifest.PSObject.Properties['RuntimeVersion']){$manifest | Add-Member -NotePropertyName RuntimeVersion -NotePropertyValue $RuntimeVersion}else{$manifest.RuntimeVersion=$RuntimeVersion}
$manifest.BankName=$bank
foreach($audioField in @(@('LogRoot',$audioLogs),@('AudioGraphBank',[bool]$AudioGraphBank),@('WarmSpare',[bool]$WarmSpare))){
    if($manifest.PSObject.Properties[$audioField[0]]){$manifest.($audioField[0])=$audioField[1]}else{$manifest | Add-Member -NotePropertyName $audioField[0] -NotePropertyValue $audioField[1]}
}
$manifest | ConvertTo-Json -Depth 9 | Set-Content -LiteralPath (Join-Path $audioOut 'runtime\Manifest.json') -Encoding UTF8
New-Item -ItemType Directory -Path (Join-Path $audioOut 'APO') | Out-Null
foreach($name in 'NabuDolbyApo.inf','NabuDolbyBridge.dll','nabudolbyapo.cat'){Copy-Item -LiteralPath (Join-Path $component $name) -Destination (Join-Path $audioOut 'APO')}
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'Install-Runtime.ps1') -Destination $audioOut
$note=@'
Experimental ARM64 original Dolby stereo DAP runtime and signed APO candidate.
Install-Runtime.ps1 installs only the portable LocalService backend, in Manual
start mode by default. -StartMode Automatic configures SCM automatic startup;
cold-boot qualification is still required. It does not install the APO, bind
Windows effects or establish Atmos object rendering. Those steps remain open.
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
[ordered]@{Bundle=$audioOut;Zip=$zip;Sha256=(Get-FileHash -LiteralPath $zip).Hash.ToLowerInvariant();Bytes=(Get-Item -LiteralPath $zip).Length;RuntimeOnlyInstaller=$true;AudioGraphBank=[bool]$AudioGraphBank;LogRoot=$audioLogs;ColdBootTested=$false;DriverInstalled=$false} | ConvertTo-Json
