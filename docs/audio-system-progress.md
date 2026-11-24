# Audio progress · 2026-11-25

[English](audio-system-progress.md) · [Русский](audio-system-progress.ru.md) · [简体中文](audio-system-progress.zh-CN.md)

System Dolby, MiSound, acoustic L/R separation and HyperOS-quality sound remain unconfirmed. No new loudness increase is claimed.

| Area | Before | Proven now |
| --- | --- | --- |
| Amplifier routing | Only CH_BAL was read back. | ✅ Slots 0/1, both DSP inputs and CH_BAL checked; layout matches original HyperOS controls. |
| I²C failures | 20 of 224 simulated failures were propagated; 204 were ignored. | ✅ All 236 candidate failure cases propagated; 16 mismatching readbacks rejected. |
| Installed driver | Working .15 baseline. | ✅ .17 started all four amplifiers; 101,280 zero-input WASAPI frames passed. |
| Dolby APO | Init3 and Windows 11 context settings were unsupported. | ✅ Seven init2 + seven init3 cases; 196,608 reference frames each, exact. System processing unproven. |
| PCM24 | Negotiation and pacing failed. | ✅ 13 timing/reopen checks passed on the existing research set; speaker validation remains. |
| MiSound | DSP dispatch timed out. | ✅ 96 original control-encoding comparisons passed; resource/ack path remains open. |

The .17 register writes on successful paths match the saved .16 source. Fault tests use an in-memory register model. The live WASAPI source contained only zeros; its approximately one-PCM16-LSB meter reading is converter dither. Neither test measures speaker quality.

APO 2.0.0.4 passed standalone Windows 10/11 initialization and original Dolby output comparisons without opening an audio device. Its signed EffectPack/component packages were staged; Windows requested a restart. No processing in normal Windows applications is proven.

## In progress

- ❌ **System Dolby:** verify audio-host loading and processed frames after the requested restart.
- ❌ **MiSound:** resolve original Qualcomm DSP resource loading and parameter acknowledgement.
- ⏳ Physical L/R separation, PCM24 speaker output, HyperOS sound quality and maximum-volume safety still need validation.

Download: **4.17.27.16**. Development source: **4.17.27.17**. The published ZIP does not include system Dolby or MiSound.

[Measured evidence · 2026-11-25](results/audio-routing-apo-2026-11-25.json) · [2026-11-24](results/audio-system-2026-11-24.json)

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
