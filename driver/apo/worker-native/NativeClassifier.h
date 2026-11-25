#pragma once
#include <windows.h>
#include <stdint.h>
#include <unicorn/unicorn.h>

// Experimental user-mode adapter. The caller must validate the SHA-pinned
// Android image, audited call graph and owned fixed shared mappings first.
typedef struct NativeClassifierPatch {
    UINT_PTR address;
    uint64_t oldValue;
    uint64_t nativeValue;
} NativeClassifierPatch;

#ifdef __cplusplus
extern "C" {
#endif
__declspec(dllexport) int64_t NativeClassifierLrint(float value);
__declspec(dllexport) void NativeClassifierUnexpectedImport(void);
__declspec(dllexport) DWORD NativeClassifierInvoke(UINT_PTR entry,const uint64_t *arguments,uint64_t fpcr);
__declspec(dllexport) SIZE_T NativeClassifierHookSize(void);
__declspec(dllexport) uc_err NativeClassifierAttach(HMODULE unicorn,uc_engine *guest,void *context,const NativeClassifierPatch *patches,UINT count);
__declspec(dllexport) uc_err NativeClassifierDetach(void *context);
__declspec(dllexport) uint64_t NativeClassifierMetric(void *context,UINT field);
__declspec(dllexport) void NativeClassifierClearMetrics(void *context);
#ifdef __cplusplus
}
#endif
