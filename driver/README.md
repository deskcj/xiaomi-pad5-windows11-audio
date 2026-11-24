# CS35L41 driver source for Xiaomi Pad 5

This directory contains the C sources, headers, INF and EWDK project for the ARM64 driver. The code builds on map220v's CS35L41 driver. The INF targets Windows 11 on nabu with the four `ACPI\10133541` devices. [Pinned upstream revision](https://github.com/map220v/cs35l41_win/tree/53d367532fa1632accf4e4d20dc0493e2673e077)

[Dolby APO lab source](apo/) is a separate, uninstalled user-mode prototype. [MiSound protocol source](dsp/) implements verified control encoding, not DSP audio processing. Neither is included in the current public driver ZIP.

## Build with EWDK

1. From this directory, run `powershell.exe -ExecutionPolicy Bypass -File .\Get-Firmware.ps1`. It retrieves the five firmware and coefficient files and checks their SHA-256 hashes.
2. Open the EWDK build environment with ARM64 as the target architecture. The release was built using EWDK 10.0.26100.
3. Build the project.

   ```cmd
   msbuild cs35l41.vcxproj /m /p:Configuration=Release /p:Platform=ARM64 /p:WindowsTargetPlatformVersion=10.0.26100.0 /p:SignMode=Off
   ```

4. The package is under `ARM64\Release\cs35l41`. For a local test build, sign the SYS, regenerate the catalog with Inf2Cat, and sign the catalog with your trusted test certificate.

The driver reads the four factory records from the binary registry value `NabuFactoryCalibration` under the `cs35l41` service's `Parameters` key. The value contains 13 little-endian 32-bit words. The first is format version 1, followed by resistance, status and checksum in BR, TR, BL, TL order. The installer generates it from the target tablet's own HyperOS files. [Installation guide](INSTALL.md)
