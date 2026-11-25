#include "MiSoundAudioConfig.h"

static uint32_t read_word(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
        ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

int NabuMiSoundParseAudioConfig(const void *config, size_t bytes,
    NabuMiSoundAudioConfig *output)
{
    const uint8_t *p = (const uint8_t *)config;
    NabuMiSoundAudioConfig result;
    uint32_t mask, count = 0;
    size_t i;
    if (!config || !output || bytes != sizeof(result.Raw))
        return NabuMiSoundAudioConfigInvalidArgument;
    if (read_word(p + 16) != read_word(p + 72) ||
        read_word(p + 20) != read_word(p + 76) || p[48] != p[104] ||
        (p[105] != 0 && p[105] != 2))
        return NabuMiSoundAudioConfigUnsupported;
    for (i = 0; i < sizeof(result.Raw); ++i)
        result.Raw[i] = p[i];
    result.SampleRate = read_word(result.Raw + 72);
    result.ChannelMask = read_word(result.Raw + 76);
    result.FormatId = result.Raw[104];
    result.OutputAccess = result.Raw[105];
    mask = result.ChannelMask;
    while (mask) {
        mask &= mask - 1;
        ++count;
    }
    result.ChannelCount = count;
    *output = result;
    return NabuMiSoundAudioConfigValid;
}
