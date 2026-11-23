# CS35L41 build results

The Windows ARM64 prototypes derive from [map220v/cs35l41_win](https://github.com/map220v/cs35l41_win/tree/53d367532fa1632accf4e4d20dc0493e2673e077). All target ACPI\10133541. The WDK package includes one speaker-protection firmware image and four position-specific coefficient files. The INF targets Windows 11 build 22000 and later.

| Prototype | Change | Package validation |
| --- | --- | --- |
| DSP reference | Protected playback with upstream calibration | InfVerif and Inf2Cat passed |
| Landscape routing | Right pair BR/TR, left pair BL/TL in the board orientation | InfVerif and Inf2Cat passed |
| Factory calibration | Local per-amplifier calibration from the test tablet | InfVerif and Inf2Cat passed |
| CS35L41 4.17.27.8 | Original HyperOS music profiles, OTP read fix, DSP write error handling and this tablet's factory calibration | InfVerif and Inf2Cat passed. Both stereo channels worked and all four amplifier nodes started. |
| Generated factory calibration | Derive per-amplifier values from the original HyperOS binary and text calibration records for the target unit | EWDK build and catalog validation passed. The bus simulation matched all 12 calibration register writes from the tested 4.17.27.8 build. |
| Qualcomm Dolby 1.0.2140.13 | Submit one new processing graph and rely on Windows to restart QCADCM during installation | Qualcomm DSP accepted the graph with the stock ADSP configuration. WASAPI playback returned `0x8007045D`. Rolling back restored stereo playback. |

The earlier test-signed DSP startup build and the later 4.17.27.8 build are separate trials. The results are listed in the measurement report. The 4.17.27.8 package includes unit-specific calibration. The public package and build source remain pending. [Measurements](baseline.md#cs35l41-417278-and-qualcomm-dolby)
