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
| [Download 4.17.27.20 for ARM64](https://github.com/deskcj/xiaomi-pad5-windows11-audio/releases/download/v4.17.27.20/xiaomi-pad5-audio-4.17.27.20-arm64-driver.zip) | [Development C/H source, INF and EWDK project](driver/) |

The ZIP includes a PowerShell installer and rollback. Use the four factory calibration pairs from your own tablet's HyperOS installation. [Installation instructions](driver/INSTALL.md)

Download: **4.17.27.20**. Development source: **4.17.27.20**. The published ZIP does not include system Dolby or MiSound.

## Update history

| Date | Change | Verification |
| --- | --- | --- |
| 26 Nov | APO .20 controls, original-state warmup, Automatic backend and raw ACDB resource reader. | Software + real SCM installation/rollback; new APO not tested in audiodg. |
| 25 Nov | Amplifier .20: legacy UEFI + Remtrik IDs. | Four devices on current UEFI; Remtrik installation pending. |
| 25 Nov | Dolby .17 active-stream recovery. | Quiet 3-minute trial; one 1.808 s fallback interval. |

[Detailed history](docs/history/readme-2026-11-26.md) · [26.11 · JSON](docs/results/audio-dolby-runtime-acdb-2026-11-26.json)

## Current status

| Area | Proven | Remaining |
| --- | --- | --- |
| Four amplifiers / UEFI | .20 installed; legacy + Remtrik ID support. | Installation on Remtrik. |
| Dolby | Earlier live SFX; .20 original-output/control tests; independent backend and SCM Automatic. | Permanent graph/cold boot, EffectsManager, latency/recovery and Atmos objects. |
| Stereo | Digital Normal/RAW L/R. | Acoustic isolation of every speaker. |
| MiSound | Original factory/control source; validated ACDB resource views. | Live DSP modules, graph, COPP and processing. |
| PCM24 / quality | Earlier software checks. | Hardware PCM24, HyperOS acoustics, sleep and maximum-volume safety. |

Current tablet: amplifier **4.17.27.20**, APO **2.0.0.4**; the Manual .18 backend was restored. Qualcomm still requires a Windows reboot before a new APO trial. APO .20 is a signed candidate, not hardware-qualified.

## ❤️ Help bring better sound to Windows on nabu

Support helps fund driver development and testing on real hardware. You can also star the repository or share reproducible audio results from your nabu.

[![Support via DonationAlerts](https://img.shields.io/badge/Support-DonationAlerts-orange?style=for-the-badge)](https://www.donationalerts.com/r/deskcj)

## Contact me

[![Telegram](https://img.shields.io/badge/Contact-Telegram-26A5E4?style=for-the-badge&logo=telegram&logoColor=white)](https://t.me/deskcj) [![Send an email](https://img.shields.io/badge/Contact-Email-D14836?style=for-the-badge&logo=gmail&logoColor=white)](mailto:deskcj@gmail.com)

## Disclaimer

This is an independent community project. Company and product names identify the hardware and technologies involved.

Experimental kernel drivers are provided as is. Back up Windows and keep a recovery method available before installing a test build.
