param([Parameter(Mandatory=$true)][string]$StateDirectory,[switch]$ValidateOnly,[switch]$Restore)
$ErrorActionPreference='Stop'
$audioAction=if($ValidateOnly){'--preflight'}elseif($Restore){'--restore'}else{'--install'}
if($ValidateOnly -and $Restore){throw 'Choose validation or restoration'}
if(-not $ValidateOnly -and -not [Security.Principal.WindowsPrincipal]::new([Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)){throw 'Use elevated native ARM64 PowerShell'}
$env:PYTHONUTF8='1'
$env:PYTHONDONTWRITEBYTECODE='1'
$env:PSModulePath=Join-Path $env:SystemRoot 'System32\WindowsPowerShell\v1.0\Modules'
$audioState=[IO.Path]::GetFullPath($StateDirectory).TrimEnd('\')
if($audioState -match '["\r\n]'){throw 'Invalid state directory'}
# Explicit pipes prevent PowerShell 5.1 from turning a Python stderr record into
# an exception before its real exit status and saved rollback can be inspected.
$audioPsi=[Diagnostics.ProcessStartInfo]::new()
$audioPsi.FileName=Join-Path $PSScriptRoot 'runtime\python\python.exe'
$audioPsi.Arguments='-B "'+(Join-Path $PSScriptRoot 'graph\Install-AudioGraph.py')+'" '+$audioAction+' --state-directory "'+$audioState+'"'
$audioPsi.UseShellExecute=$false;$audioPsi.CreateNoWindow=$true
$audioPsi.RedirectStandardOutput=$true;$audioPsi.RedirectStandardError=$true
$audioProcess=[Diagnostics.Process]::Start($audioPsi)
$audioStdout=$audioProcess.StandardOutput.ReadToEndAsync()
$audioStderr=$audioProcess.StandardError.ReadToEndAsync()
$audioProcess.WaitForExit()
Write-Output $audioStdout.Result
if($audioStderr.Result){Write-Output $audioStderr.Result}
if($audioProcess.ExitCode -ne 0){throw 'Dolby installation/validation failed. Inspect the saved state and cleanup errors.'}
