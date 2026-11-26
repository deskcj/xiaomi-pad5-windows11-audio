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

## Project status

| Function | Status |
| --- | --- |
| Four CS35L41 amplifiers | ✅ .20 installed and started on the current UEFI. |
| Legacy + Remtrik IDs | ✅ Verified DSDT/INF support. ❌ Installation on Remtrik still untested. |
| Original Dolby renderer | ✅ Exact original-output checks and earlier live SFX trials. |
| Dolby .20 control | ✅ Real .20 audiodg and Windows ON/OFF/ON: 47,521 intentional-bypass frames; zero failure fallback. |
| Independent Dolby backend | ✅ LocalService installation, SCM Automatic configuration and rollback. |
| Permanent Dolby / Atmos | ❌ Full graph/cold boot, latency/recovery and Atmos objects unfinished. |
| Stereo L/R | ✅ Digital Normal/RAW separation. ❌ Physical four-speaker isolation unconfirmed. |
| MiSound resources/control | ✅ Original factory data, command/source tests and bounded ACDB reading. |
| MiSound in Qualcomm DSP | ❌ Live modules, graph, COPP and processing unfinished. |
| Hardware PCM24 / HyperOS quality | ❌ Hardware bit depth, acoustics, sleep and maximum-volume safety unverified. |

Current tablet: amplifiers **4.17.27.20**, APO **2.0.0.4**. The Manual .18 backend is stopped after reboot. A signed .20 candidate and rollback are on SSD; this does not establish permanent system effects.

## Update history

- **26 Nov:** ✅ .20 installed/tested in audiodg; Windows ON/OFF/ON works. ❌ Three-minute playback had 50,112 additional fallback frames (1.044 s).
- **26 Nov:** ✅ Windows restart completed; Qualcomm restart flag cleared. New .20 device trial is next.
- **25 Nov:** ✅ Amplifier .20 on current UEFI; ✅ quiet Dolby .17 recovery trial. ❌ Seamless recovery remains unfinished.

[Detailed history](docs/history/readme-2026-11-26.md) · [26.11 · JSON](docs/results/audio-effects-manager-live-2026-11-26.json)

## ❤️ Help bring better sound to Windows on nabu

Support helps fund driver development and testing on real hardware. You can also star the repository or share reproducible audio results from your nabu.

[![Support via DonationAlerts](https://img.shields.io/badge/Support-DonationAlerts-orange?style=for-the-badge)](https://www.donationalerts.com/r/deskcj)

## Contact me

[![Telegram](https://img.shields.io/badge/Contact-Telegram-26A5E4?style=for-the-badge&logo=telegram&logoColor=white)](https://t.me/deskcj) [![Send an email](https://img.shields.io/badge/Contact-Email-D14836?style=for-the-badge&logo=gmail&logoColor=white)](mailto:deskcj@gmail.com)

## Disclaimer

This is an independent community project. Company and product names identify the hardware and technologies involved.

Experimental kernel drivers are provided as is. Back up Windows and keep a recovery method available before installing a test build.
