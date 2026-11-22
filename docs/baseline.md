# Nabu speaker baseline — 2026-11-23

## Windows measurements

| Layer | Observed |
| --- | --- |
| Built-in output | Qualcomm Aqstic, stereo, 48 kHz |
| Audio adapter | Xiaomi 1.0.2100.0, QCAUD |
| Audio DSP and calibration manager | Qualcomm 1.0.2140.0, qcadcm |
| Amplifiers | Four CS35L41 devices, status OK, hardware ID ACPI\10133541 |
| Installed amplifier driver | map220v 16.45.25.900 |

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

A separate trial build groups UIDs 0/1 and 2/3 to test stereo in the orientation where BR/TR are physically on the right. It keeps the reference build's protection profiles and gain settings. Test each grille with the same tones and orientation before and after the change. The pairing is a lead on stereo placement. The cause of the low acoustic output is still open.


## Source revisions

- [CS35L41 Windows source, map220v](https://github.com/map220v/cs35l41_win/tree/53d367532fa1632accf4e4d20dc0493e2673e077)
- [Nabu audio mixer, LineageOS](https://github.com/LineageOS/android_device_xiaomi_nabu/blob/15f60556d4963748ce809f389cb48ba31c2beb78/audio/mixer_paths_overlay_static.xml)
- [Nabu vendor firmware list, LineageOS](https://github.com/LineageOS/android_device_xiaomi_nabu/blob/15f60556d4963748ce809f389cb48ba31c2beb78/proprietary-files.txt)
- [Nabu board DTS, AwakenOS](https://github.com/AwakenOS-Devices/android_kernel_xiaomi_nabu/blob/2f84d28068bd3c8b9ec4281323ce65d650694efa/arch/arm64/boot/dts/qcom/nabu-sm8150.dtsi#L352)
- [Quiet-speaker report for BSP 2608.03](https://github.com/remtrik-stuff/MiPad5-Windows-Releases/issues/5)
- [Building ARM64 drivers with the WDK](https://learn.microsoft.com/en-us/windows-hardware/drivers/develop/building-arm64-drivers)
