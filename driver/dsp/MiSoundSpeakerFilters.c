#include "MiSoundSpeakerFilters.h"
#include "MiSoundResource.h"
#include <math.h>
#include <string.h>

_Static_assert(sizeof(float) == 4, "Factory coefficients require binary32");
static uint32_t le32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
        ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}
static int tagged_word(const uint8_t *data, size_t bytes, size_t at,
    const char tag[4], uint32_t *value)
{
    if (at > bytes || bytes - at < 8 || memcmp(data + at, tag, 4))
        return 0;
    *value = le32(data + at + 4);
    return 1;
}
static float binary32(const uint8_t *p)
{
    uint32_t bits = le32(p);
    float result;
    memcpy(&result, &bits, sizeof(result));
    return result;
}
int NabuMiSoundReadSpeakerFilters(const void *resource, size_t bytes,
    uint32_t sampleRate, NabuMiSoundSpeakerFilters *output)
{
    const uint8_t *data = (const uint8_t *)resource;
    uint32_t total, sectionBytes, modelBytes, maximum, model;
    /* Original HyperOS b0630cd4: cmp.eq 0x2b110, mux 0x2ee00.
     * This selects an existing factory table; it does not resample audio.
     */
    const uint32_t tableRate = sampleRate == 176400U ? 192000U : sampleRate;
    size_t end, at;
    int found = 0;
    NabuMiSoundSpeakerFilters selected = {0};
    if (!data || !output || !sampleRate)
        return NabuMiSoundFilterArgumentInvalid;
    if (NabuMiSoundScanResource(data, bytes, NULL, NULL, NULL) != NabuMiSoundResourceScanOk)
        return NabuMiSoundResourceInvalid;
    if (bytes < 52 || bytes - 12 > UINT32_MAX ||
        !tagged_word(data, bytes, 0, "SPKD", &total) || total != bytes - 12 ||
        memcmp(data + bytes - 8, "ENDSEND0", 8) ||
        !tagged_word(data, bytes, 8, "SPMD", &sectionBytes) ||
        !tagged_word(data, bytes, 16, "MMFD", &modelBytes) ||
        !tagged_word(data, bytes, 24, "MAXS", &maximum) ||
        !tagged_word(data, bytes, 32, "MODL", &model))
        return NabuMiSoundResourceInvalid;
    if (!maximum || maximum > NABU_MISOUND_MAX_SOS || model != 0)
        return NabuMiSoundResourceUnsupported;
    if (sectionBytes != modelBytes || sectionBytes > bytes - 20)
        return NabuMiSoundResourceInvalid;
    end = 16 + (size_t)sectionBytes;
    if (end < 48 || end + 4 > bytes ||
        memcmp(data + end - 8, "ENDMENDFENDP", 12))
        return NabuMiSoundResourceInvalid;
    at = 40;
    while (at < end - 8) {
        uint32_t rate, count;
        size_t coefficients, scale, next;
        NabuMiSoundSpeakerFilters table = {0};
        if (!tagged_word(data, end - 8, at, "SMPR", &rate) || !rate ||
            !tagged_word(data, end - 8, at + 8, "NSOS", &count) ||
            !count || count > maximum || end - 8 - at < 20 ||
            memcmp(data + at + 16, "COEF", 4))
            return NabuMiSoundResourceInvalid;
        coefficients = at + 20;
        if ((size_t)count * 24 + 4 + ((size_t)count + 1) * 4 > end - 8 - coefficients)
            return NabuMiSoundResourceInvalid;
        scale = coefficients + (size_t)count * 24;
        next = scale + 4 + ((size_t)count + 1) * 4;
        if (next > end - 8 || memcmp(data + scale, "SCAL", 4))
            return NabuMiSoundResourceInvalid;
        table.SampleRate = rate;
        table.SectionCount = count;
        for (uint32_t i = 0; i < count; ++i) {
            for (uint32_t j = 0; j < 6; ++j) {
                float value = binary32(data + coefficients + (size_t)i * 24 + j * 4);
                if (!isfinite(value)) return NabuMiSoundResourceInvalid;
                table.Coefficients[i][j] = value;
            }
            /* Jury conditions for the normalized real second-order denominator.
             * Reject unstable/corrupt tables before handing them to a processor.
             */
            const double a1 = table.Coefficients[i][4], a2 = table.Coefficients[i][5];
            if (table.Coefficients[i][3] != 1.0f || fabs(a2) >= 1.0 ||
                1.0 + a1 + a2 <= 0.0 || 1.0 - a1 + a2 <= 0.0)
                return NabuMiSoundResourceInvalid;
        }
        for (uint32_t i = 0; i <= count; ++i) {
            float value = binary32(data + scale + 4 + i * 4);
            if (!isfinite(value)) return NabuMiSoundResourceInvalid;
            table.Scales[i] = value;
        }
        if (rate == tableRate) {
            if (found) return NabuMiSoundResourceInvalid;
            selected = table;
            found = 1;
        }
        at = next;
    }
    if (at != end - 8) return NabuMiSoundResourceInvalid;
    if (!found) return NabuMiSoundRateAbsent;
    *output = selected;
    return NabuMiSoundFiltersRead;
}
