#requires -Version 5.1
#requires -RunAsAdministrator
[CmdletBinding()]
param(
    [ValidateSet('Install','Rollback')][string]$Action = 'Install',
    [string]$CalibrationPath,
    [switch]$TrustTestCertificate
)

$ErrorActionPreference = 'Stop'
$version = '4.17.27.20'
$expectedSigner = 'F38EDD5C105095C8342556158CE66253F045965D'
$service = 'HKLM:\SYSTEM\CurrentControlSet\Services\cs35l41'
$settings = Join-Path $service 'Parameters'
$valueName = 'NabuFactoryCalibration'
$package = Join-Path $PSScriptRoot 'package'
$inf = Join-Path $package 'cs35l41.inf'
$backup = Join-Path $PSScriptRoot 'rollback'
$backupPackage = Join-Path $backup 'package'
$statePath = Join-Path $backup 'state.json'
$nativePnp = Join-Path $env:SystemRoot 'System32\pnputil.exe'

function Invoke-Pnp([string[]]$Command, [int[]]$Allowed = @(0)) {
    & $nativePnp @Command | Out-Host
    if ($LASTEXITCODE -notin $Allowed) {
        throw "PnPUtil returned $LASTEXITCODE for $($Command[0])."
    }
}

function Test-WindowsTestMode {
    if (-not ('NabuDriverTestMode' -as [type])) {
        Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;

public static class NabuDriverTestMode
{
    [StructLayout(LayoutKind.Sequential)]
    private struct CodeIntegrityInformation
    {
        public uint Length, Options;
    }

    [DllImport("ntdll.dll")]
    private static extern int NtQuerySystemInformation(int informationClass,
        ref CodeIntegrityInformation buffer, uint length, out uint returned);

    public static bool Enabled()
    {
        var info = new CodeIntegrityInformation { Length = 8 };
        uint returned;
        int status = NtQuerySystemInformation(103, ref info, info.Length, out returned);
        if (status < 0)
            throw new InvalidOperationException("Windows could not report Code Integrity settings.");
        return (info.Options & 2) != 0;
    }
}
'@
    }
    [NabuDriverTestMode]::Enabled()
}

function Get-Amplifiers {
    $devices = @(Get-PnpDevice -PresentOnly |
        Where-Object { $_.InstanceId -match '^ACPI\\(10133541|CSCL3541)\\[0-3]$' })
    if ($devices.Count -ne 4) { throw 'Expected four Xiaomi Pad 5 CS35L41 amplifiers (ACPI\10133541 or ACPI\CSCL3541).' }
    $families = @($devices | ForEach-Object { $_.InstanceId.Split('\')[1] } | Select-Object -Unique)
    $uids = @($devices | ForEach-Object { $_.InstanceId.Split('\')[2] } | Sort-Object -Unique)
    if ($families.Count -ne 1 -or ($uids -join ',') -ne '0,1,2,3') {
        throw 'Expected one ACPI ID family with amplifier UIDs 0,1,2,3.'
    }
    $keys = 'DEVPKEY_Device_DriverInfPath','DEVPKEY_Device_DriverVersion','DEVPKEY_Device_Service'
    foreach ($device in $devices) {
        $values = @{}
        foreach ($item in (Get-PnpDeviceProperty -InstanceId $device.InstanceId -KeyName $keys)) {
            $values[$item.KeyName] = $item.Data
        }
        [pscustomobject]@{
            Id = $device.InstanceId
            Status = $device.Status
            Inf = $values['DEVPKEY_Device_DriverInfPath']
            Version = $values['DEVPKEY_Device_DriverVersion']
            Service = $values['DEVPKEY_Device_Service']
        }
    }
}

function Find-PublishedInf([string]$Hash) {
    $found = @(Get-ChildItem -LiteralPath (Join-Path $env:SystemRoot 'INF') -File -Filter 'oem*.inf' |
        Where-Object { (Get-FileHash -LiteralPath $_.FullName).Hash -eq $Hash })
    if ($found.Count -gt 1) { throw 'More than one published INF matches the same package.' }
    if ($found.Count -eq 1) { $found[0].Name }
}

function Confirm-Active([string]$PublishedInf, [string]$ExpectedVersion) {
    $devices = @(Get-Amplifiers)
    if (-not $PublishedInf -or @($devices | Where-Object {
        $_.Status -ne 'OK' -or $_.Inf -ne $PublishedInf -or $_.Version -ne $ExpectedVersion
    }).Count) { throw "Windows did not start all four amplifiers on $ExpectedVersion." }
    $devices | Format-Table Id,Status,Inf,Version -AutoSize | Out-Host
}

function Read-FactoryCalibration([string]$Folder) {
    if (-not $Folder) { throw 'Pass -CalibrationPath with the four factory calibration pairs from this tablet.' }
    $source = (Resolve-Path -LiteralPath $Folder).ProviderPath
    $blob = [byte[]]::new(52)
    [BitConverter]::GetBytes([uint32]1).CopyTo($blob, 0)
    $offset = 4
    foreach ($corner in 'br','tr','bl','tl') {
        $prefix = Join-Path $source "cs35l41_cal_$corner"
        $binary = [IO.File]::ReadAllBytes("$prefix.bin")
        $line = [IO.File]::ReadAllText("$prefix.txt")
        if ($binary.Length -ne 4 -or $line -cnotmatch '\Astatus = ([0-9]+), Impedance = [0-9]+\.[0-9]+, cal_r = ([0-9]+), ambient = -?[0-9]+, checksum = ([0-9]+)\s*\z') {
            throw "Unexpected factory calibration format for $corner."
        }
        $status = [uint32]$Matches[1]
        $resistance = [uint32]$Matches[2]
        $checksum = [uint32]$Matches[3]
        if ($status -ne 1 -or $resistance -eq 0 -or $resistance -ge 0xffffff -or
            $resistance -ne [BitConverter]::ToUInt32($binary, 0) -or $checksum -ne $resistance + $status) {
            throw "Invalid or inconsistent factory calibration for $corner."
        }
        foreach ($word in $resistance,$status,$checksum) {
            [BitConverter]::GetBytes([uint32]$word).CopyTo($blob, $offset)
            $offset += 4
        }
    }
    ,$blob
}

function Confirm-Package {
    $expected = @('cs35l41.inf','cs35l41.cat','cs35l41.sys','cs35l41-dsp1-spk-prot.wmfw',
        'BL-cs35l41-dsp1-spk-prot.bin','BR-cs35l41-dsp1-spk-prot.bin',
        'TL-cs35l41-dsp1-spk-prot.bin','TR-cs35l41-dsp1-spk-prot.bin')
    $receipt = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'sha256.json') -Raw | ConvertFrom-Json
    if ($receipt.Count -ne $expected.Count -or (Compare-Object $expected @($receipt.Name))) {
        throw 'The driver package has an unexpected hash manifest.'
    }
    foreach ($item in $receipt) {
        if ((Get-FileHash -LiteralPath (Join-Path $package $item.Name)).Hash -cne $item.Hash) {
            throw "Driver file failed the hash check: $($item.Name)."
        }
    }
    $cert = [Security.Cryptography.X509Certificates.X509Certificate2]::new((Join-Path $PSScriptRoot 'Nabu-Test.cer'))
    if ($cert.Thumbprint -cne $expectedSigner) { throw 'Unexpected test signing certificate.' }
    $catalog = Join-Path $package 'cs35l41.cat'
    $signature = Get-AuthenticodeSignature -LiteralPath $catalog
    if ($signature.SignerCertificate.Thumbprint -cne $expectedSigner) { throw 'Unexpected catalog signer.' }
    if ($signature.Status -ne 'Valid') {
        if (-not $TrustTestCertificate) {
            throw 'The test signer is untrusted. Verify the publisher, then rerun with -TrustTestCertificate if you trust this certificate.'
        }
        foreach ($store in 'Root','TrustedPublisher') {
            Import-Certificate -FilePath (Join-Path $PSScriptRoot 'Nabu-Test.cer') -CertStoreLocation "Cert:\LocalMachine\$store" | Out-Null
        }
        if ((Get-AuthenticodeSignature -LiteralPath $catalog).Status -ne 'Valid') {
            throw 'Windows could not verify the driver catalog after importing the test certificate.'
        }
    }
}

function Restore-Amplifiers {
    $state = Get-Content -LiteralPath $statePath -Raw | ConvertFrom-Json
    $previous = @(Get-ChildItem -LiteralPath $backupPackage -Filter '*.inf' -File -Recurse)
    if ($previous.Count -ne 1 -or (Get-FileHash -LiteralPath $previous[0].FullName).Hash -ne $state.PreviousHash) {
        throw 'The exported working package is missing or its INF changed.'
    }
    $old = Find-PublishedInf $state.PreviousHash
    if (-not $old) {
        Invoke-Pnp @('/add-driver',$previous[0].FullName)
        $old = Find-PublishedInf $state.PreviousHash
        if (-not $old) { throw 'Could not locate the exported working driver in the Driver Store.' }
    }
    if ($state.PreviousCalibration) {
        if (-not (Test-Path -LiteralPath $settings)) { [void](New-Item -Path $settings) }
        [void](New-ItemProperty -LiteralPath $settings -Name $valueName -PropertyType Binary -Value ([Convert]::FromBase64String($state.PreviousCalibration)) -Force)
    } elseif ((Test-Path -LiteralPath $settings) -and
        (Get-Item -LiteralPath $settings).GetValueNames() -contains $valueName) {
        Remove-ItemProperty -LiteralPath $settings -Name $valueName
    }
    $candidate = Find-PublishedInf $state.CandidateHash
    if ($candidate -and $candidate -ne $old) {
        Invoke-Pnp @('/delete-driver',$candidate,'/uninstall') @(0,3010)
    }
    $amps = @(Get-Amplifiers)
    if (@($amps | Where-Object { $_.Inf -ne $old -or $_.Status -ne 'OK' }).Count) {
        Invoke-Pnp @('/add-driver',$previous[0].FullName,'/install') @(0,259,3010)
    }
    Confirm-Active $old $state.PreviousVersion
    Write-Host "Restored CS35L41 $($state.PreviousVersion)."
}

if ($Action -eq 'Rollback') { Restore-Amplifiers; return }

if ((Get-CimInstance Win32_OperatingSystem).BuildNumber -as [int] -lt 22000 -or
    (Get-CimInstance Win32_Processor | Select-Object -First 1).Architecture -ne 12) {
    throw 'This package requires Windows 11 ARM64.'
}
$amps = @(Get-Amplifiers)
if (@($amps | Where-Object { $_.Status -ne 'OK' -or $_.Service -ne 'cs35l41' }).Count) {
    throw 'Start from four working CS35L41 devices so the current driver can be restored.'
}
$previousAlias = @($amps.Inf | Select-Object -Unique)
$previousVersion = @($amps.Version | Select-Object -Unique)
if ($previousAlias.Count -ne 1 -or $previousAlias[0] -notmatch '^oem[0-9]+\.inf$' -or $previousVersion.Count -ne 1) {
    throw 'Expected the same OEM package on all four amplifiers.'
}
$calibration = Read-FactoryCalibration $CalibrationPath
if (-not (Test-WindowsTestMode)) {
    throw 'Enable Windows Test Mode before installing the test-signed package.'
}
Confirm-Package
$candidateHash = (Get-FileHash -LiteralPath $inf).Hash
$previousHash = (Get-FileHash -LiteralPath (Join-Path (Join-Path $env:SystemRoot 'INF') $previousAlias[0])).Hash
$oldCalibration = $null
if (Test-Path -LiteralPath $settings) {
    $parameters = Get-Item -LiteralPath $settings
    try { $oldCalibration = $parameters.GetValue($valueName, $null) }
    finally { $parameters.Close() }
}
if ($previousHash -eq $candidateHash) {
    if (($oldCalibration -is [byte[]]) -and
        ([Convert]::ToBase64String($oldCalibration) -ceq [Convert]::ToBase64String($calibration))) {
        Write-Host "CS35L41 $version and this calibration are already installed."
        return
    }
    throw 'Roll back the active package before changing its calibration.'
}
if ($null -ne $oldCalibration -and $oldCalibration -isnot [byte[]]) {
    throw 'The existing calibration registry value has an unexpected type.'
}

if (Test-Path -LiteralPath $statePath) {
    $state = Get-Content -LiteralPath $statePath -Raw | ConvertFrom-Json
    if ($state.PreviousHash -ne $previousHash -or $state.CandidateHash -ne $candidateHash) {
        throw 'The saved rollback is for a different package. Extract the ZIP to a fresh folder.'
    }
} else {
    if (Test-Path -LiteralPath $backup) { throw 'An incomplete rollback folder exists. Extract the ZIP to a fresh folder.' }
    [void](New-Item -ItemType Directory -Path $backupPackage)
    Invoke-Pnp @('/export-driver',$previousAlias[0],$backupPackage)
    $exported = @(Get-ChildItem -LiteralPath $backupPackage -Filter '*.inf' -File -Recurse)
    if ($exported.Count -ne 1 -or (Get-FileHash -LiteralPath $exported[0].FullName).Hash -ne $previousHash) {
        throw 'Could not verify the exported working driver.'
    }
    [ordered]@{
        CandidateHash=$candidateHash; PreviousHash=$previousHash; PreviousVersion=$previousVersion[0]
        PreviousCalibration=if ($oldCalibration) { [Convert]::ToBase64String($oldCalibration) } else { $null }
    } | ConvertTo-Json | Set-Content -LiteralPath $statePath -Encoding UTF8
}

try {
    if (-not (Test-Path -LiteralPath $settings)) { [void](New-Item -Path $settings) }
    [void](New-ItemProperty -LiteralPath $settings -Name $valueName -PropertyType Binary -Value $calibration -Force)
    Invoke-Pnp @('/add-driver',$inf,'/install')
    Start-Sleep -Seconds 4
    Confirm-Active (Find-PublishedInf $candidateHash) $version
    Write-Host "CS35L41 $version installed. Keep the extracted folder for rollback."
} catch {
    $installError = $_.Exception.Message
    try { Restore-Amplifiers } catch { throw "Installation error: $installError. Rollback error: $($_.Exception.Message)" }
    throw "Installation error: $installError. The previous driver was restored."
}
