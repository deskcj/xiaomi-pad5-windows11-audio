param([Parameter(Mandatory=$true)][ValidateSet('Plan','Stop','Start')][string]$Action)
$ErrorActionPreference='Stop'
$audioNames=@('Audiosrv','AudioEndpointBuilder')
$audioBefore=@(foreach($audioName in $audioNames){$audioService=Get-Service -Name $audioName; [ordered]@{Name=$audioName;Status=$audioService.Status.ToString()}})
if($Action -eq 'Plan'){
    [ordered]@{Before=$audioBefore;StopOrder=$audioNames;StartOrder=@('AudioEndpointBuilder','Audiosrv');Force=$false;WindowsReboot=$false;Changed=$false} | ConvertTo-Json -Depth 4
    return
}
$audioPrincipal=[Security.Principal.WindowsPrincipal]::new([Security.Principal.WindowsIdentity]::GetCurrent())
if(-not $audioPrincipal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)){throw 'Elevated token required for audio service transition'}
if($Action -eq 'Stop'){
    $audioUnexpected=@((Get-Service AudioEndpointBuilder).DependentServices | Where-Object {$_.Status -eq 'Running' -and $_.Name -notin $audioNames})
    if($audioUnexpected){throw ('Other running dependent services are present; preserve them: '+($audioUnexpected.Name -join ','))}
    foreach($audioName in $audioNames){
        $audioService=Get-Service -Name $audioName
        if($audioService.Status -ne 'Stopped'){Stop-Service -Name $audioName -ErrorAction Stop;(Get-Service $audioName).WaitForStatus('Stopped',[TimeSpan]::FromSeconds(15))}
    }
}else{
    foreach($audioName in @('AudioEndpointBuilder','Audiosrv')){
        $audioService=Get-Service -Name $audioName
        if($audioService.Status -ne 'Running'){Start-Service -Name $audioName -ErrorAction Stop;(Get-Service $audioName).WaitForStatus('Running',[TimeSpan]::FromSeconds(20))}
    }
}
$audioAfter=@(foreach($audioName in $audioNames){$audioService=Get-Service -Name $audioName; [ordered]@{Name=$audioName;Status=$audioService.Status.ToString()}})
$audioExpected=if($Action -eq 'Stop'){'Stopped'}else{'Running'}
if(@($audioAfter|Where-Object Status -ne $audioExpected).Count){throw 'Audio service state did not settle'}
[ordered]@{Action=$Action;Before=$audioBefore;After=$audioAfter;Force=$false;WindowsReboot=$false;Changed=$true} | ConvertTo-Json -Depth 4
