#pragma once
#include <stddef.h>
#include "MiSoundProtocol.h"
#ifdef __cplusplus
extern "C" {
#endif
#define NABU_MISOUND_SPEAKER_PLAN_MAX_COMMANDS 12
#define NABU_MISOUND_PERSISTENT_RATE_COUNT 5
typedef struct NabuMiSoundSpeakerCommand {
    NabuMiSoundRoute Route;
    uint32_t PayloadBytes;
    uint32_t PayloadWords[2];
    char PayloadBase64[12];
} NabuMiSoundSpeakerCommand;
typedef struct NabuMiSoundSpeakerPlan {
    uint32_t CommandCount;
    NabuMiSoundRoute FinalRoute;
    NabuMiSoundSpeakerCommand Commands[NABU_MISOUND_SPEAKER_PLAN_MAX_COMMANDS];
} NabuMiSoundSpeakerPlan;
/* Original MiSoundDSP_SetSPKParam command ordering, without DSP/audio I/O.
 * Payload is an exact 4- or 8-byte little-endian Android control value.
 * The five persistent rates must come from the target's captured DSPVarFS.
 * Eight-byte commands do not use that array. Payload bits are not interpreted
 * as calibration units. FinalRoute retains the original wrapper's state changes.
 * Unsupported/invalid input leaves output untouched; no commands are dispatched.
 */
int NabuMiSoundBuildSpeakerPlan(uint32_t parameterType, uint32_t androidDeviceMask,
    const NabuMiSoundRoute *route, const void *payload, size_t bytes,
    const uint32_t persistentRates[NABU_MISOUND_PERSISTENT_RATE_COUNT],
    NabuMiSoundSpeakerPlan *output);
#ifdef __cplusplus
}
#endif
