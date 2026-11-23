# CS35L41 build results

The Windows ARM64 prototypes derive from [map220v/cs35l41_win](https://github.com/map220v/cs35l41_win/tree/53d367532fa1632accf4e4d20dc0493e2673e077). All target ACPI\10133541. The WDK package includes one speaker-protection firmware image and four position-specific coefficient files. The INF targets Windows 11 build 22000 and later.

| Prototype | Change | Package validation |
| --- | --- | --- |
| DSP reference | Protected playback with upstream calibration | InfVerif and Inf2Cat passed |
| Landscape routing | Right pair BR/TR, left pair BL/TL in the board orientation | InfVerif and Inf2Cat passed |
| Factory calibration | Local per-amplifier calibration from the test tablet | InfVerif and Inf2Cat passed |
| CS35L41 4.17.27.8 | Original HyperOS music profiles, OTP read fix, DSP write error handling and this tablet's factory calibration | InfVerif and Inf2Cat passed. Both stereo channels worked and all four amplifier nodes started. |
| Qualcomm Dolby | Original Dolby calibration and module registration on the Windows Qualcomm DSP route | Driver packages bound after reboot. A Windows shared audio stream failed to open. |

The earlier test-signed DSP startup build and the later 4.17.27.8 build are separate trials. The results are listed in the measurement report. The 4.17.27.8 package includes unit-specific calibration. The public package and build source remain pending. [Measurements](baseline.md#cs35l41-417278-and-qualcomm-dolby)
