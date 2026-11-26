# Original Dolby runtime · experimental

Current qualified trial: APO .22 / SFX .37 / backend lab13. The original Android
stereo DAP renderer uses the captured Dynamic / speaker_landscape profile in an
ARM64 guest with audited native helpers. Atmos object rendering and MiSound DSP
are not implemented.

A private warmed spare opens no transport before an assignment from its owning
broker. Confirmed worker exit releases only worker ownership; promotion retains
the active APO epoch and queues. The spare is outside the visible bank. Standby
initialization runs below Audio priority; replenishment waits five idle seconds.
`Build-RuntimeBundle.ps1 -WarmSpare` stages the explicit `WarmSpare=1` setting.

The installed quiet lab13 trial recovered the same audiodg/stream with one 30 ms
failure fallback. ON/OFF/ON, the next stream and three quiet minutes passed:
8,634,816 wet frames, zero soak fallback. Runtime upgrade and exact lab11 rollback
passed. Four paced original-output comparisons remain exact. This establishes
the measured software/Windows path, not acoustic speaker isolation or universal
stability. [Evidence](../../../../docs/results/audio-dolby-warm-recovery-2026-11-26.json)

Telemetry serialization and file I/O run on a separate BELOW_NORMAL thread. The
processing owner captures immutable snapshots into a one-entry coalescing queue;
the writer never accesses original DSP or transport state. Shutdown drain is
bounded. The synchronous candidate failed its long trial and was rolled back;
its results remain on SSD.

The standalone LocalService runtime resolves its resources beside its EXE and
uses the LocalService profile for logs. It requires no chat, laboratory process,
working directory or connected SSD. Saved CAPX preference remains handled by .22
and the provisioned .37 User store. Latency is still 128 ms. Gapless recovery,
cold boot, Atmos objects, MiSound DSP and physical audio qualification remain open.

Build broker/service from an ARM64 EWDK environment with `Build-Broker.ps1` and
`Build-Service.ps1`, using new output directories. `Build-AudioGraphBundle.ps1`
stages the matching signed APO/SFX/backend and graph installer; fresh installation
requires its documented baseline. Preserve the installation state for rollback.
The runtime-only installer upgrades the backend of an already matching .22/.37
graph; it does not install or bind an APO. Keep packages and rollback on SSD.