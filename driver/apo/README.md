# Dolby APO development source · 2.0.0.10-lab

Native ARM64 COM transport, stereo queues, delayed dry fallback and diagnostics. The separate worker runs original Android Dolby code; that library and worker runtime are not included here. This source does not implement MiSound processing.

The .10 prototype honors an explicitly configured bank when it is missing, invalid or occupied: it bypasses without selecting another bank. All 28 native policy cases pass; the .9 control failed 14. The fixed Global resolver is redirected to Local mappings only in the test fixture. Existing memory/logging/epoch/original-worker tests pass, preserving exact output for 196,608 frames. [Evidence](../../docs/results/audio-apo-policy-settings-2026-11-25.json)

`NabuDolbyBound.inf` (.36) declares the Windows 11 Default settings context used by the APO and leaves OEM MFX/EFX registration alone. Native SetupAPI schema checks and signed package verification passed locally. The .10/.36 pair remains uninstalled. Its prepared trial captures absent Nabu contexts and uses [narrow cleanup](../../tools/ApoContextRollback.py); pre-existing or unexpected content is preserved. Cleanup tests use synthetic trees; actual context rollback and system processing are untested.

The .9 prototype allocates each APO state on separate VirtualAlloc pages, avoiding shared heap-page ownership during locking. Three native COM instances verified allocation/teardown; five memory fault, seven logging, two epoch and seven original-worker cases passed. Original output remains exact. The signed .9 component is prepared locally and uninstalled. [Evidence](../../docs/results/audio-isolation-filters-2026-11-25.json)

The .8 prototype removes pageable view-wrapper reads from processing/reset and releases pinned pages after failed locks and unlock. Five native COM memory fault fixtures, seven logging cases, two epoch cases and seven original-worker scenarios passed. Normal and irregular streams still match all 196,608 frames exactly. The pinning/allocation fixture uses substituted APIs and guarded memory; it is not a hardware crash test. A signed .8 component is prepared locally and remains uninstalled. [Evidence](../../docs/results/audio-apo-memory-2026-11-25.json)

The stale-reply fix preserves valid processed blocks after discarded old replies. Six queue comparisons, nine boundary/safety cases and seven original-worker lifecycle scenarios passed offline. These checks opened no audio device and do not establish system Dolby or acoustic L/R separation. The transport delay remains 128 ms.

The .7 prototype adds Windows 11 logging of source identity, mapping name and epoch, and clears old frame counters on reset/reopen. Seven logging contracts, two native counter cases and seven original-worker scenarios passed. `APOProcess` and `Reset` perform no logging. The build script generates the source fingerprint header inside the output folder.

From an ARM64 EWDK 10.0.26100 environment, run `powershell.exe -File .\Build-Apo.ps1 -OutputDirectory <new-folder>`. The lab DLL is unsigned and unregistered; this script does not install anything. `DigitalHarness.cpp` provides the in-memory COM test interface.

`NabuDolbyApo.inf` is the separate driver-bound candidate component. A local package passed Inf2Cat and catalog signature verification, including its INF/DLL members. It excludes the rejected public global-pack registration and still needs a controlled integration trial. No certificate or installer is distributed here.

The current public driver ZIP excludes this prototype. [Measured project status](../../docs/audio-system-progress.md)
