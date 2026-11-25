# Dolby COM APO progress · 24 November 2026

[English](dolby-apo-progress.md) · [Русский](dolby-apo-progress.ru.md) · [简体中文](dolby-apo-progress.zh-CN.md)

### 2026-11-25 — Dolby .19: effect control verified in software

- **Code:** ON/OFF through IAudioSystemEffects3, a 10 ms delay-aligned crossfade, owned notification handles and separate intentional-bypass telemetry. Original DSP state continues while OFF; default ON remains byte-exact.
- **Proof:** 24 COM factory, 14 control / 10,000 concurrent changes, 11 crossfade guards and 108 settings-failure cases pass. Original ON→OFF→ON: 148,129 exact Dolby, 47,521 exact input and 958 crossfade frames; zero unexpected fallback. Three ordinary original-output comparisons also pass. These opened no audio device.
- **Open:** .19 is built/signed and packaged on SSD, not installed or tested in audiodg. Adapter restart flag still blocks live deployment. Qualcomm initialization can hide topology-registration errors; Ghidra confirmed the caller/resource path, not live processing. Global preference, startup, physical L/R and PCM24 remain unfinished.

[Evidence](results/audio-dolby-toggle-software-2026-11-25.json)

### 2026-11-25 — Dolby .17 diagnostics; .18 startup candidate

- **Device .17:** three-minute quiet recovery test: one dry interval, frames 122,880–209,663 (1.808 s); no later dry frames in this run. The initial zero-stream verification caught a transient global meter signal; a separate zero recheck and FX readback passed.
- **Code .18:** retain 16 dry intervals and discard the committed startup queue prefix without resetting the producer. Signed ARM64 build: 24 COM, seven concurrent queue checks (10,000 records) and three exact original-output comparisons pass. Hardware .18 has not run.
- **Blocker:** during metadata rollback the Qualcomm adapter vetoed query-removal and set `NeedsRestart`. Device-only restart cannot finish the pending operation; Windows was not rebooted. .20 / APO .4 remain installed. ZIP is prepared locally; toggle, startup, MiSound, acoustic L/R and PCM24 stay open.

[Evidence](results/audio-dolby-history-startup-2026-11-25.json)

### 2026-11-25 — Portable Dolby service and active-stream recovery (.16)

- **Before → now:** research-path worker → packaged LocalService backend; restart after stream release → replacement inside the same live stream. Signed .16 and 24 COM / 8 ownership / 7 expiry / 10 broker cases pass; three original-output comparisons remain exact.
- **Device:** three-minute quiet .15: 8,635,296 wet / zero dry frames. Quiet .16 worker-kill trial recovered in the same audiodg/epoch, with 86,272 dry frames (1.80 s total); 896 accumulated after the recovery snapshot. APO .4 / kernel .20 and calibration restored, no reboot. Runtime-only ZIP installer, reuse and rollback passed.
- **Open:** intermittent fallback, 128 ms latency, effect toggle, permanent APO/startup, Atmos objects, MiSound, acoustic four-speaker L/R and hardware PCM24. The public kernel ZIP remains .20; the runtime ZIP is retained locally for development.

[Evidence](results/audio-dolby-service-recovery-2026-11-25.json)

**2026-11-25 — Bounded Dolby runtime and quiet live verification**

The supervisor now waits for worker readiness, rejects duplicate banks and manages only its own Job Object. Bounded counters replace full audio/packet logs; file-sharing failures are isolated from DSP processing. Ten native lifecycle/fault cases pass, and normal/irregular output still matches all 196,608 original frames; the reset comparison also passes.

A real quiet .13/.36 trial passed with the new supervisor and original Android worker. The last normal stream had 72,096 wet frames and zero dry frames; four Normal/RAW L/R cases passed without changing device volume. Both workers stopped cooperatively, all owned mappings were released, and .18 / APO .4 returned healthy. This remains an experimental 128 ms setup with external backend dependencies; service deployment, recovery, MiSound, acoustic L/R, PCM24 and maximum-volume safety are unfinished.

[Source / Исходники / 源码](../driver/apo/worker-native/runtime/) · [Evidence / Данные / 数据](results/audio-runtime-broker-2026-11-25.json)

<details>
<summary>Previous research and tests</summary>

**2026-11-25 — Fixed APO aggregation; live Dolby processing**

The .12 control returned `0x80040110` for 11 aggregated creations in two audiodg processes. .13 now supplies a private IUnknown and delegates public identity/reference counts to the controlling object. This fixes a demonstrated system-activation failure; ordinary non-aggregated creation still works.

| Area | Confirmed now | Remaining |
| --- | --- | --- |
| Dolby SFX | ✅ .13 creates, initializes and processes in real audiodg; quiet nonzero stream tested. | ❌ Permanent worker package, sustained timing and latency. |
| Digital L/R | ✅ Four Normal/RAW cases at unchanged device volume. | ❌ Isolation of the four physical speakers. |
| MiSound | ✅ Exact factory/config/command source checks; enable fan-out reviewed in Ghidra. | ❌ Live Qualcomm DSP graph/resource integration. |
| PCM24 / protection | Earlier software checks retained. | ❌ Hardware PCM24, HyperOS acoustics and maximum-volume safety. |

The tested setup uses a temporary original Android Dolby worker and a 128 ms buffer. Four very quiet 800 Hz Normal/RAW L/R cases passed; opposite coherent tone ≤0.955%, with PCM16 dither retained in the measurements. These results establish digital separation and bounded live SFX processing, not physical speaker isolation, better sound or maximum-volume safety.

Current tablet: four .18 amplifiers / APO .4 restored; no reboot, calibration, gain or protection changes. Signed .13/.36, sources and rollback are retained on SSD for subsequent tests. The public .16 ZIP remains unchanged and contains neither system Dolby nor MiSound.

[Evidence / Данные / 数据](results/audio-apo-aggregation-2026-11-25.json)



> [Newer system-integration results and before/after comparison](audio-system-progress.md).

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

- ❌ Actual Windows audio-graph activation and processing. Official property-store initialization, the worker broker and component installation have since passed their separate checks; [current system-integration results](audio-system-progress.md) distinguish these from live processing.
- ❌ MiSound integration and the unresolved Qualcomm DSP module-loading path.
- ⏳ Long-running playback under load, amplifier telemetry, acoustic comparison with Android and maximum-volume safety. The earlier shutdown cause remains unknown.

Windows supports [composite effect registration](https://learn.microsoft.com/en-us/windows-hardware/drivers/audio/pkey-compositefx-endpointeffectclsid), but [streaming support and the driver mixing location](https://learn.microsoft.com/en-us/windows-hardware/drivers/audio/implementing-audio-processing-objects) must be checked before inserting an effect alongside the existing Qualcomm proxy.

[Measurement summary](results/dolby-apo-2026-11-24.json) · [PCM24 progress](pcm24-progress.md) · [Main page](../README.md)

</details>
