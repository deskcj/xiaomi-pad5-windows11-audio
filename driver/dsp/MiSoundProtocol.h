#pragma once
#include <stdint.h>

/* Original Android MiSound control encoding, independent of audio I/O.
 * Reference libmisoundfx.so SHA256:
 * 6340604109aefb8f6e24e3e877dc19f8e118dc9eeebacf1e8301a81433061b2c
 * MiSoundDSP_SetSParam ELF 0xf774; Base64encode ELF 0xefcc.
 * This helper neither enables an effect nor applies amplifier settings.
 */
#ifdef __cplusplus
extern "C" {
#endif
#ifdef NABU_MISOUND_TEST_DLL
#define NABU_MISOUND_API __declspec(dllexport)
#else
#define NABU_MISOUND_API
#endif
typedef struct NabuMiSoundRoute {
    uint32_t Persist;
    uint32_t Device;
    uint32_t AppType;
    uint32_t CalType;
    uint32_t Rate;
    uint32_t Topology;
    uint32_t Module;
    uint32_t Parameter;
} NabuMiSoundRoute;

typedef struct NabuMiSoundI32Command {
    NabuMiSoundRoute Route;
    uint32_t Value;
    char PayloadBase64[8];
} NabuMiSoundI32Command;

enum NabuMiSoundEncodeResult {
    NabuMiSoundEncoded = 0,
    NabuMiSoundUnsupported = 1,
    NabuMiSoundInvalidArgument = 2
};

/* Route/value must come from the target device's original calibration/control
 * state. No defaults are supplied. Unsupported/invalid input leaves output intact.
 */
NABU_MISOUND_API int NabuMiSoundEncodeI32(
    uint32_t parameterType, uint32_t androidDeviceMask,
    const NabuMiSoundRoute *route, uint32_t value, NabuMiSoundI32Command *output);
#ifdef __cplusplus
}
#endif
