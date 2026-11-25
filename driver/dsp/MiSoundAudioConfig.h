#pragma once
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef struct NabuMiSoundAudioConfig {
    uint8_t Raw[112];
    uint32_t SampleRate, ChannelMask, ChannelCount, FormatId, OutputAccess;
} NabuMiSoundAudioConfig;

enum NabuMiSoundAudioConfigResult {
    NabuMiSoundAudioConfigValid = 0,
    NabuMiSoundAudioConfigUnsupported = 1,
    NabuMiSoundAudioConfigInvalidArgument = 2
};

/* Captured ARM64 MiSound_setConfig contract, not a Windows WAVEFORMAT parser.
 * Accepts exactly the original 112-byte Android effect_config_t representation.
 * Rates, masks and format bytes must match; output access byte must be 0 or 2.
 * Valid input preserves all opaque bytes and derives the channel-mask popcount.
 * The original accepts zero/unknown rates/masks/formats when those checks match;
 * acceptance does not establish supported PCM24, DSP formats or a live route.
 * Opaque Android pointers are copied as data and must never be dereferenced on
 * Windows. No resource, context, audio stream, calibration or DSP is changed.
 * Failed input leaves output untouched. Inputs remain immutable while read;
 * output may overlap input because the entire result is constructed first.
 */
int NabuMiSoundParseAudioConfig(const void *config, size_t bytes,
    NabuMiSoundAudioConfig *output);

#ifdef __cplusplus
}
#endif
