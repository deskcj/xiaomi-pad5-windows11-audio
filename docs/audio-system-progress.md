# Audio progress · 2026-11-25

[English](audio-system-progress.md) · [Русский](audio-system-progress.ru.md) · [简体中文](audio-system-progress.zh-CN.md)

**New · original native level stage and hardware installation:** the audited Android DRC level calculation now has an ARM64 C++/H adapter. Four calls match every mutable region, including the guest-stack output; 196,608 output frames and persistent state match exactly. Fourteen argument guards, thirteen hook/page contracts and seven worker scenarios pass. Moving mapping ownership checks to attachment removes the measured per-call query overhead; a sustained whole-worker speed gain is still unproven.

The signed .11/.36 pair was actually installed and tested with zero input. Windows recreated the speaker endpoint: rollback now follows the same physical adapter and preserves existing contexts. The stream opened, but Dolby counters/events stayed zero. A classic-registration diagnostic required a reboot and was rolled back. Current .18 / APO .4 passes a silent stream; the adapter retains a restart flag, so further installs are stopped. System Dolby/MiSound, physical L/R, hardware PCM24, HyperOS parity and maximum-volume safety remain unverified.

[Native code and live-trial evidence](results/audio-native-level-and-live-apo-2026-11-25.json) · [C++/H source](../driver/apo/worker-native/)

<details>
<summary>Earlier source checks · 25 November</summary>

**New · original native classifier prototype:** an ARM64 C++/H adapter executes the audited Android classifier slice without changing the profile, gain or algorithm. An active call preserved all 658 changed bytes. The public EWDK build preserves 196,608 original output frames and persistent guest heap/data; 11 invalid contracts reject. The hybrid worker with the preceding equivalent bridge passed seven offline scenarios.

Whole-stream speed improvement is not proven: results vary, and the public comparison was slower overall. The earlier pause failure remains in the evidence. Ghidra/PE inspection also confirms that three apparent live-GET function references are exception metadata, not a callable MiSound interface. System Dolby/MiSound, physical L/R, hardware PCM24 and maximum-volume safety remain open. Installed .18 / APO .4 is unchanged; .11/.36 is prepared.

[Native/state evidence](results/audio-native-classifier-2026-11-25.json) · [C++/H source](../driver/apo/worker-native/)



**New · Dolby APO .11:** failed context/settings reads now use zero-latency bypass and retain the error, instead of selecting the common profile. **108 native cases pass**; .10 failed 74. The earlier 28 bank-selection cases, five memory cases, seven logging cases and two epoch cases pass. ARM64 .11 is built, signed and uninstalled.

Normal/irregular output exactly matches the original Dolby for 196,608 frames each. The first pause test did not recover the effect before the stream ended; the .10 control and .11 repeat recovered. This timing limitation remains open. Ghidra also reviewed the captured Android protection handlers; no gain/protection setting changed. System Dolby/MiSound, physical L/R, PCM24 and maximum-volume safety are unverified. Installed .18 / APO .4 remains; .11/.36 is prepared.

[.11 source/tests and retained failure](results/audio-apo-property-read-2026-11-25.json)



**New · matched DSP reply source:** the C/H readback helper now checks the standard APR envelope, both endpoints, transaction token, V6 opcode and DSP error status before reading parameter data. A basic GET receipt with zero status still returns “no parameter data”. Native checks passed on 112 original Windows mapped GET readbacks, 100 constructed in-band packets, 21 rejected replies and 73 real memory-boundary cases. Earlier 76 readback memory cases also pass on the new library.

The pinned official Xiaomi audio-kernel source and captured HyperOS DTS/mixer confirm DSP_A/TDM, amplifier slots 4/5, and Channel Swap On for BR/TR, Off for BL/TL. The conditional I2S hardware reversal does not apply to that capture. Windows has its own 0/1 slot pair; copying Android slot numbers into another serial frame would not prove L/R. The four-amp Channel Swap table agrees, but physical separation still needs measurement. No driver, gain, protection or sound changed; system Dolby/MiSound, physical L/R and maximum-volume safety remain open. Installed .18 / APO .4 remains; .10/.36 is prepared.

[Reply and stereo source evidence](results/audio-apr-stereo-source-2026-11-25.json) · [C/H source](../driver/dsp/)



**New · bounded DSP readback source:** `QualcommPpReadback.c/.h` pack the original Windows ADM GET body and validate returned module/instance/parameter, reserved bits and exact buffer length. All **112 original comparisons**, including 91 factory blocks, **76 native memory cases** and 13 malformed-response cases pass. The nine-component ARM64 EWDK library builds; prior route, command-plan and factory-filter tests still pass.

Ghidra confirms that the Windows host-stream getter uses ACDB `0x1326b`, while live ADM GET uses `0x1035e`. No live GET was sent: the active COPP, owned DSP mapping and matched reply must be established first. A stock Windows host-cache read using the newly linked library passed and matched the installed file; it remains separate from DSP state. The last quiet digital L/R result is the earlier evidence below. Installed kernel .18 / APO .4 is unchanged; .10/.36 is prepared. System Dolby/MiSound, physical L/R and maximum-volume safety remain open.

[New source checks](results/audio-adm-readback-2026-11-25.json) · [Earlier real cache/quiet results](results/audio-adcm-host-selector-2026-11-25.json) · [C/H source](../driver/dsp/)

**New · native ADCM calibration access:** two new C/H helpers preserve the original stream-mask selection and construct the Windows host-calibration GET buffer. The selector is separate from an Android ACDB Device ID; the strict ADM check rejects truncated flags. All 3,146 original selector comparisons, 95 original getter comparisons and 27/21 native memory cases passed.

The new C buffer was used with the running OEM ADCM driver: a stock PBE-enable read matched the installed Windows value, while the original MiSound-enable query returned Win32 error 31 without changing its buffer. This reads the host ACDB cache, not DSP state; the OEM reports zero API bytes despite writing the stock buffer, which is retained in the evidence. Earlier route/filter/command tests still pass, and 91 factory PP payloads now match with the original COPP lookup executed against emulated context contents. Four new quiet Normal/RAW L/R cases passed at unchanged 6% volume. System effects, physical speakers and maximum-volume safety remain open; APO .10/.36 is prepared and uninstalled.

[Measured evidence](results/audio-adcm-host-selector-2026-11-25.json) · [C/H source](../driver/dsp/)

**New · verified Qualcomm route and payload source:** current configured Windows calibration selects topology `0x10000009` for Device 73 / AppType `0x11134` and `0x11138`; its seven modules contain neither MiSound nor Dolby. The new C/H validator rejects mismatched/ambiguous file routes without remapping Android IDs. The Windows parameter packer preserves all **91 factory payloads** exactly against original QCADCM execution and rejects oversized buffers or truncated instance IDs.

The six-component ARM64 EWDK library passed 901 route cases, 1,770 route memory cases and 49 packing memory cases. Earlier 864 MiSound comparisons and ten factory filters still pass. Four quiet Normal/RAW L/R tests passed at unchanged 6% volume (opposite coherent tone ≤0.0787%). This is digital separation; actual system effects, physical speakers and maximum-volume safety remain open. Software Dolby APO binding is a separate Windows graph check. The installed kernel is .18 / APO .4; signed .10/.36 remains prepared and uninstalled.

[Measured evidence](results/audio-acdb-pp-2026-11-25.json) · [C/H source](../driver/dsp/)

**New · original MiSound speaker plan:** the C/H helper reproduces MiSoundDSP_SetSPKParam instead of emitting only a scalar control. All **864 comparisons** against actual original ARM64 execution passed: 480 supported cases with exact commands/final routes and 384 unsupported cases with untouched output. The original emits 2/4/10/12 commands across AppType 0x11130/0x11134; its persistent-rate table is 44100/48000/88200/96000/192000. Eight-byte payloads and the special mode rules retain their original bits and order. This is calibration/control routing, not a hardware-rate or speaker-isolation claim.

The four-component ARM64 EWDK library passed 53 native cases using real NOACCESS pages and 13 invalid-input cases. The ten factory tables and fourteen rejection cases still pass; ordinary installed-file reads still match HyperOS. Four original functions were exported in Ghidra with raw instructions. Android service dispatch/logging was intercepted in the reference execution; no DSP command, playback or driver installation occurred. This closes the missing command-plan source, while actual system MiSound/Dolby, physical L/R and maximum-volume safety remain open.

[Measured evidence](results/audio-misound-speaker-plan-2026-11-25.json) · [MiSound C/H source](../driver/dsp/)

**Previous · APO .10 / Windows 11 settings .36:** an explicitly selected unavailable, invalid or occupied Dolby bank no longer falls through to the common bank. The .9 control failed 14 of 28 native policy fixtures; .10 passed all 28. Only the fixed Global resolver was redirected to real Local test mappings. This is a reproducible software fix, not a system-audio or shutdown-cause diagnosis.

The .36 extension declares the Default store for the exact context used by APOInitSystemEffects3, instead of placing the custom bank property in the legacy flat store. Native SetupAPI parsing passed and rejected five malformed contracts, including an OEM MFX overwrite. Existing OEM MFX/EFX values are untouched by this INF. Signed .10/.36 catalogs and members passed verification; read-only preflight passed. Five memory, seven logging, two epoch and seven original-worker regressions passed; normal/irregular output remains exact for 196,608 frames. Fifteen synthetic context-cleanup cases pass; actual installation, context read and rollback remain untested.

Ghidra reviewed the original AMP0/LMT0 per-channel constructors. Compared code already exists in Windows firmware; this does not prove live amplifier protection. No new driver was installed or sound started. System Dolby/MiSound, physical L/R and maximum-volume safety remain open.

[Measured evidence](results/audio-apo-policy-settings-2026-11-25.json) · [APO source](../driver/apo/) · [Windows 11 settings framework](https://learn.microsoft.com/en-us/windows-hardware/drivers/audio/windows-11-apis-for-audio-processing-objects)

**Previous · MiSound resource reader v2:** the ARM64 EWDK library now validates every profile/module before exposing raw parameters. Both factory resources retain all **502 exact records** (7 profiles and 19 modules each). Thirteen malformed cases produced no callbacks; ten damaged-tail comparisons now reject files the earlier filter-only reader accepted. All ten original filter tables and fourteen rejection cases still pass. A native test with real NOACCESS guard pages passed **30,770 truncation/mutation cases**. Structurally accepted mutations are not factory-approved calibration.

The four-channel file adds eight MMS0 records across NORM/MOVI/GAME/SYSS, **32 bytes per profile**. Their selectors and value bits are retained without inventing parameter units or physical speaker positions. Ghidra traced the original special filter-attachment path; its indices alone do not prove acoustic L/R. Ordinary host reads match HyperOS; successful DSP resource opening remains unobserved.

Ten further original functions were reviewed. Raw instructions confirm the original **176400 → 192000 Hz factory-table selection**, now preserved by the reader. Both alias cases match every table bit, while fourteen nearby/invalid-rate cases still reject. This does not resample audio or prove playback at those rates.

The original Qualcomm driver searches DSDT for **SOSI**, which is present inside its search window. Four isolated original-code cache tests show that a failed first lookup can remain cached; the SOC provider and OS imports were mocked. This is a testable failure condition, not the proven cause of the live timeout. No kernel patch is justified by these tests. APO .9 remains prepared and uninstalled; this update changed no installed driver, gain or protection setting.

[Measured data](results/audio-misound-resource-2026-11-25.json) · [MiSound source](../driver/dsp/) · [Xiaomi SOC layout](https://github.com/MiCode/Xiaomi_Kernel_OpenSource/blob/71b6effd2dfa8f703dd1ea17acd1323b2fd3cf6c/drivers/soc/qcom/socinfo.c)

**Previous · APO 2.0.0.9-lab / MiSound support:** APO state now has its own VirtualAlloc allocation, so page locking does not depend on heap-object boundaries. Native inspection of three unregistered instances confirmed separate allocations, reclaim of the middle allocation and usable remaining instances. The .8 control correctly used heap storage; retained heap pages are not labelled a leak or crash. Five memory fault, seven logging, two epoch and seven original-worker cases passed again; normal/irregular output still matches 196,608 frames exactly. Signed .9 and read-only preflight passed; no candidate was installed.

The new ARM64 MiSound support library reads the captured SPMD/MMFD tables. Ten tables matched every binary32 coefficient/scale bit; fourteen malformed/unsupported cases preserved the caller's output. The linked EWDK library passed the same comparisons. Both installed resources were read normally by Windows and matched HyperOS. Their SPMD regions are identical; the differing OAL/dynamics/channel processing is not implemented. Four more original Ghidra functions identified the HPMD/SPMD/HRIR subregion parser and pointer helpers. This does not prove DSP resource opening or system MiSound. No new audible improvement, acoustic L/R or maximum-volume safety is claimed.

[Source and evidence](results/audio-isolation-filters-2026-11-25.json) · [MiSound C/H source](../driver/dsp/) · [Microsoft page-unlock semantics](https://learn.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-virtualunlock)

**Previous · APO 2.0.0.8-lab:** processing and reset now use a cached, pinned mapping instead of dereferencing the pageable view wrapper. Failed lock/allocation paths and UnlockForProcess release their pinned pages; successful retry clears the old lock error. Five native COM fault fixtures passed (.7 failed all five). Allocation/pinning APIs are substituted and view pages deliberately protected: this verifies the code path, not an observed device crash. Seven logging, two counter and seven original-worker cases passed; normal/irregular streams still match 196,608 frames exactly. The signed .8 package passed catalog/member checks and read-only preflight, but is not installed.

Four new quiet digital L/R controls passed at unchanged 6% volume; worst opposite 800 Hz tone ratio: 0.000673. A separate very quiet preprocessed-file delivery test failed template correlation (0.491) and captured a zero right channel with only PCM16-sized values. It is not accepted as Dolby evidence; equal-peak channel-only controls passed independently. Acoustic L/R, system effects and maximum-volume safety remain open.

Ghidra reviewed eight further HyperOS resource/parser/channel-stage functions; the compared code windows already match Windows firmware. The loader selects OAL1/NORM/OAL2; its file reader distinguishes open failure, long name and short read. A decompiler cache return was corrected against raw `mux` instructions (Boolean 0/1). These findings guide the next resource-opening trace; they do not confirm MiSound processing.

[Source and test evidence](results/audio-apo-memory-2026-11-25.json)

No loudness or HyperOS-quality improvement is claimed. Quiet digital loopback is not an acoustic speaker test.

**Previous · APO 2.0.0.7-lab:** added Windows 11 logging of the source fingerprint, mapping name and epoch. Old frame counters are cleared when a stream resets or reopens, preventing results from a previous owner being counted as new processing. Seven logging contracts, two native counter tests and seven original-worker scenarios passed; normal/irregular renders each remained exact for 196,608 frames. Logging is limited to Initialize/LockForProcess and excluded from Reset/APOProcess. Seventeen synthetic observer cases reject wrong builds, streams, generations and helper PIDs. These are software checks; the signed .7/.35 trial is prepared but has not installed a candidate.

**MiSound investigation:** the installed `qcadsprpcd` kernel driver is running and its ACPI device reports no problem. Ghidra identified `apps_std` registration, the `audiopd` branch and the resource path handler. Two archived traces contain only two daemon messages named `FastRPCDriverD0Entry`; they do not establish an active listener, successful resource opening or ADM acknowledgement. Starting another user-mode listener is therefore not justified by the earlier absence hypothesis.

[Latest build and evidence](results/audio-apo-observability-2026-11-25.json) · [Microsoft logging framework](https://learn.microsoft.com/en-us/windows-hardware/drivers/audio/windows-11-apis-for-audio-processing-objects) · [Reset constraints](https://learn.microsoft.com/en-us/windows/win32/api/audioenginebaseapo/nf-audioenginebaseapo-iaudioprocessingobject-reset)

</details>

| Area | Before | Proven now |
| --- | --- | --- |
| IRQ/GPIO and routing | IRQ setup ignored I²C errors; only CH_BAL was checked. | ✅ .18 rejects failed IRQ setup; real readbacks passed on all four amplifiers. |
| Digital L/R | Separation unmeasured. | ✅ Four quiet cases passed; opposite 800 Hz tone <0.1%. UID/I²C board mapping matches Android. Acoustic isolation remains unverified. |
| Dolby APO | An unavailable explicit bank could select the common fallback; settings used the flat store. | ✅ .10 honors explicit selection; .36 declares the correct context. 28 software cases pass. ❌ System processing unverified. |
| PCM24 | Format/pacing failures. | ✅ Earlier 13 timing/reopen cases passed. Hardware PCM24 remains unverified. |
| MiSound / DSP routes | No complete plan, cache GET or checked DSP readback. | ✅ Exact factory data, stream selector, real cache read and bounded ADM GET/readback source. ❌ MiSound query fails; live DSP processing unverified. |

GPIO interrupt configuration and the protection mask now propagate errors and verify readbacks before DSP/amp enablement. The healthy writes, firmware, coefficients and factory calibration remain unchanged. EWDK, catalog generation and signing passed.

Actual kernel events confirmed route and IRQ checks on UID 0–3. Four -60 dBFS source tests passed without changing the 6% endpoint volume. The earlier total-RMS criterion also failed on baseline .17 because PCM16 dither was large relative to this very quiet signal; the corrected measurement compares the coherent 800 Hz tone and retains raw RMS/peaks. Neither criterion measures acoustic speaker quality.

The corrected Dolby COMPUTER extension registered its global pack without a reboot. Explicit selection succeeded, but audiodg still supplied zero APO callbacks/processed frames. The temporary extension was removed and the selection restored. Protection counters showed no overtemperature/overexcursion in this small-signal session; a latched VmonLowFlag was observed before codec restart. This does not identify the earlier loud-music shutdown or prove maximum-volume safety.

The latest quiet retest passed four normal/RAW L/R cases at unchanged 6% endpoint volume. The worst opposite-channel 800 Hz amplitude ratio was 0.000910; total RAW RMS still includes PCM16 dither. Raw ACPI UID/I²C descriptors and original Android DTS independently matched all four board positions. This establishes the configured mapping, not acoustic speaker isolation.

A fresh standalone Dolby init3 render matched all 196,608 reference frames (seven initialization cases, zero unexpected/dry frames). The rebuilt ARM64 MiSound encoder again matched 96 cases executed on original Android machine code. Neither test proves system effects. Ghidra plus read-only endpoint properties identified the public global-pack OEM filter. Driver-bound SFX extension .35 passed catalog/signature checks and read-only preflight; it is not installed. Two resources and selected MiSound DSP regions match HyperOS; actual resource opening/parameter acknowledgement remains unresolved.

[Latest evidence](results/audio-effects-integration-2026-11-25.json)

**Dolby 2.0.0.5-lab:** the old queue budget counted valid replies as well as stale replies. With six stale records, only 2,048 of 4,800 / 8,192 valid frames stayed processed; the candidate preserves all 4,800 / 8,192. Six comparisons and nine boundary/safety cases passed, including cursor wrap, recovery and rejecting NaN/Inf. A fresh ARM64 build from the [published source](../driver/apo/) passed all seven original-worker scenarios; normal and irregular renders each matched 196,608 frames. The DLL remains unsigned, unregistered and uninstalled; delay stays 128 ms.

**MiSound file routing:** six Windows FastRPC functions were reviewed in Ghidra. Four isolated original-machine-code cases reproduced the default `%DriverData%\Qualcomm\fastRPC` search and the `0x102020` kernel path query. All OS/CRT imports, device acquisition and FILE bookkeeping were intercepted; the kernel resolver and file opening were mocked. The current speaker resource mappings and bytes match HyperOS, but actual listener ownership, resource opening and ADM acknowledgement remain unproven. The absent `adsp_avs_config.acdb` mapping in this device's list is an observation, not an established failure cause.

The .5 component package also passed Inf2Cat and catalog/member signature verification. It omits public global-pack registration; exact .4 INF/CAT/DLL rollback files were retained. The combined .5/.35 runner passed read-only preflight and refused the non-admin token without installation, workers, sound or UAC. The actual integration and rollback trial remain untested.

[Queue, build and routing evidence](results/audio-transport-2026-11-25.json) · [Qualcomm daemon architecture](https://github.com/qualcomm/fastrpc/blob/development/Docs/daemons.md)

## In progress

- ❌ **System Dolby:** validate the driver-bound SFX candidate; public global packs are inapplicable to this OEM endpoint.
- ❌ **MiSound:** resolve original DSP resource loading and parameter acknowledgement.
- ⏳ Physical per-speaker L/R, PCM24 output, HyperOS quality and maximum-volume safety remain unverified.

Download: **4.17.27.16**. Development source: **4.17.27.18**. The public ZIP does not include system Dolby or MiSound.

[Measured evidence · 2026-11-25](results/audio-postboot-2026-11-25.json) · [Earlier](results/audio-routing-apo-2026-11-25.json)

<details>
<summary>Earlier evidence (24–25 November 2026)</summary>

The final system-init test passed six initialization cases: missing configuration, missing broker, discovery-only, RAW, normal processing and an occupied broker slot. Normal processing matched **196,608 stereo frames** byte for byte, with **529 callbacks, zero unexpected frames and zero dry fallback frames**. Discovery, RAW and broker contention used zero-latency bypass. This test never registered an APO or opened an audio device.

The Windows component trial accepted two signed packages and created a healthy APO device. After restarting the audio hosts, normal Windows playback still supplied **zero APO callbacks and zero processed frames**. The candidate was removed and the working baseline recovered; Windows was not rebooted. This result narrows the integration problem but does not close it.

Four original-machine-code checks of `libmisoundfx.so!MiSound_process` at ELF address `0xb168` passed. Valid separate buffers left the output untouched; valid in-place data remained unchanged. Invalid arguments returned `-22`. The 68-byte callback is buffer validation, so speaker MiSound must be implemented through its DSP path.

The native ARM64 MiSound control encoder now matches **96 offline cases executed on the original Android machine code**. Parameter IDs, module/topology changes and unpadded Base64 output match; unsupported types leave the output untouched. [C source and header](../driver/dsp/) contain no audio I/O or amplifier gain settings. DSP dispatch is intercepted in the reference test, so these results do not prove that Windows applies the effect.

A later silent selection diagnostic returned `0x800700FF` on the first default stream and recorded an `audiodg` heap failure (`0xC0000374`) in the same interval. The WER loaded-module list did not contain the Nabu DLL. The cause is not established; all baseline initializations passed after rollback. No audio was started or submitted in this diagnostic.

A further prototype fix separates **effect discovery** from claiming a processing worker. Previously a discovery-only instance returned an empty effect list; it now reports the configured Dolby effect without consuming a slot, including when the worker is occupied. Seven initialization cases passed, and all 196,608 reference frames remained exact. This corrects the prototype contract; it did not activate system Dolby.

The latest SFX and valid legacy-only GFX trials used a signed Driver Store DLL and property-change notification, with no audio-service restart. Both zero-input streams passed, but still produced zero APO processing. The meter stayed at approximately one PCM16 least-significant bit (converter dither), not a music/tone signal. Both temporary packages were removed and the baseline restored without errors.

</details>
