param([Parameter(Mandatory=$true)][string]$StateDirectory,[switch]$ValidateOnly,[switch]$Restore,
    [ValidateSet('Manual','Automatic')][string]$StartMode='Manual')
$ErrorActionPreference='Stop'
$audioPackage=$PSScriptRoot
$audioName='NabuDolbyRuntime'
$audioStateRoot=[IO.Path]::GetFullPath($StateDirectory)
$audioStatePath=Join-Path $audioStateRoot 'RuntimeInstall.json'
function Read-AudioManifest([string]$Root) {
    $manifest=Get-Content -LiteralPath (Join-Path $Root 'Manifest.json') -Raw | ConvertFrom-Json
    $base=[IO.Path]::GetFullPath($Root).TrimEnd('\')+'\'
    foreach($file in $manifest.Files.PSObject.Properties){
        $path=[IO.Path]::GetFullPath((Join-Path $base $file.Name))
        if(-not $path.StartsWith($base,[StringComparison]::OrdinalIgnoreCase)){throw 'Manifest path escapes the runtime'}
        if((Get-FileHash -LiteralPath $path).Hash.ToLowerInvariant() -ne $file.Value){throw ('Runtime hash mismatch: '+$file.Name)}
    }
    foreach($name in 'NabuDolbyService.exe','NabuDolbyBroker.exe','NabuDolbyBridge.dll'){
        if((Get-AuthenticodeSignature -LiteralPath (Join-Path $Root $name)).Status -ne 'Valid'){throw ('Runtime signature invalid: '+$name)}
    }
    return $manifest
}
function Save-AudioInstall { $audioState | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $audioStatePath -Encoding UTF8 }
function Prepare-AudioLogs($manifest) {
    if(-not $manifest.PSObject.Properties['LogRoot'] -or -not $manifest.LogRoot){return}
    $root=[IO.Path]::GetFullPath($manifest.LogRoot).TrimEnd('\')
    if($root -notmatch '^[A-Za-z]:\\.+' -or $root -match '[\r\n]' -or -not (Test-Path -LiteralPath (Split-Path -Parent $root) -PathType Container)){throw 'Invalid dedicated log directory'}
    $marker=Join-Path $root 'NabuRuntimeLogs.json'
    $identity=[ordered]@{RuntimeVersion=$manifest.RuntimeVersion;BankName=$manifest.BankName;BridgeSha256=$manifest.BridgeSha256}
    if(Test-Path -LiteralPath $root){
        if(-not (Test-Path -LiteralPath $marker -PathType Leaf)){throw 'Existing log directory is not owned by this runtime; preserve it'}
        $previous=Get-Content -LiteralPath $marker -Raw | ConvertFrom-Json
        foreach($key in $identity.Keys){if($previous.$key -ne $identity[$key]){throw 'Existing logs belong to a different runtime; preserve them'}}
    }else{
        New-Item -ItemType Directory -Path $root | Out-Null
        $identity | ConvertTo-Json | Set-Content -LiteralPath $marker -Encoding UTF8
        # Grant only this new logs directory. Runtime/code stays under Program Files.
        $acl=Get-Acl -LiteralPath $root
        $sid=[Security.Principal.SecurityIdentifier]::new('S-1-5-19')
        $rule=[Security.AccessControl.FileSystemAccessRule]::new($sid,'Modify','ContainerInherit,ObjectInherit','None','Allow')
        $acl.AddAccessRule($rule);Set-Acl -LiteralPath $root -AclObject $acl
    }
    $audioState.LogRoot=$root;Save-AudioInstall
}
function Owned-AudioConfig {
    $service=Get-Service -Name $audioName -ErrorAction SilentlyContinue
    if(-not $service){return $null}
    $config=Get-ItemProperty -LiteralPath ('HKLM:\SYSTEM\CurrentControlSet\Services\'+$audioName)
    $match=[regex]::Match($config.ImagePath,'^"([^"\r\n]+\\NabuDolbyService\.exe)" --service$')
    if(-not $match.Success -or $config.ObjectName -ne 'NT AUTHORITY\LocalService'){throw 'Existing service is not the owned LocalService runtime'}
    $root=Split-Path -Parent $match.Groups[1].Value
    $owned=[IO.Path]::GetFullPath((Join-Path $env:ProgramFiles 'NabuAudio\Dolby')).TrimEnd('\')+'\'
    if(-not ([IO.Path]::GetFullPath($root)+'\').StartsWith($owned,[StringComparison]::OrdinalIgnoreCase)){throw 'Existing service is outside the owned runtime directory'}
    $null=Read-AudioManifest $root
    return [ordered]@{ImagePath=$config.ImagePath;Start=[int]$config.Start;Account=$config.ObjectName;Root=$root;WasRunning=($service.Status -eq 'Running')}
}
if($ValidateOnly){
    $manifest=Read-AudioManifest (Join-Path $audioPackage 'runtime')
    [ordered]@{Validated=$true;ApoVersion=$manifest.ApoVersion;Files=@($manifest.Files.PSObject.Properties).Count;RequestedStartMode=$StartMode;LogRoot=$manifest.LogRoot;DriverInstalled=$false;ServiceChanged=$false;AudioOpened=$false} | ConvertTo-Json
    return
}
$principal=[Security.Principal.WindowsPrincipal]::new([Security.Principal.WindowsIdentity]::GetCurrent())
if(-not $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)){throw 'Use an elevated PowerShell for the LocalService runtime installation'}
Add-Type -TypeDefinition @'
using System;using System.Runtime.InteropServices;
public static class NabuRuntimeScm {
 [DllImport("advapi32.dll",EntryPoint="OpenSCManagerW",ExactSpelling=true,CharSet=CharSet.Unicode,SetLastError=true)]static extern IntPtr Manager(string machine,string database,uint access);
 [DllImport("advapi32.dll",EntryPoint="OpenServiceW",ExactSpelling=true,CharSet=CharSet.Unicode,SetLastError=true)]static extern IntPtr Open(IntPtr manager,string name,uint access);
 [DllImport("advapi32.dll",EntryPoint="CreateServiceW",ExactSpelling=true,CharSet=CharSet.Unicode,SetLastError=true)]static extern IntPtr Create(IntPtr manager,string name,string display,uint access,uint type,uint start,uint error,string path,string group,IntPtr tag,string deps,string account,string password);
 [DllImport("advapi32.dll",EntryPoint="ChangeServiceConfigW",ExactSpelling=true,CharSet=CharSet.Unicode,SetLastError=true)]static extern bool Change(IntPtr service,uint type,uint start,uint error,string path,string group,IntPtr tag,string deps,string account,string password,string display);
 [DllImport("advapi32.dll",SetLastError=true)]static extern bool DeleteService(IntPtr service);
 [DllImport("advapi32.dll")]static extern bool CloseServiceHandle(IntPtr handle);
 public static void Apply(string path,uint start,bool exists,bool remove){
  IntPtr manager=Manager(null,null,3);if(manager==IntPtr.Zero)throw new System.ComponentModel.Win32Exception(Marshal.GetLastWin32Error());
  IntPtr service=IntPtr.Zero;
  try{
   service=exists?Open(manager,"NabuDolbyRuntime",0xF01FF):Create(manager,"NabuDolbyRuntime","Nabu original Dolby runtime (experimental)",0xF01FF,0x10,start,1,path,null,IntPtr.Zero,null,"NT AUTHORITY\\LocalService",null);
   if(service==IntPtr.Zero)throw new System.ComponentModel.Win32Exception(Marshal.GetLastWin32Error());
   bool ok=remove?DeleteService(service):!exists||Change(service,0xFFFFFFFF,start,0xFFFFFFFF,path,null,IntPtr.Zero,null,null,null,null);
   if(!ok)throw new System.ComponentModel.Win32Exception(Marshal.GetLastWin32Error());
  }finally{if(service!=IntPtr.Zero)CloseServiceHandle(service);CloseServiceHandle(manager);}
 }
}
'@
if($Restore){
    $audioState=Get-Content -LiteralPath $audioStatePath -Raw | ConvertFrom-Json
    $current=Owned-AudioConfig
    if(-not $current -or $current.ImagePath -ne $audioState.NewImagePath){throw 'Current service changed since this installation; preserve it'}
    Stop-Service -Name $audioName
    (Get-Service $audioName).WaitForStatus('Stopped',[TimeSpan]::FromSeconds(15))
    if($audioState.Prior){
        $null=Read-AudioManifest $audioState.Prior.Root
        [NabuRuntimeScm]::Apply($audioState.Prior.ImagePath,$audioState.Prior.Start,$true,$false)
        if($audioState.Prior.WasRunning){Start-Service $audioName;(Get-Service $audioName).WaitForStatus('Running',[TimeSpan]::FromSeconds(30))}
    }else{[NabuRuntimeScm]::Apply($audioState.NewImagePath,3,$true,$true)}
    $audioState.Restored=$true;Save-AudioInstall
    [ordered]@{Restored=$true;DriverInstalled=$false;AudioOpened=$false;RebootPerformed=$false;RuntimeFilesPreserved=$true} | ConvertTo-Json
    return
}
if(Test-Path -LiteralPath $audioStateRoot){throw 'Use a new state directory on SSD to preserve previous installation records'}
$manifest=Read-AudioManifest (Join-Path $audioPackage 'runtime')
if($manifest.Architecture -ne 'ARM64' -or $manifest.RuntimeVersion -notmatch '^1\.0\.0-lab[0-9]+$'){throw 'Invalid ARM64 runtime manifest'}
if($env:PROCESSOR_ARCHITECTURE -ne 'ARM64'){throw 'Use native ARM64 PowerShell'}
$prior=Owned-AudioConfig
$audioTarget=Join-Path $env:ProgramFiles ('NabuAudio\Dolby\'+$manifest.RuntimeVersion)
$audioReuse=Test-Path -LiteralPath $audioTarget
if($audioReuse){
    $null=Read-AudioManifest $audioTarget
    if((Get-FileHash -LiteralPath (Join-Path $audioTarget 'Manifest.json')).Hash -ne (Get-FileHash -LiteralPath (Join-Path $audioPackage 'runtime\Manifest.json')).Hash){throw 'Existing runtime differs; preserve it and use a new package version'}
}
New-Item -ItemType Directory -Path $audioStateRoot | Out-Null
$audioState=[ordered]@{Prior=$prior;NewImagePath=('"'+(Join-Path $audioTarget 'NabuDolbyService.exe')+'" --service');Installed=$false;Restored=$false;Error=$null;CleanupError=$null;DriverInstalled=$false;AudioOpened=$false;RebootPerformed=$false;StartMode=$StartMode;RuntimeFilesReused=$audioReuse;LogRoot=$null}
Save-AudioInstall
try{
    if(-not $audioReuse){
        New-Item -ItemType Directory -Path $audioTarget | Out-Null
        foreach($file in $manifest.Files.PSObject.Properties){
            $target=Join-Path $audioTarget $file.Name
            New-Item -ItemType Directory -Path (Split-Path -Parent $target) -Force | Out-Null
            Copy-Item -LiteralPath (Join-Path (Join-Path $audioPackage 'runtime') $file.Name) -Destination $target
        }
        Copy-Item -LiteralPath (Join-Path $audioPackage 'runtime\Manifest.json') -Destination $audioTarget
    }
    $null=Read-AudioManifest $audioTarget
    Prepare-AudioLogs $manifest
    if($prior){Stop-Service $audioName;(Get-Service $audioName).WaitForStatus('Stopped',[TimeSpan]::FromSeconds(15))}
    $audioStart=if($StartMode -eq 'Automatic'){2}else{3}
    [NabuRuntimeScm]::Apply($audioState.NewImagePath,$audioStart,[bool]$prior,$false)
    $audioState.Installed=$true;Save-AudioInstall
    Start-Service $audioName;(Get-Service $audioName).WaitForStatus('Running',[TimeSpan]::FromSeconds(30))
    $audioState.ServiceRunning=$true;Save-AudioInstall
}catch{
    $audioInstallError=$_
    $audioState.Error=$_.Exception.Message;Save-AudioInstall
    try{
        $current=Get-Service $audioName -ErrorAction SilentlyContinue
        if($audioState.Installed){
            $owned=Owned-AudioConfig
            if(-not $owned -or $owned.ImagePath -ne $audioState.NewImagePath){throw 'Service changed during failed install; preserve it'}
            if($current.Status -ne 'Stopped'){Stop-Service $audioName;(Get-Service $audioName).WaitForStatus('Stopped',[TimeSpan]::FromSeconds(15))}
            if($prior){
                [NabuRuntimeScm]::Apply($prior.ImagePath,$prior.Start,$true,$false)
                if($prior.WasRunning){Start-Service $audioName;(Get-Service $audioName).WaitForStatus('Running',[TimeSpan]::FromSeconds(30))}
            }else{[NabuRuntimeScm]::Apply($audioState.NewImagePath,3,$true,$true)}
            $audioState.Restored=$true;Save-AudioInstall
        }elseif($prior -and $prior.WasRunning -and $current.Status -eq 'Stopped'){
            Start-Service $audioName;(Get-Service $audioName).WaitForStatus('Running',[TimeSpan]::FromSeconds(30))
            $audioState.Restored=$true;Save-AudioInstall
        }
    }catch{$audioState.CleanupError=$_.Exception.Message;Save-AudioInstall}
    throw $audioInstallError
}
$audioState | ConvertTo-Json -Depth 5
