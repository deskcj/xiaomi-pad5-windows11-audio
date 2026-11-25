# Dolby APO development source · 2.0.0.16-lab

**2.0.0.16-lab:** the packaged original stereo DAP backend runs under SCM
LocalService. Owned worker death can recover inside an active APO stream without
resetting its epoch or SPSC producer cursors. Signed ARM64 .16, 24 COM cases,
eight release checks, seven expiry cases, ten broker scenarios and three exact
original comparisons pass.

Two three-minute quiet device trials completed: .15 had zero dry frames; .16
recovered from an injected worker exit with 86,272 dry frames, including 896
after the recovery snapshot. Timing remains open. Both trials restored APO .4
on kernel .20 without reboot or calibration changes. The runtime-only installer,
identical-version reuse and rollback passed. Permanent APO binding, startup,
the effect toggle and Atmos objects remain unfinished.
[Evidence](../../docs/results/audio-dolby-service-recovery-2026-11-25.json)

**2.0.0.14-lab:** the supervisor verifies the exited worker's process handle and PID, then reclaims queues only after the APO releases the stream. Each slot permits two restart attempts; active-stream recovery remains unfinished. Eight transport/API cases, ten supervisor cases, three original-output comparisons, 24 COM factory cases and 108 settings cases pass. A fresh quiet .14/.36 trial with kernel .20 processed 72,096 wet frames and zero dry frames in real audiodg; four Normal/RAW digital L/R cases passed. Both workers stopped normally and APO .4 was restored. The physical trial did not inject a worker failure or measure acoustic speaker separation. [Evidence](../../docs/results/audio-idle-recovery-live-2026-11-25.json)

**2.0.0.13-lab:** real audiodg SFX activation and quiet nonzero processing now pass after fixing COM aggregation. A private IUnknown manages the inner lifetime; public interfaces delegate to the controlling object. The .12 control returned CLASS_E_NOAGGREGATION in 11 live calls. ARM64 EWDK: 24 factory, 108 settings and seven original-worker scenarios pass. Embedded DLL signing with page hashes and catalog/member verification are available through `Build-ApoPackage.ps1`.

This is a tested lab configuration with a temporary original Android worker and 128 ms buffering, not a standalone permanent Dolby package. .18 / APO .4 were restored; .13/.36 and rollback are retained locally. [Live results and limits](../../docs/results/audio-apo-aggregation-2026-11-25.json)

The identity/lifetime implementation follows [Microsoft COM aggregation rules](https://learn.microsoft.com/en-us/windows/win32/com/aggregation). The observed factory failure and its resolution are retained separately from [Windows APO graph-load diagnostics](https://learn.microsoft.com/en-us/windows-hardware/drivers/audio/implementing-audio-processing-objects).

<details>
<summary>Previous research and builds</summary>


Native ARM64 COM transport, stereo queues, delayed dry fallback and diagnostics. The separate worker runs original Android Dolby code; that library and worker runtime are not included here. This source does not implement MiSound processing.

Audited native [classifier and level-scale adapters](worker-native/) preserve original output/state in the lab worker. After the manual reboot, .11/.36 installed without another reboot and the actual Windows CAPX Default bank read passed. Zero-input streams opened, but Nabu processing counters/events stayed zero. The final rollback preserved prior settings and restored kernel .18 / APO .4 with a passing zero-input stream and no pending adapter restart. System Dolby/MiSound, physical L/R, hardware PCM24 and maximum-volume safety remain unverified. [Current evidence](../../docs/results/audio-config-capx-stereo-2026-11-25.json) · [Native adapter evidence](../../docs/results/audio-native-level-and-live-apo-2026-11-25.json)

The .11 prototype distinguishes an absent property (successful `VT_EMPTY`) from a failed context or settings read. Failed reads use zero-latency bypass and advertise no effect; diagnostic field 4 retains the original error. All 108 native fault/control cases pass, while .10 failed 74. The previous 28 bank-selection, five memory, seven logging and two epoch checks pass. Test fixtures substitute the fixed Global resolver with real Local mappings; no system endpoint is involved.

Normal/irregular output matches the original Dolby for 196,608 frames each. Pause recovery failed in the first run; the .10 control and .11 repeat subsequently passed. That timing sensitivity remains open. The signed .11/.36 pair is prepared locally and uninstalled; current installed APO remains .4. System effects, acoustic L/R and maximum-volume safety are unverified. [Evidence](../../docs/results/audio-apo-property-read-2026-11-25.json)

The .10 prototype honors an explicitly configured bank when it is missing, invalid or occupied: it bypasses without selecting another bank. All 28 native policy cases pass; the .9 control failed 14. The fixed Global resolver is redirected to Local mappings only in the test fixture. Existing memory/logging/epoch/original-worker tests pass, preserving exact output for 196,608 frames. [Evidence](../../docs/results/audio-apo-policy-settings-2026-11-25.json)

`NabuDolbyBound.inf` (.36) declares the Windows 11 Default settings context used by the APO and leaves OEM MFX/EFX registration alone. Native SetupAPI schema checks and signed package verification passed locally. The .10/.36 pair remains uninstalled. Its prepared trial captures absent Nabu contexts and uses [narrow cleanup](../../tools/ApoContextRollback.py); pre-existing or unexpected content is preserved. Cleanup tests use synthetic trees; actual context rollback and system processing are untested.

The .9 prototype allocates each APO state on separate VirtualAlloc pages, avoiding shared heap-page ownership during locking. Three native COM instances verified allocation/teardown; five memory fault, seven logging, two epoch and seven original-worker cases passed. Original output remains exact. The signed .9 component is prepared locally and uninstalled. [Evidence](../../docs/results/audio-isolation-filters-2026-11-25.json)

The .8 prototype removes pageable view-wrapper reads from processing/reset and releases pinned pages after failed locks and unlock. Five native COM memory fault fixtures, seven logging cases, two epoch cases and seven original-worker scenarios passed. Normal and irregular streams still match all 196,608 frames exactly. The pinning/allocation fixture uses substituted APIs and guarded memory; it is not a hardware crash test. A signed .8 component is prepared locally and remains uninstalled. [Evidence](../../docs/results/audio-apo-memory-2026-11-25.json)

The stale-reply fix preserves valid processed blocks after discarded old replies. Six queue comparisons, nine boundary/safety cases and seven original-worker lifecycle scenarios passed offline. These checks opened no audio device and do not establish system Dolby or acoustic L/R separation. The transport delay remains 128 ms.

The .7 prototype adds Windows 11 logging of source identity, mapping name and epoch, and clears old frame counters on reset/reopen. Seven logging contracts, two native counter cases and seven original-worker scenarios passed. `APOProcess` and `Reset` perform no logging. The build script generates the source fingerprint header inside the output folder.

From an ARM64 EWDK 10.0.26100 environment, run `powershell.exe -File .\Build-Apo.ps1 -OutputDirectory <new-folder>`. The lab DLL is unsigned and unregistered; this script does not install anything. `DigitalHarness.cpp` provides the in-memory COM test interface.

`NabuDolbyApo.inf` is the separate driver-bound candidate component. A local package passed Inf2Cat and catalog signature verification, including its INF/DLL members. It excludes the rejected public global-pack registration and still needs a controlled integration trial. No certificate or installer is distributed here.

The current public driver ZIP excludes this prototype. [Measured project status](../../docs/audio-system-progress.md)


</details>
