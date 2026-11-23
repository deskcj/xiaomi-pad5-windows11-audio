# Xiaomi Pad 5 speakers on Windows 11 ARM64

**English** · [Русский](docs/README.ru.md) · [简体中文](docs/README.zh-CN.md)

Driver research for the four internal Cirrus Logic CS35L41 amplifiers on Xiaomi Pad 5 (nabu).

## Status · 23 November 2026

| | Result |
| --- | --- |
| ✅ | The Windows ARM64 CS35L41 4.17.27.8 build started on all four amplifiers of the test tablet. Left and right playback passed. |
| ✅ | The build uses the original HyperOS speaker protection and music profiles, with the factory calibration for each amplifier on the test tablet. It fixes the OTP calibration read and checks for DSP programming errors. |
| ✅ | The built-in microphone measured **6.6–11.7 dB** higher levels on selected test tones compared with the previous Windows driver. The input signal and Windows volume were held constant. |
| ✅ | A boot trace captured successful opens of the Qualcomm DSP firmware and calibration files. The trace reached its size limit before the playback test. |
| ❌ | The system Dolby test on Qualcomm DSP failed while opening the Windows audio stream after boot. A separate test using the original Dolby library improved the response at 220 Hz by 4.4–4.8 dB on this tablet. |
| ❌ | Matching HyperOS at maximum volume and validating on other Xiaomi Pad 5 units are still open. |

## In progress

- Trace the Dolby registration file and library from Qualcomm DSP startup through the first playback request to isolate the audio stream failure.
- Compare music and maximum output with HyperOS and validate calibration handling for other units.

## Driver and source

The tested 4.17.27.8 package uses calibration from one tablet. A public Windows ARM64 package and its buildable source are pending per-device calibration handling and validation. The source pages below cover the upstream Windows driver and the Xiaomi board files.

[Measurements](docs/baseline.md) · [Build results](docs/reference-build.md) · [Upstream sources](docs/baseline.md#source-revisions)
