# Dolby COM APO progress · 24 November 2026

[English](dolby-apo-progress.md) · [Русский](dolby-apo-progress.ru.md) · [简体中文](dolby-apo-progress.zh-CN.md)

**Proven:** an unregistered native ARM64 COM APO receives the original Android Dolby renderer's Float32 output through a separate process. Regular and irregular callback tests each matched all 196,608 stereo frames byte for byte. **System-wide Dolby and MiSound remain unfinished.**

The original `libswdap.so` runs in an isolated ARM64 guest using captured Dynamic / speaker_landscape parameters verified against HyperOS ACDB. This remains emulated Android code, not a native Windows Dolby library. No EQ, amplifier gain or protection limits were increased.

## What changed

- The default COM factory previously created a passthrough instance. It now accepts the lab transport configuration during initialization and connects separate request/reply queues.
- Arbitrary callback sizes are accumulated into the original renderer's 1,024-frame batches. The prototype uses a fixed 128 ms transport delay.
- Late or unavailable worker output falls back to the original signal at the same frame position. Processing recovered after an injected 120 ms worker pause.
- A new mapping epoch prevents stale results after reset or COM reopen. Exact guest state restoration replaces approximately 260 ms of reconstruction with 2–3 ms in the measured tests.
- Native log and ties-to-even rounding imports preserve the reference result. All 339 math ABI cases and two complete 196,608-frame comparisons passed.

## Evidence

Seven digital scenarios passed: regular callbacks, irregular callbacks, worker exit, temporary pause, state reset, zero-latency unconfigured bypass and configured worker absence. A separate two-open test used the same worker, advanced epochs from 1 to 2, matched 196,608 reference frames on each open and released the mapping after its last owner closed. These tests opened no audio device and installed no driver.

## Remaining work

- ❌ Actual Windows audio-graph initialization, worker broker, component registration and processing-mode validation. The lab initialization payload is not the system property-store initialization used by Windows.
- ❌ MiSound integration and the unresolved Qualcomm DSP module-loading path.
- ⏳ Long-running playback under load, amplifier telemetry, acoustic comparison with Android and maximum-volume safety. The earlier shutdown cause remains unknown.

Windows supports [composite effect registration](https://learn.microsoft.com/en-us/windows-hardware/drivers/audio/pkey-compositefx-endpointeffectclsid), but [streaming support and the driver mixing location](https://learn.microsoft.com/en-us/windows-hardware/drivers/audio/implementing-audio-processing-objects) must be checked before inserting an effect alongside the existing Qualcomm proxy.

[Measurement summary](results/dolby-apo-2026-11-24.json) · [PCM24 progress](pcm24-progress.md) · [Main page](../README.md)
