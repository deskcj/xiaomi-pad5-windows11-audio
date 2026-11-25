# Original Dolby runtime · development

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

The installer installs only the backend, with a unique smoke bank and Manual
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
