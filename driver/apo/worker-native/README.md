# Original Dolby classifier adapter · development prototype

ARM64 C++/H bridge for `DAP_CPDP_PVT_classifier_process` and its audited `DAP_CPDP_PVT_mi_vec_exp` dependency from the captured `libswdap.so` SHA256 `207d04317997bbca253157d6038505605335a5982af39d465b249fb52a590539`. The Android library remains external.

The caller must verify the exact image, code bytes, call graph and owned shared mappings. The adapter preserves Windows floating-point state, restricts execution to the verified entry and one thread, rejects invalid GOT descriptors, and restores import bindings before the emulator resumes. The captured disabled/no-input gates skip native execution. This component performs no audio-device I/O or driver installation.

An active-call comparison preserved all changed state exactly. The public EWDK build preserves 196,608 original output frames and persistent guest heap/data, and rejects 11 invalid contracts. The hybrid worker with the preceding equivalent bridge passed seven offline lifecycle scenarios. These are software results; sustained timing gains, system Dolby/MiSound and speaker behavior are unverified. [Evidence](../../../docs/results/audio-native-classifier-2026-11-25.json)

Build with the ARM64 EWDK environment and Unicorn 2.1.4 headers using `Build-NativeClassifier.ps1`. Select a new output directory. The helper DLL is unregistered and excluded from the public driver ZIP.
