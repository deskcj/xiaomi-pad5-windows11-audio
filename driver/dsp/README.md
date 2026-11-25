# MiSound support source

`MiSoundProtocol.c/.h` encode the original Android control commands. `MiSoundResource.c/.h` validate the captured SPKD/SPMD/NUMM container and all named profile/module records before visiting their raw selectors and binary32 value bits. `MiSoundSpeakerFilters.c/.h` then read the SPMD/MMFD tables without changing their coefficients or scales. Damaged profile tails now reject the resource even when its filter section is intact.

`MiSoundSpeakerPlan.c/.h` reproduce the original `MiSoundDSP_SetSPKParam` sequence for 4- and 8-byte controls. This includes both AppType IDs, active/persistent routes, the five caller-supplied factory rates, mode exceptions and exact unpadded Base64. The returned plan contains 2, 4, 10 or 12 commands. All 864 comparisons against execution of the original Android ARM64 code passed, plus 53 native guard-page and 13 invalid-input cases. [Speaker-plan evidence](../../docs/results/audio-misound-speaker-plan-2026-11-25.json)

The original `DSPVarFS` table contains 44100/48000/88200/96000/192000. These are control/calibration routing values, not proof of Windows playback at those rates. The helper preserves the original final route, including its rate and persist flag; it does not choose values, apply controls or enable MiSound. Unsupported requests leave its output untouched even though the Android wrapper may update its own internal route without dispatching a command.

The two captured resources contain seven profiles and nineteen modules each: 247 stereo and 255 four-channel parameter records. All 502 records were compared exactly. Thirteen malformed cases, ten filter-only regression comparisons and 30,770 native bounds/mutation cases passed; the ten original filter tables and fourteen rejection cases still pass. [Evidence](../../docs/results/audio-misound-resource-2026-11-25.json)

Original HyperOS instructions also select the existing 192000-Hz table for a 176400-Hz request. The reader now preserves that selection, reporting the factory table rate without resampling. Both alias cases match every original table bit; fourteen nearby/invalid-rate cases still reject without changing the output. This does not establish hardware playback at those rates.

This is a user-mode support library. Structural acceptance is not approval of calibration values. It preserves raw OAL/dynamics records without implementing their algorithms, units or DSP dispatch. It does not process audio or enable system MiSound. The CS35L41 driver and current public ZIP do not apply these tables.

The input must remain unchanged throughout a scan. Complete structural validation precedes callbacks; cancellation can still occur after earlier callbacks, leaving the optional summary untouched. Collect records before attempting a DSP transaction. Profile/module order and both selectors are preserved; DSP indices do not establish physical left/right speaker positions.

From an ARM64 EWDK environment, run `powershell.exe -File .\Build-MiSoundSupport.ps1 -OutputDirectory <new-folder>`. It builds all four components into `NabuMiSoundSupport.lib` and records the source hashes. No installation or device access occurs.

[Measured project status](../../docs/audio-system-progress.md)
