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
| [Download 4.17.27.16 for ARM64](https://github.com/deskcj/xiaomi-pad5-windows11-audio/releases/download/v4.17.27.16/xiaomi-pad5-audio-4.17.27.16-arm64-driver.zip) | [Development C/H source, INF and EWDK project](driver/) |

The ZIP includes a PowerShell installer and rollback. Use the four factory calibration pairs from your own tablet's HyperOS installation. [Installation instructions](driver/INSTALL.md)

Download: **4.17.27.16**. Development source: **4.17.27.18**. The published ZIP does not include system Dolby or MiSound.

## Update history

### 2026-11-25 — original MiSound speaker command plan

- ✅ Added the original 2/4/10/12-command speaker sequence, including active/persistent routes and mode exceptions.
- ✅ ARM64 build passed **864 original-machine-code comparisons + 53 memory-bound cases**; existing factory filter checks still pass.
- ❌ Actual system Dolby/MiSound and physical L/R remain unconfirmed. Installed kernel .18 is unchanged.

[Measured evidence](docs/audio-system-progress.md)

<details>
<summary>Previous updates</summary>

#### 2026-11-25 — Dolby APO 2.0.0.10-lab / settings .36

- ✅ Fixed explicit Dolby configuration fallback: **28 software cases** pass; the .9 control failed 14.
- ✅ Added the Windows 11 settings context; signed .10/.36 packages are prepared. **5 + 7 + 2 + 7** regression cases pass; original output stays exact for 196,608 frames.
- ❌ Real system Dolby/MiSound and physical L/R remain unconfirmed. Installed kernel .18 is unchanged.

#### 2026-11-25 — MiSound resource reader v2

- ✅ All **502 factory parameter records** are read exactly; damaged profile tails are rejected before callbacks.
- ✅ **13 malformed + 30,770 bounds/mutation cases** passed. Original 176.4 → 192 kHz factory-table selection now matches both resources.
- ❌ System Dolby/MiSound and acoustic L/R remain open. Installed kernel .18 and prepared APO .9 are unchanged.

#### 2026-11-25 — 4.17.27.18 / Dolby APO 2.0.0.9-lab

- ✅ APO instances now own separate pages; allocation/release and **5 + 7 + 2 + 7** software cases passed. Original output remains exact for 196,608 frames.
- ✅ Added the ARM64 MiSound factory-table reader: **10 exact tables + 14 rejection cases** passed; installed files match HyperOS.
- ❌ System Dolby/MiSound and acoustic L/R remain unverified. Signed .9 is prepared; the installed driver remains .18.

#### 2026-11-25 — 4.17.27.18 / Dolby APO 2.0.0.8-lab

- ✅ Removed pageable view access from processing/reset and cleaned up failed memory locks: **5 native fault cases** passed.
- ✅ Original Dolby output still matches 196,608 frames; **7 + 2 + 7** logging/reset/worker cases passed. Signed .8 candidate prepared.
- ✅ Four quiet digital L/R controls passed again. ❌ System Dolby, MiSound and acoustic L/R remain unverified.

#### 2026-11-25 — 4.17.27.18 / Dolby APO 2.0.0.7-lab

- ✅ Added Windows 11 logging, per-stream identity and clearing of old counters: **7 + 2 + 7 software cases** passed.
- ✅ Original Dolby output still matches 196,608 frames exactly; a signed .7 candidate is prepared. [APO source](driver/apo/)
- ⏳ MiSound: the kernel RPC daemon is already running; real resource opening and ADM acknowledgement remain unresolved. System effects and acoustic L/R are still unverified.

#### 2026-11-25 — APO 2.0.0.5-lab

Fixed stale-reply handling: 6 + 9 + 7 offline cases passed. Four quiet digital L/R cases and raw ACPI/Android board mapping passed; acoustic isolation remained unverified.

#### 2026-11-25 — 4.17.27.17 / APO 2.0.0.4 — development

- ✅ Fixed ignored I²C errors and added L/R route readback checks. Four amplifiers started on **4.17.27.17**; the zero-input WASAPI stream passed.
- ✅ Added Windows 11 APO initialization: **7 + 7 offline cases** passed; each 196,608-frame stereo render matched the original.

[Measured evidence](docs/audio-system-progress.md)

#### 2026-11-25 — MiSound

MiSound control encoder: 96 original-machine-code comparisons passed; DSP processing remains unfinished.

#### 2026-11-24 — Dolby APO

Original Dolby ↔ COM prototype: exact stereo output, fallback/reset/reopen and 339 math ABI cases passed. [↗](docs/dolby-apo-progress.md)

#### 2026-11-24 — PCM24

PCM24 research set: all 13 timing/reopen tests passed; average 10 ms notification improved from 19.978 to 9.994 ms. [↗](docs/pcm24-progress.md)

</details>

## Before → after

| Area | Before | Proven now |
| --- | --- | --- |
| IRQ/GPIO and routing | IRQ setup ignored I²C errors; only CH_BAL was checked. | ✅ .18 rejects failed IRQ setup; real readbacks passed on all four amplifiers. |
| Digital L/R | Separation unmeasured. | ✅ Four quiet cases passed; opposite 800 Hz tone <0.1%. UID/I²C board mapping matches Android. Acoustic isolation remains unverified. |
| Dolby APO | An unavailable explicit bank could select the common fallback; settings used the flat store. | ✅ .10 honors explicit selection; .36 declares the correct context. 28 software cases pass. ❌ System processing unverified. |
| PCM24 | Format/pacing failures. | ✅ Earlier 13 timing/reopen cases passed. Hardware PCM24 remains unverified. |
| MiSound | Only single controls were encoded; complete speaker dispatch was absent. | ✅ 502 exact factory records and the original command plan; 864 code comparisons pass. ❌ System DSP processing unverified. |

System Dolby, MiSound, acoustic L/R separation and HyperOS-quality sound remain unconfirmed. No new loudness increase is claimed.

## In progress

- ❌ **System Dolby:** validate the driver-bound SFX candidate; public global packs are inapplicable to this OEM endpoint.
- ❌ **MiSound:** resolve original DSP resource loading and parameter acknowledgement.
- ⏳ Physical per-speaker L/R, PCM24 output, HyperOS quality and maximum-volume safety remain unverified.

## ❤️ Help bring better sound to Windows on nabu

Support helps fund driver development and testing on real hardware. You can also star the repository or share reproducible audio results from your nabu.

[![Support via DonationAlerts](https://img.shields.io/badge/Support-DonationAlerts-orange?style=for-the-badge)](https://www.donationalerts.com/r/deskcj)

## Contact me

[![Telegram](https://img.shields.io/badge/Contact-Telegram-26A5E4?style=for-the-badge&logo=telegram&logoColor=white)](https://t.me/deskcj) [![Send an email](https://img.shields.io/badge/Contact-Email-D14836?style=for-the-badge&logo=gmail&logoColor=white)](mailto:deskcj@gmail.com)

## Disclaimer

This is an independent community project. Company and product names identify the hardware and technologies involved.

Experimental kernel drivers are provided as is. Back up Windows and keep a recovery method available before installing a test build.
