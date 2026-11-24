# Audio progress · 2026-11-25

[English](audio-system-progress.md) · [Русский](audio-system-progress.ru.md) · [简体中文](audio-system-progress.zh-CN.md)

No loudness or HyperOS-quality improvement is claimed. Quiet digital loopback is not an acoustic speaker test.

| Area | Before | Proven now |
| --- | --- | --- |
| IRQ/GPIO and routing | IRQ setup ignored I²C errors; only CH_BAL was checked. | ✅ .18 rejects failed IRQ setup; real readbacks passed on all four amplifiers. |
| Digital L/R | Separation unmeasured. | ✅ Four quiet 800 Hz loopback cases passed, normal and RAW. Physical speaker positions remain unverified. |
| Dolby APO | Global pack attached under AUDD; no system processing. | ✅ COMPUTER enumeration and selection accepted. ❌ Zero APO processing frames. |
| PCM24 / MiSound | Format/pacing failures; DSP parameters timed out. | ✅ Earlier 13 PCM24 and 96 encoder cases. ❌ MiSound DSP processing and acoustic parity remain open. |

GPIO interrupt configuration and the protection mask now propagate errors and verify readbacks before DSP/amp enablement. The healthy writes, firmware, coefficients and factory calibration remain unchanged. EWDK, catalog generation and signing passed.

Actual kernel events confirmed route and IRQ checks on UID 0–3. Four -60 dBFS source tests passed without changing the 6% endpoint volume. The earlier total-RMS criterion also failed on baseline .17 because PCM16 dither was large relative to this very quiet signal; the corrected measurement compares the coherent 800 Hz tone and retains raw RMS/peaks. Neither criterion measures acoustic speaker quality.

The corrected Dolby COMPUTER extension registered its global pack without a reboot. Explicit selection succeeded, but audiodg still supplied zero APO callbacks/processed frames. The temporary extension was removed and the selection restored. Protection counters showed no overtemperature/overexcursion in this small-signal session; a latched VmonLowFlag was observed before codec restart. This does not identify the earlier loud-music shutdown or prove maximum-volume safety.

## In progress

- ❌ **System Dolby:** resolve endpoint eligibility/graph selection; registration and manual selection do not yet load the APO.
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
