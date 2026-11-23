# CS35L41 build results

The Windows ARM64 prototypes derive from [map220v/cs35l41_win](https://github.com/map220v/cs35l41_win/tree/53d367532fa1632accf4e4d20dc0493e2673e077). All target ACPI\10133541. The WDK package includes one speaker-protection firmware image and four position-specific coefficient files. The INF targets Windows 11 build 22000 and later.

| Prototype | Change | Package validation |
| --- | --- | --- |
| DSP reference | Protected playback with upstream calibration | InfVerif and Inf2Cat passed |
| Landscape routing | Right pair BR/TR, left pair BL/TL in the board orientation | InfVerif and Inf2Cat passed |
| Factory calibration | Local per-amplifier calibration from the test tablet | InfVerif and Inf2Cat passed |
| CS35L41 4.17.27.8 | Original HyperOS music profiles, OTP read fix, DSP write error handling and this tablet's factory calibration | InfVerif and Inf2Cat passed. Both stereo channels worked and all four amplifier nodes started. |
| Generated factory calibration | Derive per-amplifier values from the original HyperOS binary and text calibration records for the target unit | EWDK build and catalog validation passed. The bus simulation matched all 12 calibration register writes from the tested 4.17.27.8 build. |
| Qualcomm Dolby 1.0.2140.13 | Submit one new processing graph and rely on Windows to restart QCADCM during installation | Qualcomm DSP registered the graph with both the stock and experimental ADSP configurations. Playback failed with `0x8007045D`. With the experimental extension, the trace showed `ADSP_EFAILED` when opening the stream. Rolling back restored stereo playback. |
| Qualcomm Dolby 1.0.2140.17 | Request Dolby module preloading and trace Qualcomm FastRPC from boot | The file mapping table was submitted. The trace leaves Dolby module loading unconfirmed. PCM16 and the native Windows mix failed with `0x8007045D` and both opened after rollback. |
| Cirrus 4.17.27.10 / Qualcomm 1.0.2060.2 | Test 24-bit speaker playback with a wider physical link | Both packages bound, and all four amplifiers started. WASAPI could not open the shared or exclusive test streams. Rolling back to 4.17.27.8 and the stock Qualcomm extension restored stereo. |

The earlier test-signed DSP startup build and 4.17.27.8 are separate trials. The unit-specific 4.17.27.8 package remains local. [Measurements](baseline.md#cs35l41-417278-and-qualcomm-dolby)

## Driver ZIP 4.17.27.16

On 24 November 2026, 4.17.27.15 stayed active while the test tablet played music at 100% Windows volume with external power. The interval from first to last detected signal was 213.1 seconds, and the Windows meter peaked at −0.12 and −0.11 dBFS. No protection interrupts were logged. The four amplifier snapshots recorded zero over-temperature, over-excursion and abnormal-mute counts. The cause of the earlier 4.17.27.8 shutdown is still unknown.

4.17.27.16 uses the same amplifier gain, DSP path, protection firmware and music profiles as 4.17.27.15. It reads each tablet's original factory calibration at startup. The installer checks the per-speaker binary and text pairs and saves the previous Windows driver for rollback. EWDK and the Windows test-signing checks passed.

On the test tablet, the ZIP installed all four amplifiers. All 12 imported calibration words matched the hardware-tested 4.17.27.15 build. PCM16 / 48 kHz stereo and the Windows native mix opened, and rollback restored 4.17.27.15 in the same Windows boot. Long playback and calibration validation on additional units remain work in progress.

[Installation](../driver/INSTALL.md) · [EWDK source and build instructions](../driver/)
