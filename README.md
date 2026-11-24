# Xiaomi Pad 5 (nabu) speakers on Windows 11 ARM64

[![English](https://img.shields.io/badge/English-2f81f7?style=for-the-badge&logo=data%3Aimage%2Fsvg%2Bxml%3Bbase64%2CPHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHZpZXdCb3g9IjAgMCA2MCAzNiI%2BPHJlY3Qgd2lkdGg9IjYwIiBoZWlnaHQ9IjM2IiBmaWxsPSIjMDEyMTY5Ii8%2BPHBhdGggZD0iTTAgMCA2MCAzNk02MCAwIDAgMzYiIHN0cm9rZT0iI2ZmZiIgc3Ryb2tlLXdpZHRoPSIxMCIvPjxwYXRoIGQ9Ik0wIDAgNjAgMzZNNjAgMCAwIDM2IiBzdHJva2U9IiNDODEwMkUiIHN0cm9rZS13aWR0aD0iNCIvPjxwYXRoIGQ9Ik0zMCAwdjM2TTAgMThoNjAiIHN0cm9rZT0iI2ZmZiIgc3Ryb2tlLXdpZHRoPSIxMiIvPjxwYXRoIGQ9Ik0zMCAwdjM2TTAgMThoNjAiIHN0cm9rZT0iI0M4MTAyRSIgc3Ryb2tlLXdpZHRoPSI2Ii8%2BPC9zdmc%2B)](README.md) [![Русский](https://img.shields.io/badge/%D0%A0%D1%83%D1%81%D1%81%D0%BA%D0%B8%D0%B9-d0d7de?style=for-the-badge&logo=data%3Aimage%2Fsvg%2Bxml%3Bbase64%2CPHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHZpZXdCb3g9IjAgMCA2MCAzNiI%2BPHBhdGggZmlsbD0iI2ZmZiIgZD0iTTAgMGg2MHYxMkgweiIvPjxwYXRoIGZpbGw9IiMwMDM5QTYiIGQ9Ik0wIDEyaDYwdjEySDB6Ii8%2BPHBhdGggZmlsbD0iI0Q1MkIxRSIgZD0iTTAgMjRoNjB2MTJIMHoiLz48L3N2Zz4%3D)](docs/README.ru.md) [![简体中文](https://img.shields.io/badge/%E7%AE%80%E4%BD%93%E4%B8%AD%E6%96%87-d0d7de?style=for-the-badge&logo=data%3Aimage%2Fsvg%2Bxml%3Bbase64%2CPHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHZpZXdCb3g9IjAgMCA2MCAzNiI%2BPHJlY3Qgd2lkdGg9IjYwIiBoZWlnaHQ9IjM2IiBmaWxsPSIjREUyOTEwIi8%2BPHBvbHlnb24gcG9pbnRzPSIxMi4wLDMuNyAxMy41LDcuOSAxOC4wLDguMSAxNC41LDEwLjggMTUuNywxNS4xIDEyLjAsMTIuNiA4LjMsMTUuMSA5LjUsMTAuOCA2LjAsOC4xIDEwLjUsNy45IiBmaWxsPSIjRkZERTAwIi8%2BPHBvbHlnb24gcG9pbnRzPSIyNC4wLDEuNyAyNC42LDMuMiAyNi4yLDMuMyAyNS4wLDQuMyAyNS40LDUuOSAyNC4wLDUuMCAyMi42LDUuOSAyMy4wLDQuMyAyMS44LDMuMyAyMy40LDMuMiIgZmlsbD0iI0ZGREUwMCIvPjxwb2x5Z29uIHBvaW50cz0iMjkuMCw2LjcgMjkuNiw4LjIgMzEuMiw4LjMgMzAuMCw5LjMgMzAuNCwxMC45IDI5LjAsMTAuMCAyNy42LDEwLjkgMjguMCw5LjMgMjYuOCw4LjMgMjguNCw4LjIiIGZpbGw9IiNGRkRFMDAiLz48cG9seWdvbiBwb2ludHM9IjI5LjAsMTMuNyAyOS42LDE1LjIgMzEuMiwxNS4zIDMwLjAsMTYuMyAzMC40LDE3LjkgMjkuMCwxNy4wIDI3LjYsMTcuOSAyOC4wLDE2LjMgMjYuOCwxNS4zIDI4LjQsMTUuMiIgZmlsbD0iI0ZGREUwMCIvPjxwb2x5Z29uIHBvaW50cz0iMjQuMCwxOC43IDI0LjYsMjAuMiAyNi4yLDIwLjMgMjUuMCwyMS4zIDI1LjQsMjIuOSAyNC4wLDIyLjAgMjIuNiwyMi45IDIzLjAsMjEuMyAyMS44LDIwLjMgMjMuNCwyMC4yIiBmaWxsPSIjRkZERTAwIi8%2BPC9zdmc%2B)](docs/README.zh-CN.md)

![Status – in development](https://img.shields.io/badge/status-in%20development-orange?style=for-the-badge) ![Device – Xiaomi Pad 5](https://img.shields.io/badge/device-Xiaomi%20Pad%205-blue?style=for-the-badge) ![Platform – Windows 11 ARM64](https://img.shields.io/badge/platform-Windows%2011%20ARM64-0078D4?style=for-the-badge)

> [!WARNING]
> Experimental audio builds are intended only for Xiaomi Pad 5 (`nabu`) running Windows 11 ARM64.

Driver research for the four internal Cirrus Logic CS35L41 amplifiers on Xiaomi Pad 5.

## Requirements

- An **unlocked bootloader** is required to set up Windows on the tablet. [Windows installation guide for nabu](https://github.com/erdilS/Port-Windows-11-Xiaomi-Pad-5/blob/main/guide/English/won-deployer-install-en.md)
- **Windows Test Mode** (`TESTSIGNING`) is required for the experimental test-signed audio driver. Windows needs a restart after enabling it. [Microsoft Test Mode documentation](https://learn.microsoft.com/en-us/windows-hardware/drivers/install/the-testsigning-boot-configuration-option)

## Download

| Driver ZIP | Driver source |
| --- | --- |
| [Download 4.17.27.16 for ARM64](https://github.com/deskcj/xiaomi-pad5-windows11-audio/releases/download/v4.17.27.16/xiaomi-pad5-audio-4.17.27.16-arm64-driver.zip) | [C source, headers, INF and EWDK project](driver/) |

The ZIP includes a PowerShell installer and rollback. Use the four factory calibration pairs from your own tablet's HyperOS installation. [Installation instructions](driver/INSTALL.md)

## Update history

### 25 November 2026 — original MiSound control encoding verified

- ✅ Added a native ARM64 command encoder from the original Android contract: 96 offline comparisons against executed original machine code passed. This verifies control data, not speaker processing. [Before/after and remaining work](docs/audio-system-progress.md) · [C source and header](driver/dsp/)

### 24 November 2026 — system integration: verified progress and remaining blockers

- ✅ The original Dolby prototype passed official Windows APO initialization and worker contention tests; all 196,608 stereo frames matched the reference again. Signed APO installation and clean rollback were checked without reboot.
- ❌ System-wide Dolby and MiSound remain unfinished: Windows has not activated the APO, and the original MiSound HAL callback performs no sample processing. [Before/after comparison, evidence and remaining tasks](docs/audio-system-progress.md)

### 24 November 2026 — original Dolby connected to a COM APO prototype

- ✅ The unregistered ARM64 APO now receives original Android Dolby output through a separate worker. All 196,608 stereo frames matched the reference in both regular and irregular callback tests. Seven digital scenarios, COM reopen and 339 native math ABI cases passed.
- Worker interruption preserves the delayed original signal; processing recovers after a pause. System-wide Dolby and MiSound remain unfinished. These checks opened no audio device and installed no driver. [Evidence and remaining work](docs/dolby-apo-progress.md)

### 24 November 2026 — verified PCM24 transport

- ✅ Fixed format negotiation and PCM24 word packing. The experimental component set passed all 13 native ARM64 WASAPI timing checks, including 20-second shared/exclusive streams and ten reopen cycles. The 10 ms buffer notification interval improved from 19.978 ms to 9.994 ms.
- The one-second −66 dBFS PCM24 stream was accepted by Windows. Android sound quality, additional loudness and maximum-volume safety remain unconfirmed. [Results and remaining work](docs/pcm24-progress.md) · [Measurement data](docs/results/pcm24-2026-11-24.json)

These results apply to the PCM24 research configuration. The download above remains the existing Cirrus 4.17.27.16 package.

## Status · 25 November 2026

| Area | At the start | Current result |
| --- | --- | --- |
| PCM24 transport | Stream opening failed; later trials supplied about 50% and 75% of the required frames. | ✅ 48 kHz stereo PCM24 in 32-bit containers passed all 13 timing checks. Exclusive playback supplied 961,440 frames over 20.006312 seconds of audio clock time. [Evidence](docs/pcm24-progress.md) |
| Amplifiers | map220v 16.45.25.900 routed the serial input directly to the DAC. | ✅ The Cirrus build routes stereo through the amplifier DSP with the original HyperOS speaker protection and music profiles. All four amplifiers start. |
| Loud playback on the PCM16 baseline | 4.17.27.8 was louder on the selected test tones and the tablet powered off once during loud music. | ✅ Earlier, 4.17.27.15 played music with external power at 100% Windows volume for a measured 3 min 33 s without shutting down or increasing the recorded protection counters. This result does not validate the new PCM24 configuration at maximum volume. |
| Factory calibration | The local build compiled the test tablet's calibration into its binary. | ✅ The public 4.17.27.16 installer reads the target tablet's HyperOS calibration. The ZIP install, PCM16 and native stereo stream opening, and rollback were tested in one Windows session. |
| Standalone Dolby processing | HyperOS Dolby playback on Windows had yet to be demonstrated. | ✅ The original library played 59.84 seconds through the speakers in a standalone process with zero buffer underruns. A separate Float32 test matched the reference output byte for byte. System-wide Dolby remains unfinished. |
| Dolby ↔ COM APO prototype | The default COM instance was passthrough; worker queues were connected only by the research factory. | ✅ The default factory now connects original Dolby through shared memory in an unregistered digital harness. Exact reference output, fallback, reset and reopen passed. [Evidence](docs/dolby-apo-progress.md). This is not system-wide integration. |
| Windows APO initialization | Only the private lab init payload was supported. | ✅ Official system-init and shared worker-broker tests passed; signed component installation and rollback passed. ❌ Actual processing in the Windows graph remains missing. [Before/after](docs/audio-system-progress.md) |

[Earlier level measurements](docs/baseline.md#pcm16-level-response) · [Release build and test details](docs/reference-build.md#driver-zip-4172716)

## In progress

- ❌ **System-wide Dolby.** Exact original-library processing and official Windows initialization passed standalone. The installed component still produced zero audio-host APO callbacks; actual system processing is not active. [Before/after and current blocker](docs/audio-system-progress.md)
- ❌ **MiSound.** The original Android HAL callback was proven to validate buffers without processing samples. Its speaker effect needs the DSP path, where parameter application still times out. [Details](docs/audio-system-progress.md)
- ✅ **PCM24 transport.** Native WASAPI shared/exclusive timing and repeated stream opening passed on the experimental component set. [PCM24 results](docs/pcm24-progress.md)
- ⏳ **PCM24 speaker validation.** Confirm amplifier telemetry and acoustic output. The latest traces contain no amplifier events, so they cannot establish that protection faults were absent.
- ⏳ **Quality and power validation.** Compare music quality and maximum loudness with HyperOS, investigate the earlier loud-music shutdown, and validate factory calibration on other Xiaomi Pad 5 units.

[Measurements](docs/baseline.md) · [Build results](docs/reference-build.md) · [Upstream sources](docs/baseline.md#source-revisions)

## ❤️ Help bring better sound to Windows on nabu

Support helps fund driver development and testing on real hardware. You can also star the repository or share reproducible audio results from your nabu.

[![Support via DonationAlerts](https://img.shields.io/badge/Support-DonationAlerts-orange?style=for-the-badge)](https://www.donationalerts.com/r/deskcj)

## Contact me

[![Telegram](https://img.shields.io/badge/Contact-Telegram-26A5E4?style=for-the-badge&logo=telegram&logoColor=white)](https://t.me/deskcj) [![Send an email](https://img.shields.io/badge/Contact-Email-D14836?style=for-the-badge&logo=gmail&logoColor=white)](mailto:deskcj@gmail.com)

## Disclaimer

This is an independent community project. Company and product names identify the hardware and technologies involved.

Experimental kernel drivers are provided as is. Back up Windows and keep a recovery method available before installing a test build.
