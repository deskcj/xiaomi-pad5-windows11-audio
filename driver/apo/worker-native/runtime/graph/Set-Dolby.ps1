param([ValidateSet('Query','On','Off','Default')][string]$State='Query')
$ErrorActionPreference='Stop'
$audioArg=@{Query='--query';On='--on';Off='--off';Default='--clear'}[$State]
$audioPsi=[Diagnostics.ProcessStartInfo]::new()
$audioPsi.FileName=Join-Path $PSScriptRoot 'NabuDolbySettings.exe';$audioPsi.Arguments=$audioArg
$audioPsi.UseShellExecute=$false;$audioPsi.CreateNoWindow=$true
$audioPsi.RedirectStandardOutput=$true;$audioPsi.RedirectStandardError=$true
$audioProcess=[Diagnostics.Process]::Start($audioPsi)
$audioOutput=$audioProcess.StandardOutput.ReadToEndAsync();$audioError=$audioProcess.StandardError.ReadToEndAsync()
$audioProcess.WaitForExit();Write-Output $audioOutput.Result
if($audioError.Result){Write-Output $audioError.Result}
if($audioProcess.ExitCode -ne 0){throw 'Dolby preference operation failed; inspect the returned API status'}
