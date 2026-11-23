#requires -Version 5.1
[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$revision = '53d367532fa1632accf4e4d20dc0493e2673e077'
$source = "https://raw.githubusercontent.com/map220v/cs35l41_win/$revision/firmware"
$firmware = Join-Path $PSScriptRoot 'firmware'
$expected = [ordered]@{
    'BL-cs35l41-dsp1-spk-prot.bin' = 'ED3B09FC202372FF099FC3D27AA8170548248E9850E3EDD3557899ECE4D2DED9'
    'BR-cs35l41-dsp1-spk-prot.bin' = '8AFD4530FAA3716128B2807C6860E255C81E9AEE5690C2D7FB6261185A69DF55'
    'TL-cs35l41-dsp1-spk-prot.bin' = 'BF510D2EBA58FEF26C927229D08289AE39EB515C628CE05CB5DA0E7A5EAA3E2C'
    'TR-cs35l41-dsp1-spk-prot.bin' = '9E7E1EF1CE28059FE7B0A8C880C5C6AF2EBA9FEA4B01365C951A30A20D076859'
    'cs35l41-dsp1-spk-prot.wmfw' = 'D1BEA27E41001024EB3878276292B5BCC0CDA4484FFCED0880E12C03EC9BAC2C'
}
[void][IO.Directory]::CreateDirectory($firmware)
foreach ($entry in $expected.GetEnumerator()) {
    $target = Join-Path $firmware $entry.Key
    if (Test-Path -LiteralPath $target) {
        if ((Get-FileHash -LiteralPath $target).Hash -cne $entry.Value) {
            throw "Existing firmware file differs from the pinned revision: $($entry.Key)."
        }
        continue
    }
    $partial = "$target.download"
    try {
        Invoke-WebRequest -UseBasicParsing -Uri "$source/$($entry.Key)" -OutFile $partial
        if ((Get-FileHash -LiteralPath $partial).Hash -cne $entry.Value) {
            throw "Downloaded firmware failed the hash check: $($entry.Key)."
        }
        Move-Item -LiteralPath $partial -Destination $target
    } finally {
        if (Test-Path -LiteralPath $partial) { Remove-Item -LiteralPath $partial }
    }
}
