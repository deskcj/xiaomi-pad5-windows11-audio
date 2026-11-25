#pragma once
#include <stddef.h>
#include <stdint.h>

/* Decoder for the SPMD/MMFD filter section in the two captured nabu HyperOS
 * speaker resources. It only reads coefficients: no DSP/amp I/O or gain changes.
 * This is a user-mode validation component, not the complete MiSound algorithm.
 */
#define NABU_MISOUND_MAX_SOS 8
#ifdef __cplusplus
extern "C" {
#endif
#ifdef NABU_MISOUND_FILTER_TEST_DLL
#define NABU_MISOUND_FILTER_API __declspec(dllexport)
#else
#define NABU_MISOUND_FILTER_API
#endif
typedef struct NabuMiSoundSpeakerFilters {
    uint32_t SampleRate;
    uint32_t SectionCount;
    float Coefficients[NABU_MISOUND_MAX_SOS][6]; /* b0 b1 b2 a0 a1 a2 */
    float Scales[NABU_MISOUND_MAX_SOS + 1];
} NabuMiSoundSpeakerFilters;
enum NabuMiSoundFilterResult {
    NabuMiSoundFiltersRead = 0,
    NabuMiSoundResourceInvalid = 1,
    NabuMiSoundResourceUnsupported = 2,
    NabuMiSoundRateAbsent = 3,
    NabuMiSoundFilterArgumentInvalid = 4
};
/* Caller selects the factory resource and stream rate. The original DSP selects
 * its 192000-Hz table for a 176400-Hz request; SampleRate reports the table rate.
 * No interpolation, resampling, inferred profile or default values. Output stays
 * untouched on failure. OAL/dynamics algorithms are not implemented here.
 */
NABU_MISOUND_FILTER_API int NabuMiSoundReadSpeakerFilters(
    const void *resource, size_t bytes, uint32_t sampleRate,
    NabuMiSoundSpeakerFilters *output);
#ifdef __cplusplus
}
#endif
