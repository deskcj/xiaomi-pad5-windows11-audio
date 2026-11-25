# Dolby runtime supervisor · development

`NabuDolbyBroker.cpp` creates fresh transport slots, starts owned workers inside
a Windows Job Object and publishes the APO bank after all workers report ready.
It monitors worker exit, closes the bank and stops its workers on completion.
Closing an abruptly terminated broker's job also terminates its owned children.
Duplicate banks are rejected; another owner's mappings and processes are retained.
[Windows Job Object lifecycle](https://learn.microsoft.com/en-us/windows/win32/procthread/job-objects)

`BoundedOriginalWorker.py` keeps the qualified original Android renderer,
classifier and level helper. It writes periodic counters, hashes and recent
epoch peaks instead of saving audio or packet logs. Timing history is limited
to 2048 blocks, context resets to 64 and epochs to 16. Report-file sharing errors
are retried briefly and counted independently of DSP processing.

ARM64 EWDK compilation, ten native lifecycle/failure cases, four original-output
comparisons and a real quiet .13/.36 trial passed. Ordinary and irregular streams
each preserve all 196,608 reference frames. The quiet trial's last normal stream
recorded 72,096 wet frames, zero dry frames and cooperative worker shutdown.
[Measured results](../../../../docs/results/audio-runtime-broker-2026-11-25.json)

The external Android renderer still runs inside an ARM64 guest with audited
native helpers and 128 ms buffering. A relocatable backend package, service
deployment, worker restart/recovery and sustained scheduling remain open.
Startup registration is not performed by these tools. Kernel/codec gain,
calibration and amplifier protection are untouched.

Build from an ARM64 EWDK environment:

```powershell
powershell.exe -File .\Build-Broker.ps1 -OutputDirectory <new-folder>
```

The command contract is:

```text
NabuDolbyBroker.exe <bridge.dll> <worker.exe> <worker-script-or-dash> <new-output>
                   <bank-name> <duration-seconds> <slots> <source-fingerprint>
```

Use `-` for a native worker; otherwise the broker invokes the worker executable
with its script. Each worker receives the transport name, a fresh output folder
and the bridge DLL path. Slots are 1–4; duration is 0–3600 seconds, with 0 meaning
run until stopped. `Stop.request` in the owned output folder requests cooperative
shutdown. Ctrl+C is supported when attached to a console. The broker opens no
audio endpoint and installs no drivers.

The current original worker needs explicitly configured, previously qualified
external resources:

- `NABU_DOLBY_SUPPORT_ROOT`: Bridge, OriginalState and MathImports support folder.
- `NABU_DOLBY_RENDER_ROOT`: original renderer and benchmark dependency folder.
- `NABU_DOLBY_NATIVE_ROOT`: qualified SharedLevelScale adapter folder.
- `NABU_CLASSIFIER_BUILD`: selected qualified classifier helper build.
- `NABU_DOLBY_BRIDGE_SHA256`: exact bridge DLL digest, checked before processing.

These paths describe the current research backend; they are recorded in the
prepared local package rather than inferred from the chat or current directory.
Real system activation uses the verified .14 APO and bound .36 settings; source
fingerprint and DLL hashes must match their tested manifests. The current public
kernel driver ZIP does not include this experimental runtime.

Worker claims now store their PID. The supervisor validates a signaled process
handle against that PID before reclaiming an idle slot. If an APO still owns the
stream, readiness is withdrawn and recovery waits for release; queue cursors are
not reset during active production. Each slot has at most two restart attempts,
with previous worker logs retained in separate attempt folders. Active-stream
recovery and a complete relocatable backend/service installation remain open.

The .14 quiet trial on kernel .20 confirms normal live processing and cleanup.
Worker failure/restart was exercised separately with paced COM and original
Android DSP software comparisons; it was not injected into the physical trial.
