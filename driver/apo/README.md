# Dolby APO development source · 2.0.0.5-lab

Native ARM64 COM transport, stereo queues, delayed dry fallback and diagnostics. The separate worker runs original Android Dolby code; that library and worker runtime are not included here. This source does not implement MiSound processing.

The stale-reply fix preserves valid processed blocks after discarded old replies. Six queue comparisons, nine boundary/safety cases and seven original-worker lifecycle scenarios passed offline. These checks opened no audio device and do not establish system Dolby or acoustic L/R separation. The transport delay remains 128 ms.

From an ARM64 EWDK 10.0.26100 environment, run `powershell.exe -File .\Build-Apo.ps1 -OutputDirectory <new-folder>`. The lab DLL is unsigned and unregistered; this script does not install anything. `DigitalHarness.cpp` provides the in-memory COM test interface.

`NabuDolbyApo.inf` is the separate driver-bound candidate component. A local package passed Inf2Cat and catalog signature verification, including its INF/DLL members. It excludes the rejected public global-pack registration and still needs a controlled integration trial. No certificate or installer is distributed here.

The current public driver ZIP excludes this prototype. [Measured project status](../../docs/audio-system-progress.md)
