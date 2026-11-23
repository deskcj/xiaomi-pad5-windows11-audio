# Nabu speaker baseline — 2026-11-23

## Windows measurements

| Layer | Observed |
| --- | --- |
| Built-in output | Qualcomm Aqstic, stereo, 48 kHz |
| Audio adapter | Xiaomi 1.0.2100.0, QCAUD |
| Audio DSP and calibration manager | Qualcomm 1.0.2140.0, qcadcm |
| Amplifiers | Four CS35L41 devices, status OK, hardware ID ACPI\10133541 |
| Initial amplifier driver | map220v 16.45.25.900 |

The quiet 440 Hz source peaked at −44.4 dBFS. The endpoint meter returned these normalized linear values:

| Tone | Left endpoint peak | Right endpoint peak |
| --- | ---: | ---: |
| Left | 0.00604 | 0.000031 |
| Right | 0.000031 | 0.00604 |

This establishes a symmetric digital baseline. An acoustic measurement at each grille is needed to identify any downstream loss or silent driver.

## Driver and firmware paths

- The installed Windows package matches ACPI\10133541. Its INF installs only the SYS. The installed binary's import table does not include the native file-open/read functions used by the reference source to load DSP files. Its exact equivalence to an upstream revision has not been established.
- The newer BSP 2611.21 package for ACPI\CSCL3541 maps each amplifier to a separate protection coefficient file. That model ID differs from the one on this tablet. The quiet-speaker report in issue #5 concerns the ACPI\CSCL3541 package, so its calibration and APO findings need to be checked separately.
- A newer map220v source revision targets ACPI\10133541, loads the speaker-protection firmware and chooses a protection blob by ACPI UID. It also contains built-in per-position tuning. Adding a calibration file to the INF alone does not run a calibration procedure.
- The Android tree names four amps by position. The main stereo speaker path uses 48 kHz, two channels and DSP as PCM source. Individual corner routes specify ASP. Android's vendor manifest lists both protection and calibration files for all four positions.

| Position from Android DTS | I²C address | Windows ACPI UID |
| --- | --- | --- |
| Bottom right (BR) | 0x40 | 0 |
| Top right (TR) | 0x41 | 1 |
| Bottom left (BL) | 0x42 | 2 |
| Top left (TL) | 0x43 | 3 |

The live Windows DSDT confirms that UIDs 0–3 use I²C addresses 0x40–0x43. Ghidra and the ARM64 disassembly of the installed driver confirmed that the ACPI UID selects two serial audio slot groups, 0/2 and 1/3. The pinned upstream source uses the same groups. These correspond to bottom/top under the Android board position names.

A separate trial build groups UIDs 0/1 and 2/3 to test stereo in the orientation where BR/TR are physically on the right. It keeps the reference build's protection profiles and gain settings. The pairing is a lead on stereo placement. The cause of the low acoustic output is still open.


## Loudness investigation

Disassembly of the installed map220v 16.45.25.900 SYS and the compiled reference SYS showed distinct amplifier paths:

| Build | CS35L41 DAC input | Analog PCM gain code |
| --- | --- | ---: |
| Installed map220v | ASP RX1, directly from the serial audio input | 10 |
| Reference map220v with protection firmware | DSP TX1, through the amplifier DSP | 18 |
| Nabu Android mixer | DSP on the main speaker path | 18 |

The Windows writes to AMP_GAIN_CTRL are 0x153 and 0x253, with the analog field at bits 9:5. The Linux CS35L41 driver declares 1 dB per step in this field. The eight-step difference is nominal speaker-amplifier gain. It does not predict the change in measured acoustic output, where per-corner tuning, limiting, the signal level and the Qualcomm DSP also matter. [Linux CS35L41 control definitions](https://github.com/torvalds/linux/blob/master/sound/soc/codecs/cs35l41.c)

The five protection files in the reference package match the staged BSP 2611.21 package byte for byte. All four music profiles in that BSP match the arrays compiled into the reference source. The LineageOS manifest also lists a MiSound four-speaker file for the Qualcomm ADSP. Its presence does not establish that every Android configuration enables it. [Android firmware manifest](https://github.com/LineageOS/android_device_xiaomi_nabu/blob/15f60556d4963748ce809f389cb48ba31c2beb78/proprietary-files.txt) · [Xiaomi Nabu board source](https://github.com/MiCode/Xiaomi_Kernel_OpenSource/blob/71b6effd2dfa8f703dd1ea17acd1323b2fd3cf6c/arch/arm64/boot/dts/qcom/nabu-sm8150.dtsi#L352)



## Stock HyperOS and factory calibration

The selected vendor audio files were extracted from the full Global HyperOS OS1.0.6.0.TKXMIXM payload. The speaker mixer XML matches the LineageOS copy byte for byte. All five CS35L41 protection files match the reference Windows package by SHA-256. The decompiled stock HAL reads a four-byte value per corner from vendor persist, validates its range and applies it to the corresponding CS35L41 DSP mixer control. [Xiaomi recovery archive](https://bigota.d.miui.com/OS1.0.6.0.TKXMIXM/miui_NABUGlobal_OS1.0.6.0.TKXMIXM_4af37553d5_13.0.zip)

A local factory-calibrated Windows trial uses the four records from this tablet. Each binary value agrees with the text record and the recorded checksum. The package uses the reference board routing and protection profiles. InfVerif and Inf2Cat passed. Later runtime results are recorded below. The package and unit-specific values are kept locally.

At the same quiet output level, two built-in microphone runs gave approximately −52 dBFS for the left test tone and −56 to −57 dBFS for the right test tone. Those numbers measure the 440 Hz component at the front microphone in its fixed position. They provide a repeatable Windows A/B reference. Speaker grille output and equivalence to Android at high levels require separate acoustic tests.


## Windows driver trial

The test-signed DSP startup build completed initialization on all four CS35L41 devices without a reboot on the second attempt. During low-level A/B measurements, the protected build produced less 440–1760 Hz output at the built-in front microphone. Both sides of the digital Windows endpoint remained symmetric.

| Tone | Source dBFS | Legacy, L/R mic dBFS | Protected trial, L/R mic dBFS |
| --- | ---: | --- | --- |
| 440 Hz | −44.4 | −51.5 / −56.5 | −58.8 / −62.2 |
| 440 Hz | −36 | −43.0 / −48.1 | −57.4 / −54.0 |
| 880 Hz | −36 | −36.9 / −45.7 | −50.1 / −51.4 |
| 1760 Hz | −36 | −39.9 / −41.8 | −53.8 / −46.5 |

The 220 Hz signal from the protected build was close to the room's 220 Hz background in this setup. The legacy package was restored on all four devices. Maximum acoustic output and the stock Android reference remain unmeasured.

## CS35L41 4.17.27.8 and Qualcomm Dolby

The later 4.17.27.8 build corrects the length of the OTP calibration read and checks errors when loading and programming the DSP. It uses the original HyperOS protection firmware, music profiles and the four factory calibration records from the test tablet. All four amplifier devices started in two runs.

The table compares the legacy driver and the two 4.17.27.8 runs on the same tablet, at matching input levels and Windows volume. Each pair gives the left and right test tones measured separately at the same built-in front microphone.

| Tone (Hz) | Input (dBFS) | Legacy 16.45.25.900 (dBFS) | 4.17.27.8 first run (dBFS) | 4.17.27.8 repeat (dBFS) | Repeat gain (dB) |
| ---: | ---: | ---: | ---: | ---: | ---: |
| 220 | −36.0 | −56.5 / −61.0 | −48.0 / −52.8 | −48.0 / −52.9 | +8.5 / +8.1 |
| 440 | −44.4 | −51.7 / −56.8 | −43.1 / −48.9 | −43.1 / −48.9 | +8.6 / +7.9 |
| 440 | −36.0 | −43.2 / −48.4 | −34.7 / −40.6 | −34.8 / −40.6 | +8.4 / +7.8 |
| 880 | −36.0 | −36.6 / −46.0 | −29.8 / −34.3 | −29.8 / −34.3 | +6.8 / +11.7 |
| 1760 | −36.0 | −39.7 / −40.8 | −28.5 / −34.1 | −28.6 / −34.2 | +11.1 / +6.6 |

The two 4.17.27.8 runs differed by at most 0.1 dB per tone and channel. Maximum acoustic output and absolute equivalence to HyperOS remain open.

The original Dolby software library was also tested separately with the captured HyperOS profile and output level. In Windows shared playback, the 220 Hz test tone increased by 4.4 dB on the left and 4.8 dB on the right compared with ordinary playback. At 440 Hz, the difference was under 1 dB.

The system Dolby test used Qualcomm calibration and DSP registration extensions. Both bound after boot and the built-in output reported a healthy device. Opening a shared PCM16 stream at 48 kHz failed with `0x8007045D`, so the experimental packages were removed. The file trace captured successful opens of `qcadsp8150.mbn`, `avs_config.acdb`, `General_cal.acdb` and `Global_cal.acdb`. The recording reached its 96 MiB limit on boot, before the playback test.

In a later trial, Windows restarted QCADCM while installing calibration package 1.0.2140.12. Qualcomm DSP accepted the new graph on the first registration. An extra device restart submitted it again, and `AVCS_CMD_REGISTER_TOPOLOGIES` (`0x12923`) returned `ADSP_EBADPARAM` (`2`).

Package 1.0.2140.13 submits only the new graph and skips the extra restart. With the stock ADSP configuration, the DSP accepted the graph in a single request. WASAPI still returned `0x8007045D` when opening the stream. Rolling back the calibration extension restored stereo playback in the same Windows session.

With the experimental ADSP extension active, 1.0.2140.13 also registered the Dolby graph once. The DSP returned `ADSP_EFAILED` (1) to `ADM_CMD_DEVICE_OPEN_V7` for both PCM16 and the 32-bit shared format at 48 kHz. WASAPI returned `0x8007045D`. A separate control registered a new graph made of stock modules and opened both formats successfully. The trace places the failure at stream opening. The reason for the DSP rejection remains unknown.

Two MiSound trials also registered and opened the DSP graph. Both then timed out on post-processing parameter commands, including the trial that kept the stock Windows parameter set.

Trial 1.0.2140.17 requested Dolby module preloading. The boot trace confirmed that Windows submitted the file mapping table to Qualcomm FastRPC. In the recorded file service events, neither the module configuration file nor the Dolby library was named. The shared PCM16 and native Windows mix streams both failed to open with `0x8007045D`. After rollback, both streams opened and stereo playback passed on CS35L41 4.17.27.8.

## PCM16 level response

Inspection of the installed 4.17.27.8 binary confirmed that it sets the HyperOS analog gain code 18, enables DRE, and uses the 4 ms volume ramp. With Windows volume fixed, two input levels were compared through shared PCM16 playback at 48 kHz.

| Tone (Hz) | Input change (dB) | Built-in microphone change, left / right (dB) |
| ---: | ---: | ---: |
| 220 | +16.0 | +16.0 / +14.6 |
| 440 | +16.0 | +15.8 / +15.8 |
| 880 | +16.0 | +15.9 / +15.7 |

The input ranged from −36 to −20 dBFS. At 440 Hz, the second harmonic measured 43.7 / 40.5 dB below the fundamental at the lower input level and 30.6 / 28.2 dB below it at the higher level. The measurements include the microphone, capture path and room. Maximum loudness and music quality relative to HyperOS still require matched acoustic tests.

## Standalone Dolby playback on Windows

The original ARM64 HyperOS Dolby library ran in an isolated ARM64 guest hosted by a standalone Windows process. WASAPI played 2,872,320 PCM16 frames (59.84 seconds at 48 kHz) through the built-in speakers with zero buffer underruns. The PCM hashes matched between the Dolby process and the WASAPI receiver. The starting buffer held 128 ms of audio. Process startup took 1.235 seconds.

A separate Float32 test paced the input in 480-frame chunks and processed batches of 1,024 frames at 48 kHz. All 574,464 output frames matched a separate reference render byte for byte. Time from the arrival of a batch's first input frame to output from the processing worker was 73.37 ms at the 95th percentile and 105.89 ms at maximum. This timing covers the standalone process. System integration and comparisons of music and maximum loudness against HyperOS are still pending.

## 24-bit speaker playback

24-bit playback failed in the tested Qualcomm and Cirrus configurations. In one trial with the stock physical link, WASAPI accepted exclusive 24-in-32 on initialization, but the first buffer request failed with `0x80070032`. After rollback and restart, PCM16 and the native Windows mix opened successfully. All four amplifiers were running Cirrus 4.17.27.8, and both stereo channels passed the microphone tone check.


## Source revisions

- [CS35L41 Windows source, map220v](https://github.com/map220v/cs35l41_win/tree/53d367532fa1632accf4e4d20dc0493e2673e077)
- [Nabu audio mixer, LineageOS](https://github.com/LineageOS/android_device_xiaomi_nabu/blob/15f60556d4963748ce809f389cb48ba31c2beb78/audio/mixer_paths_overlay_static.xml)
- [Nabu vendor firmware list, LineageOS](https://github.com/LineageOS/android_device_xiaomi_nabu/blob/15f60556d4963748ce809f389cb48ba31c2beb78/proprietary-files.txt)
- [Nabu board DTS, AwakenOS](https://github.com/AwakenOS-Devices/android_kernel_xiaomi_nabu/blob/2f84d28068bd3c8b9ec4281323ce65d650694efa/arch/arm64/boot/dts/qcom/nabu-sm8150.dtsi#L352)
- [Quiet-speaker report for BSP 2608.03](https://github.com/remtrik-stuff/MiPad5-Windows-Releases/issues/5)
- [Building ARM64 drivers with the WDK](https://learn.microsoft.com/en-us/windows-hardware/drivers/develop/building-arm64-drivers)
