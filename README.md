# Xiaomi Pad 5 (nabu) speakers on Windows 11 ARM64

[![English](https://img.shields.io/badge/English-2f81f7?style=for-the-badge&logo=data%3Aimage%2Fsvg%2Bxml%3Bbase64%2CPHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHZpZXdCb3g9IjAgMCA2MCAzNiI%2BPHJlY3Qgd2lkdGg9IjYwIiBoZWlnaHQ9IjM2IiBmaWxsPSIjMDEyMTY5Ii8%2BPHBhdGggZD0iTTAgMCA2MCAzNk02MCAwIDAgMzYiIHN0cm9rZT0iI2ZmZiIgc3Ryb2tlLXdpZHRoPSIxMCIvPjxwYXRoIGQ9Ik0wIDAgNjAgMzZNNjAgMCAwIDM2IiBzdHJva2U9IiNDODEwMkUiIHN0cm9rZS13aWR0aD0iNCIvPjxwYXRoIGQ9Ik0zMCAwdjM2TTAgMThoNjAiIHN0cm9rZT0iI2ZmZiIgc3Ryb2tlLXdpZHRoPSIxMiIvPjxwYXRoIGQ9Ik0zMCAwdjM2TTAgMThoNjAiIHN0cm9rZT0iI0M4MTAyRSIgc3Ryb2tlLXdpZHRoPSI2Ii8%2BPC9zdmc%2B)](README.md) [![Русский](https://img.shields.io/badge/%D0%A0%D1%83%D1%81%D1%81%D0%BA%D0%B8%D0%B9-d0d7de?style=for-the-badge&logo=data%3Aimage%2Fsvg%2Bxml%3Bbase64%2CPHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHZpZXdCb3g9IjAgMCA2MCAzNiI%2BPHBhdGggZmlsbD0iI2ZmZiIgZD0iTTAgMGg2MHYxMkgweiIvPjxwYXRoIGZpbGw9IiMwMDM5QTYiIGQ9Ik0wIDEyaDYwdjEySDB6Ii8%2BPHBhdGggZmlsbD0iI0Q1MkIxRSIgZD0iTTAgMjRoNjB2MTJIMHoiLz48L3N2Zz4%3D)](docs/README.ru.md)

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
| [Download 4.17.27.20 for ARM64](https://github.com/deskcj/xiaomi-pad5-windows11-audio/releases/download/v4.17.27.20/xiaomi-pad5-audio-4.17.27.20-arm64-driver.zip) | [Development C/H source, INF and EWDK project](driver/) |

The ZIP includes a PowerShell installer and rollback. Use the four factory calibration pairs from your own tablet's HyperOS installation. [Installation instructions](driver/INSTALL.md)

Download: **4.17.27.20**. Development source: **4.17.27.20**. The published ZIP does not include system Dolby or MiSound.

[Experimental Dolby .22 / lab13 add-on](https://github.com/deskcj/xiaomi-pad5-windows11-audio/releases/tag/dolby-2.0.0.22-lab13): matching APO/SFX/backend, saved preference, warm recovery, installer and rollback. Qualification continues.

## Project status

| Function | Status |
| --- | --- |
| Four CS35L41 amplifiers | ✅ .20 installed and started on the tablet. |
| Original Dolby renderer | ✅ Exact original-output checks and earlier live SFX trials. |
| Dolby .22 saved preference | ✅ OFF/ON for new streams, live notifications and persistence across service restart; zero failure fallback. |
| Dolby graph installation | ✅ Matching signed APO/SFX/backend, LocalService Automatic and verified rollback. |
| Warm Dolby recovery | ✅ Same live audiodg/stream recovered with one 30 ms fallback; subsequent three minutes had zero fallback. |
| Permanent Dolby / Atmos | ❌ Cold-boot preference, latency/recovery and Atmos objects unfinished. |
| Stereo L/R | ✅ Digital Normal/RAW separation. ❌ Physical four-speaker isolation unconfirmed. |
| MiSound resources/control | ✅ Original factory data, command/source tests and bounded ACDB reading. |
| MiSound in Qualcomm DSP | ❌ Live modules, graph, COPP and processing unfinished. |
| Hardware PCM24 / HyperOS quality | ❌ Hardware bit depth, acoustics, sleep and maximum-volume safety unverified. |

Current tablet: amplifier **4.17.27.20**, APO **2.0.0.22**, bound SFX **.37** and standalone experimental Dolby backend **lab13** installed. Logs use the LocalService profile.

## Update history

- **26 Nov:** ✅ lab13 warm recovery: one 30 ms fallback; ON/OFF/ON, the next stream and three quiet minutes pass (8,634,816 wet frames, zero soak fallback). Telemetry writing moved off the processing thread; original-output checks and backend rollback pass. ❌ 128 ms latency, gapless recovery and cold boot remain open.

[Evidence](docs/results/audio-dolby-warm-recovery-2026-11-26.json)

## 🏖️ Sandbox

- 🔧 In progress: lower Dolby latency, gapless recovery and startup qualification.
- ❌ Unfinished: MiSound DSP, Atmos objects, physical four-speaker L/R and hardware PCM24.

## ❤️ Help bring better sound to Windows on nabu

Support helps fund driver development and testing on real hardware. You can also star the repository or share reproducible audio results from your nabu.

[![Support via DonationAlerts](https://img.shields.io/badge/Support-DonationAlerts-orange?style=for-the-badge)](https://www.donationalerts.com/r/deskcj)

## Contact me

[![Telegram](https://img.shields.io/badge/Contact-Telegram-26A5E4?style=for-the-badge&logo=telegram&logoColor=white)](https://t.me/deskcj) [![Send an email](https://img.shields.io/badge/Contact-Email-D14836?style=for-the-badge&logo=gmail&logoColor=white)](mailto:deskcj@gmail.com)

## Disclaimer

This is an independent community project. Company and product names identify the hardware and technologies involved.

Experimental kernel drivers are provided as is. Back up Windows and keep a recovery method available before installing a test build.
