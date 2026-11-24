# PCM24 progress · 24 November 2026

[English](pcm24-progress.md) · [Русский](pcm24-progress.ru.md) · [简体中文](pcm24-progress.zh-CN.md)

**Verified:** the experimental component set now transfers 48 kHz stereo PCM24 in 32-bit containers at the required rate through shared and exclusive WASAPI. All 13 native ARM64 timing checks passed. This is progress in the audio data path; additional loudness and Android acoustic parity have not been demonstrated.

## Before and after

| Exclusive 10 ms buffer | Submitted frames | Audio clock (s) | Supply ratio | Mean notification interval |
| --- | ---: | ---: | ---: | ---: |
| Earlier PCM24 trial (v16) | 481,440 | 20.016729 | 0.501081 | 19.978 ms |
| Decoder valid-bit correction (v18) | 721,440 | 20.010375 | 0.751110 | 13.323 ms |
| Complete playback word-size correction (v20) | 961,440 | 20.006312 | 1.001184 | 9.994 ms |

The supply ratio compares submitted frames with the frames required by the audio clock. Totals include buffer prefill, so a passing ratio can slightly exceed 1. The notification interval describes the buffer cadence, not total input-to-speaker latency.

## What changed

- Corrected speaker format negotiation, COPP width matching and an unsupported cached endpoint format.
- Preserved 24 valid bits when building both the stream-open format and the separate playback decoder format.
- Set PCM24 `sample_word_size` to 32 in the DSP pull-mode command and the actual playback media-format update. The separate encoder converter was not the playback fix.

These values follow the [original Android Qualcomm PCM format contract](https://github.com/MiCode/vendor_qcom_opensource_audio-kernel/blob/d41f8e1a6827294af6a61adbe18544518e750166/include/dsp/apr_audio-v2.h). The PCM24 trials did not increase mixer volume, amplifier gain or protection limits.

## Verified checks

- ✅ Native ARM64 / MMCSS exclusive stream: 20 seconds with a 10 ms buffer.
- ✅ Exclusive stream with a 100 ms buffer, shared stream for 20 seconds, and ten exclusive reopen cycles.
- ✅ Windows accepted a one-second PCM24 tone at −66 dBFS: Initialize, Start and Stop returned S_OK; 48,000 frames were submitted.
- ✅ Recovery in the same Windows boot restored Cirrus 4.17.27.15 and the original Qualcomm modules, settings and PCM16 endpoint format. The native PCM16 recovery check also passed.

Tone submission is an API result, not an acoustic comparison. The amplifier traces contained no amplifier events; they do not prove the absence of protection faults.

## Tested research configuration

| Component | Version |
| --- | --- |
| Qualcomm miniport | 1.0.2100.6 |
| QCADCM | 1.0.2140.3 |
| QCAUDDEV clock handling | 1.0.2100.1 |
| ACSP speaker capabilities | 1.0.2140.20 |
| Speaker/COPP profile | 1.0.2060.17 |
| Cirrus amplifiers | 4.17.27.36 |

The [published driver ZIP](../README.md#download) remains Cirrus 4.17.27.16. It does not include this complete PCM24 research component set. The existing [driver source](../driver/) belongs to that published Cirrus package.

## Remaining work

- ❌ System-wide Dolby and MiSound integration.
- ⏳ Amplifier telemetry and actual speaker-output validation, longer playback, and suspend/resume checks.
- ⏳ Matched music-quality and maximum-loudness comparison with HyperOS, and validation on other tablets.
- ❌ The cause of the earlier loud-music shutdown is still unknown. Maximum-volume safety of the PCM24 set is not established.

[Measurement data](results/pcm24-2026-11-24.json) · [Earlier measurements](baseline.md#24-bit-speaker-playback) · [Build history](reference-build.md#pcm24-transport-24-november-2026) · [Main page](../README.md)
