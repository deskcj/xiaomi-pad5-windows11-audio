# Reference CS35L41 driver build

The reference driver is the upstream map220v project at commit 53d367532fa1632accf4e4d20dc0493e2673e077. It targets ACPI\10133541 and runs the CS35L41 speaker-protection firmware. This build is for investigating the Windows audio path.

## Reproduce the build

1. Check out the pinned [map220v source](https://github.com/map220v/cs35l41_win/tree/53d367532fa1632accf4e4d20dc0493e2673e077) into a separate folder. The firmware inputs are in its firmware directory.
2. In cs35l41.vcxproj, change each of the five firmware items from CopyFileToFolders to FilesToPackage, preserving the Include path and removing the FileType child. This puts the files beside the generated INF. Without the change Inf2Cat reports missing firmware files.
3. In cs35l41.inf, add the Windows 11 minimum build decoration .10.0...22000 to the NT$ARCH$ model in both Manufacturer and Standard. This limits the DIRID 13 package to its supported Windows target.
4. Open the EWDK 10.0.26100 command environment for x86_arm64, then run:

       msbuild cs35l41.vcxproj /p:Configuration=Release /p:Platform=ARM64 /p:WindowsTargetPlatformVersion=10.0.26100.0 /p:SignMode=Off /p:RunCodeAnalysis=false

5. Check the ARM64\Release\cs35l41 output for the INF, SYS, unsigned CAT, speaker-protection WMFW and four per-position BIN files.

The reference build passed InfVerif /u and generated the catalog with no Inf2Cat errors or warnings using EWDK 26100.6584. The build has not been installed on the tablet. The upstream source and firmware are linked for reference. This repository contains the test tool and our observations.
