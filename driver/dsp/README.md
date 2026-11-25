# MiSound support source

`MiSoundProtocol.c/.h` encode the original Android control commands. `MiSoundSpeakerFilters.c/.h` read the SPMD/MMFD filter tables from the two captured nabu HyperOS speaker resources without changing their binary32 coefficients or scales. Invalid lengths, unsupported layouts, nonfinite/unstable coefficients and ambiguous selected rates are rejected without modifying the output.

This is a user-mode support library. It does not send DSP commands, process audio, decode the OAL/dynamics sections or enable system MiSound. The CS35L41 driver and current public ZIP do not apply these tables.

From an ARM64 EWDK environment, run `powershell.exe -File .\Build-MiSoundSupport.ps1 -OutputDirectory <new-folder>`. It builds both components into `NabuMiSoundSupport.lib` and records the source hashes. No installation or device access occurs.

[Measured project status](../../docs/audio-system-progress.md)
