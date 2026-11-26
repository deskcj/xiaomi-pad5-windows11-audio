param([Parameter(Mandatory=$true)][string]$Root)
$ErrorActionPreference='Stop'
$audioRoot=[IO.Path]::GetFullPath($Root).TrimEnd('\')+'\'
$audioManifest=Get-Content -LiteralPath (Join-Path $audioRoot 'GraphPackage.json') -Raw | ConvertFrom-Json
foreach($audioFile in $audioManifest.Files.PSObject.Properties){
    $audioPath=[IO.Path]::GetFullPath((Join-Path $audioRoot $audioFile.Name))
    if(-not $audioPath.StartsWith($audioRoot,[StringComparison]::OrdinalIgnoreCase)){throw 'Manifest path escapes package'}
    if((Get-FileHash -LiteralPath $audioPath).Hash.ToLowerInvariant() -ne $audioFile.Value){throw ('Package hash mismatch: '+$audioFile.Name)}
}
$audioSigned=@('runtime\NabuDolbyService.exe','runtime\NabuDolbyBroker.exe','runtime\NabuDolbyBridge.dll','APO\NabuDolbyBridge.dll','APO\nabudolbyapo.cat','extension\nabudolbybound.cat')
if($audioManifest.ApoVersion -eq '2.0.0.22'){$audioSigned+=@('NabuDolbySettings.exe')}
foreach($audioName in $audioSigned){
    $audioSignature=Get-AuthenticodeSignature -LiteralPath (Join-Path $audioRoot $audioName)
    if($audioSignature.Status -ne 'Valid' -or $audioSignature.SignerCertificate.Thumbprint -ne $audioManifest.SignerThumbprint){throw ('Signature identity mismatch: '+$audioName)}
}
[ordered]@{Passed=$true;Signatures=$audioSigned.Count;Files=@($audioManifest.Files.PSObject.Properties).Count;AudioOpened=$false;SystemChanged=$false} | ConvertTo-Json
