# Audited original Dolby adapters · development prototypes

ARM64 C++/H adapters for selected functions from captured `libswdap.so`, SHA256 `207d04317997bbca253157d6038505605335a5982af39d465b249fb52a590539`. The Android library remains external. The entire library cannot be run directly on Windows: it uses Android TLS and Windows-reserved `x18`.

`NativeClassifier.cpp/.h` handles the audited classifier and its `mi_vec_exp` dependency. It preserves floating-point state, checks its fixed entry/thread and GOT descriptors, and restores imports before emulation resumes. Its earlier active/output/state and eleven invalid-contract comparisons remain valid. [Classifier evidence](../../../docs/results/audio-native-classifier-2026-11-25.json)

`NativeLevelScale.cpp/.h` adds the original 792-byte DRC level calculation. Its complete call graph has no imports, indirect branches, TLS or `x18`. The typed ABI retains two floating and six integer arguments. Captured output resides in the caller's guest stack, so four active comparisons include that stack and every mutable region. The record index is a software DRC index, not a physical speaker position.

Attachment verifies full owned RW heap/guest-stack mappings once; the owner keeps them committed, unchanged and exclusive until detachment. Each call still bounds direct and nested pointers. Direct invocation independently queries ownership on every call. The caller verifies the SHA-pinned original image/code/data and complete call graph. Zero-initialized context storage stays alive on the owner thread; duplicate attachment is rejected.

The EWDK builds preserve 196,608 original output frames and persistent state. Fourteen argument guards, thirteen real private-page/hook contracts and seven hybrid-worker scenarios pass. The first prototype's repeated mapping queries made it slower; moving those checks to attachment removed that overhead. The later whole-stream difference was small, so sustained speed, louder/better sound and resolution of the earlier pause failure are not claimed. [Level-stage and live-trial evidence](../../../docs/results/audio-native-level-and-live-apo-2026-11-25.json)

Use an ARM64 EWDK environment with Unicorn 2.1.4 headers and a new output directory:

- `Build-NativeClassifier.ps1` builds the classifier helper.
- `Build-NativeLevelScale.ps1` builds the level helper.

Both helpers are experimental, unsigned/unregistered and excluded from the public driver ZIP. Neither opens an audio device, dispatches DSP commands, changes the profile/gain/protection, installs a driver or proves system effects. The actual .11/.36 installation trial opened zero-input streams but did not activate Nabu processing; current .18 / APO .4 is restored. Physical L/R, MiSound, PCM24, HyperOS parity and maximum-volume safety remain open.
