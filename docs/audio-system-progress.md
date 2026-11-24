# Audio progress: before and after · 24 November 2026

[English](audio-system-progress.md) · [Русский](audio-system-progress.ru.md) · [简体中文](audio-system-progress.zh-CN.md)

The two main unfinished features are **system-wide Dolby** and **MiSound**. Original Dolby processing and PCM24 transport have passed their respective research tests; neither result proves Android sound quality on Windows.

| Task | Before | What is proven now | What remains |
| --- | --- | --- | --- |
| Amplifier DSP and calibration | Direct serial-to-DAC path on map220v; the first local build included one tablet's calibration. | ✅ Four amplifiers start with original HyperOS protection/music profiles; the public 4.17.27.16 ZIP uses the target tablet's factory calibration. | Validate calibration on other tablets and compare acoustic output. |
| PCM24 | Format negotiation failed; partial frame delivery in intermediate trials. | ✅ All 13 native ARM64 WASAPI timing/reopen checks passed on the research component set. | Amplifier telemetry, speaker quality and maximum-volume validation before release. |
| Original Dolby software | The standard COM factory was passthrough. | ✅ Exact original-library output through COM, failure fallback and reopen; official system-init structure and a shared worker broker also passed standalone tests. | ❌ Make the Windows audio graph load and process this APO. |
| APO installation | Only an unregistered laboratory initialization existed. | ✅ Signed component/extension packages installed; the APO child was healthy. Removal and baseline recovery passed without reboot. | The audio host produced no APO initialization events or processed frames; installation alone is not Dolby activation. |
| MiSound | Effect parameters timed out in the Qualcomm DSP path. | ✅ Executing the original Android HAL callback showed that it validates buffers without processing samples. | ❌ Resolve the original DSP resource-loading and parameter path; emulating this HAL callback cannot provide the speaker effect. |
| Loudness and shutdown | An early experimental build shut the tablet down during loud music. | An earlier PCM16 4.17.27.15 test completed 3 min 33 s at 100% Windows volume with external power, without shutdown or increased recorded protection counters. | The original cause and universal maximum-volume safety remain unproven. No new acoustic/loudness improvement is claimed in this update. |

## Newly verified

The final system-init test passed six initialization cases: missing configuration, missing broker, discovery-only, RAW, normal processing and an occupied broker slot. Normal processing matched **196,608 stereo frames** byte for byte, with **529 callbacks, zero unexpected frames and zero dry fallback frames**. Discovery, RAW and broker contention used zero-latency bypass. This test never registered an APO or opened an audio device.

The Windows component trial accepted two signed packages and created a healthy APO device. After restarting the audio hosts, normal Windows playback still supplied **zero APO callbacks and zero processed frames**. The candidate was removed and the working baseline recovered; Windows was not rebooted. This result narrows the integration problem but does not close it.

Four original-machine-code checks of `libmisoundfx.so!MiSound_process` at ELF address `0xb168` passed. Valid separate buffers left the output untouched; valid in-place data remained unchanged. Invalid arguments returned `-22`. The 68-byte callback is buffer validation, so speaker MiSound must be implemented through its DSP path.

## Active work

- ❌ Dolby: inspect the effect selection and loading actually used by the Windows audio host, then validate processing in normal applications.
- ❌ MiSound: trace original Android/Qualcomm DSP resources and parameter application.
- ⏳ Confirm long playback, amplifier telemetry, quality against HyperOS and power safety on the final combined configuration.

The published download remains **4.17.27.16**. There is no new Dolby/MiSound release. Original EQ, gain and speaker protection limits were not increased.

[Measurement summary](results/audio-system-2026-11-24.json) · [Dolby COM evidence](dolby-apo-progress.md) · [PCM24 evidence](pcm24-progress.md) · [Main page](../README.md)
