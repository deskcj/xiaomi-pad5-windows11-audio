# Dolby APO progress · 2026-11-26

[English](dolby-apo-progress.md) · [Русский](dolby-apo-progress.ru.md) · [简体中文](dolby-apo-progress.zh-CN.md)

- **Changed:** unavailable-effect controls reject both states; guest signal warmup restores exact original state; backend installer supports Automatic and SSD logs; MiSound library exposes bounded raw ACDB resources.
- **Passed:** 34 control / 24 factory cases, 10,000 event replacements and 2,048 races; original ON/OFF/ON and reset/replacement comparisons; fresh/reused/warmed SCM installs and rollback; 31 exact ACDB properties + 12 rejection/boundary + 33 NOACCESS cases.
- **Failure retained:** cold .20 produced 32 fallback frames. The warmed run matched the original with zero fallback; universal timing stability is unproven. No new acoustic test or APO .20 audiodg trial occurred.

| Area | Proven | Remaining |
| --- | --- | --- |
| Four amplifiers / UEFI | .20 installed; legacy + Remtrik ID support. | Installation on Remtrik. |
| Dolby | Earlier live SFX; .20 original-output/control tests; independent backend and SCM Automatic. | Permanent graph/cold boot, EffectsManager, latency/recovery and Atmos objects. |
| Stereo | Digital Normal/RAW L/R. | Acoustic isolation of every speaker. |
| MiSound | Original factory/control source; validated ACDB resource views. | Live DSP modules, graph, COPP and processing. |
| PCM24 / quality | Earlier software checks. | Hardware PCM24, HyperOS acoustics, sleep and maximum-volume safety. |

Current tablet: amplifier **4.17.27.20**, APO **2.0.0.4**; the Manual .18 backend was restored. Qualcomm still requires a Windows reboot before a new APO trial. APO .20 is a signed candidate, not hardware-qualified.

[Detailed history](history/dolby-2026-11-26.md) · [26.11 · JSON](results/audio-dolby-runtime-acdb-2026-11-26.json)

The original Android stereo DAP runs in an ARM64 guest with a 128 ms buffer. This is not verified Windows Atmos object rendering. Global preference persistence remains open.
