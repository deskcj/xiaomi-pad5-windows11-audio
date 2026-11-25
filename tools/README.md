# Audio development probes

From an ARM64 EWDK environment, build with `powershell.exe -File .\Build-AudioProbe.ps1 -Probe QuietStereoProbe -OutputDirectory <new-folder>`, or select `ApoSettingsProbe`. The build preserves older output folders and records source/executable hashes. These executables are diagnostics, not driver installers.

`QuietStereoProbe.cpp` submits separate 800 Hz L/R stimuli in normal and RAW mode, with a source peak of 0.001 and an independently verified 0.01 gain on its own render stream. It writes no device or session volume/mute setting. It refuses a busy or muted endpoint and unsupported formats; a failed case stops the sequence. The source peak multiplied by stream gain is not a bound on downstream APO/codec processing. Real loopback peaks and both coherent-tone and total RMS ratios are reported, including PCM16 dither. A loopback result cannot establish which physical amplifier/speaker produced sound, HyperOS quality or maximum-volume safety. [Microsoft stream-volume API](https://learn.microsoft.com/en-us/windows/win32/api/audioclient/nf-audioclient-iaudiostreamvolume-setallvolumes)

`ApoSettingsProbe.cpp` uses the real Windows CAPX property-store API to read the Nabu context's Default bank on the current default render endpoint. It writes no properties and activates neither an APO nor an audio stream. A matching bank establishes settings availability to this caller, not audiodg activation or processing.

[Recorded software and device results](../docs/results/audio-config-capx-stereo-2026-11-25.json)

`ApoClassActivationProbe` verifies ordinary in-process COM activation and identifies the loaded DLL, without calling Initialize or opening audio. `ApoFactoryContractProbe` loads an explicitly named DLL and checks creation failures, aggregation, public/private COM identity, reference balance and unloadability (24 cases, including 64 interface cycles); it does not register or initialize an APO.

`ApoSecurityProbe <audiodg-pid> <HKLM-relative-key> [section:<name> ...]` reads process/token and object security data. It never impersonates or changes privileges/ACLs. `accesscheck_performed=false` means the check was unavailable, not a demonstrated DACL denial. Source/executable hashes are recorded by `Build-AudioProbe.ps1`. Select these probe names with `-Probe` in an ARM64 EWDK environment.

[Live aggregation failure, fix and quiet evidence](../docs/results/audio-apo-aggregation-2026-11-25.json)
