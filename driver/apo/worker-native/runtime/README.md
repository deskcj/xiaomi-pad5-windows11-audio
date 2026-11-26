# Original Dolby runtime · development

.21 preserves original Float32 bytes without per-batch Python sample conversion.
The native scanner retains finite/range guards; 64 slowest batch timings preserve
late-stall evidence. Live control and rollback pass; the three-minute run still
has 64 extra fallback frames beyond injected restart. Full installation, seamless
recovery and stability remain open. Experimental ZIP stays local.

.20 rejects controls for unavailable effects. Portable workers now warm one
1024-frame nonzero signal inside the guest before restoring exact original
state and publishing READY. First request/render/reply timings are separate;
startup timings exclude module imports. Warmed ON/OFF/ON, reset and replacement
comparisons pass. The initial cold candidate's 32 fallback frames are retained
as a failure, not hidden by the later pass.

`Install-Runtime.ps1 -StartMode Automatic` configures SCM automatic startup.
Fresh/reused isolated installations with LocalService/session 0, logs on H:,
cooperative shutdown and restored previous start mode pass. Cold boot and
permanent audio graph integration remain untested. `Build-RuntimeBundle.ps1`
accepts `-LogDirectory` for a dedicated SSD directory and `-AudioGraphBank`
to stage the default bank; this does not install/bind the included APO.
[.20 evidence](../../../../docs/results/audio-dolby-runtime-acdb-2026-11-26.json)

.19 uses wire5 and exposes per-stream effect control with a 10 ms fade. Workers
continue the original state/input sequence during OFF; intentional bypass has
its own counter. The default ON path still matches original output exactly.
The paced software ON→OFF→ON comparison passed 196,608 total frames, including
958 transition frames. Notification, concurrent control and settings-failure
checks pass. .19 is not installed or hardware-tested, and global preference /
Windows EffectsManager integration remain open. C:still runs the .18 smoke
backend; the prepared .19 ZIP is retained on H:.
[.19 evidence](../../../../docs/results/audio-dolby-toggle-software-2026-11-25.json)

Current candidate is .18, wire4. A bounded 16-interval history distinguishes
startup fallback from later stalls. The new sole-consumer startup discard
advances only the committed request prefix; producer indices remain intact.
Seven native ownership/wrap/corruption/concurrent checks (10,000 records),
24 COM cases and three original-output comparisons pass.

A three-minute quiet .17 trial recovered with one 1.808 s dry interval and no
later dry interval. .18 hardware testing is blocked by the Qualcomm adapter's
pending restart after metadata rollback; a device-only restart could not finish
the operation. Windows was not rebooted. The global-meter zero check initially
saw a transient signal; a separate recheck passed. .20 / APO .4 remain installed.
[New evidence](../../../../docs/results/audio-dolby-history-startup-2026-11-25.json)

The original Android stereo DAP renderer runs in an ARM64 guest with qualified
native classifier, level and math helpers. It uses the captured Dynamic /
speaker_landscape profile with its original pregain. This does not establish
Windows Atmos object rendering or MiSound processing.

`PortableOriginalWorker.py` resolves its isolated Python, backend, resources and
native libraries within the runtime directory. The profile is validated during
staging; processing reads no research tree or ACDB database. The 26 MiB backend
matched all 196,608 original reference frames exactly from an unrelated directory.
Adapters are public here; captured resources and qualified dependency binaries
are retained in the local package.

`NabuDolbyService.cpp` runs as SCM-owned LocalService in session 0. It resolves
resources beside its EXE, waits for broker readiness and cooperatively stops
owned nested kill-on-close Jobs. It starts manually; automatic startup and cold
boot have not been tested.

`NabuDolbyBroker.cpp` verifies owned process handles and worker PIDs. Confirmed
death releases only worker ownership. The live APO epoch and queue cursors stay
intact. The replacement is the sole request consumer and skips entirely expired
records; discontinuities restore exact original DSP state. Each slot permits two
restart attempts, with all attempt logs retained. Broker/service open no audio
endpoint and dispatch no Qualcomm DSP commands.

ARM64 EWDK .16 passed 24 COM, eight ownership, seven expiry and ten broker cases;
three original-output comparisons remain exact. Three-minute quiet .15 recorded
8,635,296 wet / zero dry frames. Quiet .16 recovered an injected worker exit in
the same audiodg/epoch: 86,272 dry frames (1.80 s total), including 896 after the
recovery snapshot. Timing and 128 ms latency remain open. APO .4 / kernel .20
and calibration were restored; acoustic L/R and maximum-volume safety unproved.

[Measured evidence](../../../../docs/results/audio-dolby-service-recovery-2026-11-25.json)

From an ARM64 EWDK environment:

```powershell
.\Build-Service.ps1 -OutputDirectory <new-service-build>
.\Build-Broker.ps1 -OutputDirectory <new-broker-build>
```

Stage an already qualified, signed runtime plus its matching APO:

```powershell
.\Build-RuntimeBundle.ps1 -RuntimeDirectory <qualified-runtime> `
  -ApoPackageDirectory <signed-apo-package> -OutputDirectory <new-bundle> `
  -RuntimeVersion 1.0.0-lab02
```

The builder copies manifest-listed files, verifies signatures/matching APO
hashes and emits ZIP/member digests. `Install-Runtime.ps1` installs a versioned
LocalService backend and saves previous service configuration on SSD. Identical
versions can be reused; differing content is preserved and rejected. Rollback
restores the previous owned service, retaining both directories. Actual fresh
and reused installations, cooperative stop and rollback passed.

`Build-AudioGraphBundle.ps1` additionally packages the qualified signed APO,
bound SFX extension and portable graph installer. It discovers the physical
speaker, preserves OEM effects and exports baseline packages before mutation.
Both Windows audio services are quiesced for graph changes. Backend startup is
SCM Automatic; exact driver/settings/service rollback is retained on SSD.
The current add-on deliberately accepts only the qualified .20/.4/Qualcomm
1.0.2100.0 baseline. It retains unexpected registrations, devices and newer APOs.
Eight offline safety contracts and an actual injected-installation rollback pass.
Full graph .21/lab09 is now installed: Windows ON/OFF/ON, three quiet minutes
with zero failure fallback and idle service restart followed by a new stream
passed. This establishes original stereo DAP processing, not Atmos objects,
cold-boot qualification, physical speaker isolation or seamless active recovery.
[Evidence](../../../../docs/results/audio-dolby-graph-install-live-2026-11-26.json)

The separate `Install-Runtime.ps1` installs only the backend, with a unique smoke bank and Manual
start. The included signed APO candidate still needs controlled graph binding.
It changes no drivers, endpoint properties, gain/protection, automatic startup
or Windows boot state. The public kernel ZIP remains .20 without this runtime.

Broker contract:

```text
NabuDolbyBroker.exe <bridge.dll> <worker.exe> <worker-script-or-dash> <new-output>
                   <bank-name> <duration-seconds> <slots> <source-fingerprint>
```

Slots are 1–4; duration is 0–3600 seconds (0 runs until stopped). `Stop.request`
in the owned output requests cooperative shutdown. Duplicate banks, other
owners' mappings and unconfirmed/live process handles are rejected.
