#pragma once
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
#ifdef NABU_MISOUND_RESOURCE_TEST_DLL
#define NABU_MISOUND_RESOURCE_API __declspec(dllexport)
#else
#define NABU_MISOUND_RESOURCE_API
#endif
typedef struct NabuMiSoundResourceInfo {
    uint32_t ProfileCount;
    uint32_t ModuleCount;
    uint32_t ParameterCount;
    uint32_t FilterPayloadOffset;
    uint32_t FilterPayloadBytes;
} NabuMiSoundResourceInfo;
typedef struct NabuMiSoundParameterRecord {
    uint32_t ProfileIndex;
    uint32_t ModuleIndex;
    uint32_t RecordIndex;
    uint32_t ProfileTag;
    uint32_t ModuleTag;
    uint32_t SelectorA;
    uint32_t SelectorB;
    uint32_t ValueBits;
    uint32_t SourceOffset;
} NabuMiSoundParameterRecord;
typedef int (*NabuMiSoundParameterVisitor)(const NabuMiSoundParameterRecord *, void *);
enum NabuMiSoundResourceScanResult {
    NabuMiSoundResourceScanOk = 0,
    NabuMiSoundResourceScanInvalid = 1,
    NabuMiSoundResourceScanUnsupported = 2,
    NabuMiSoundResourceScanArgument = 4,
    NabuMiSoundResourceScanStopped = 5
};
/* Captured nabu SPKD/SPMD/NUMM profile layout. First validate the complete buffer,
 * then optionally visit exact raw selectors/value bits in their original order.
 * The visitor must not modify the resource; records are valid during the callback.
 * Return nonzero to continue, zero to stop. Output is optional and assigned only
 * on success. No parameter units, gain defaults, DSP commands or audio processing.
 * SPMD coefficient validity is checked separately by MiSoundSpeakerFilters.
 */
NABU_MISOUND_RESOURCE_API int NabuMiSoundScanResource(const void *resource,
    size_t bytes, NabuMiSoundParameterVisitor visitor, void *context,
    NabuMiSoundResourceInfo *output);
#ifdef __cplusplus
}
#endif
