# Xiaomi Pad 5 speakers on Windows 11 ARM64

Research and diagnostics for the four internal speakers on Xiaomi Pad 5 (nabu), using the Qualcomm Aqstic audio stack and Cirrus Logic CS35L41 amplifiers.

[По-русски](docs/README.ru.md) · [Baseline and source references](docs/baseline.md) · [Reference build](docs/reference-build.md)

## Current status

Windows enumerates four CS35L41 devices. A quiet WASAPI test confirmed that both digital stereo channels reach the built-in speakers endpoint at 48 kHz. Per-speaker acoustic output and the Windows-to-Android loudness difference still need measurement.

This repository currently contains a diagnostic and investigation notes. There is no driver release yet.

## Quiet stereo test

In Windows Sound settings, select the built-in Xiaomi Pad 5 speakers. Pause other playback, then run from 64-bit PowerShell 7:

    ./tools/Measure-SpeakerBaseline.ps1 -PlayQuietTones

The tool sends one second of 440 Hz to the left channel, pauses, then sends the same tone to the right channel. The source peak is −44.4 dBFS. The result shows the digital peaks at the Windows endpoint. For an acoustic check, listen near each grille during both tones and record the tablet orientation, master volume and audible positions. A separate microphone at a fixed distance can compare Windows and Android.
