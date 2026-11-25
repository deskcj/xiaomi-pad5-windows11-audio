# Install the Xiaomi Pad 5 audio driver

This test release requires Xiaomi Pad 5 (nabu), Windows 11 ARM64, Windows Test Mode and four working CS35L41 devices with hardware ID `ACPI\10133541` (legacy UEFI) or `ACPI\CSCL3541` (Remtrik 2611.21). A firmware upgrade is not required for the legacy ID. Windows Test Mode takes effect after Windows restarts.

## Factory calibration

Copy the following files from `/mnt/vendor/persist/audio/` on **the same tablet's original HyperOS installation** to a folder on Windows. Use a saved copy or Android access with permission to read that directory.

- `cs35l41_cal_br.bin` and `cs35l41_cal_br.txt`
- `cs35l41_cal_tr.bin` and `cs35l41_cal_tr.txt`
- `cs35l41_cal_bl.bin` and `cs35l41_cal_bl.txt`
- `cs35l41_cal_tl.bin` and `cs35l41_cal_tl.txt`

The installer checks the binary and text records for each speaker. Keep your factory files and the exported rollback package with your own backups.

## Install

Extract the Driver ZIP. Open PowerShell as administrator in the extracted folder and run:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\Install.ps1 -CalibrationPath 'D:\NabuCalibration'
```

If Windows reports that the test certificate is untrusted, verify the publisher. To explicitly trust the included test certificate for this test release, add `-TrustTestCertificate`. This imports the public certificate into the local machine's Trusted Root and Trusted Publishers stores.

The script backs up the current package in `rollback\`, asks Windows to bind the driver from the Driver Store and verifies that all four amplifiers start. Keep the extracted folder so you can restore the previous driver.

## Restore the previous driver

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\Install.ps1 -Action Rollback
```

Use the folder from which you originally installed the driver. The rollback restores the exported package and the previous calibration setting.
