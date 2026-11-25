#pragma once
#include <windows.h>
#include <stdint.h>
#include <unicorn/unicorn.h>

// Lab-only native call to the audited, SHA-pinned lvl_calc_drc_scale function.
// The caller verifies the original code/data bytes and owns the fixed shared
// image/heap/guest-stack mappings. No OS audio, calibration, gain or protection writes.
// Attach verifies full committed RW heap/stack allocations once. Their owner
// keeps both mappings and protections unchanged, with exclusive single-thread
// guest access until Detach. The hook bounds every argument and nested vector.
// Direct Invoke independently checks mapping ownership on each call. Neither
// API verifies calibration values or changes the original processing profile.
// Context storage is zero initialized, remains alive while attached, and is
// attached/detached on its owner thread. Duplicate attachment is rejected.
#ifdef __cplusplus
extern "C" {
#endif
__declspec(dllexport) DWORD NativeLevelScaleInvoke(UINT_PTR entry,
    const uint64_t *arguments, const float *floatingArguments, uint64_t fpcr);
__declspec(dllexport) SIZE_T NativeLevelScaleHookSize(void);
__declspec(dllexport) uc_err NativeLevelScaleAttach(HMODULE unicorn,
    uc_engine *guest, void *context);
__declspec(dllexport) uc_err NativeLevelScaleDetach(void *context);
__declspec(dllexport) uint64_t NativeLevelScaleMetric(void *context, UINT field);
__declspec(dllexport) void NativeLevelScaleClearMetrics(void *context);
#ifdef __cplusplus
}
#endif
