#include "MiSoundResource.h"
#include <string.h>
static uint32_t word(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
        ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}
static int tag_valid(const uint8_t *p)
{
    for (unsigned i = 0; i < 4; ++i)
        if (!((p[i] >= 'A' && p[i] <= 'Z') || (p[i] >= '0' && p[i] <= '9')))
            return 0;
    return 1;
}
static int walk(const uint8_t *data, size_t bytes, NabuMiSoundParameterVisitor visitor,
    void *context, NabuMiSoundResourceInfo *result)
{
    size_t at, rootEnd, filterEnd;
    uint32_t filterBytes, profiles;
    NabuMiSoundResourceInfo info = {0};
    if (bytes < 64 || bytes > UINT32_MAX || memcmp(data, "SPKD", 4) ||
        word(data + 4) != bytes - 12 || memcmp(data + bytes - 8, "ENDSEND0", 8) ||
        memcmp(data + 8, "SPMD", 4))
        return NabuMiSoundResourceScanInvalid;
    filterBytes = word(data + 12);
    if (filterBytes < 32 || filterBytes > bytes - 20)
        return NabuMiSoundResourceScanInvalid;
    filterEnd = 16 + (size_t)filterBytes;
    if (filterEnd + 12 > bytes - 8 || memcmp(data + filterEnd, "ENDP", 4))
        return NabuMiSoundResourceScanInvalid;
    at = filterEnd + 4;
    if (memcmp(data + at, "NUMM", 4)) return NabuMiSoundResourceScanInvalid;
    profiles = word(data + at + 4); at += 8; rootEnd = bytes - 8;
    if (!profiles || profiles > (rootEnd - at) / 12)
        return NabuMiSoundResourceScanInvalid;
    /* Fixed bounds keep this initializer allocation-free and bounded. These are
     * implementation limits, not inferred factory calibration values.
     */
    if (profiles > 32) return NabuMiSoundResourceScanUnsupported;
    info.ProfileCount = profiles;
    info.FilterPayloadOffset = 16;
    info.FilterPayloadBytes = filterBytes;
    for (uint32_t p = 0; p < profiles; ++p) {
        uint32_t profileTag, payload, moduleIndex = 0;
        size_t end;
        if (rootEnd - at < 12 || !tag_valid(data + at))
            return NabuMiSoundResourceScanInvalid;
        profileTag = word(data + at); payload = word(data + at + 4); at += 8;
        if (payload < 4 || payload % 4 || payload > rootEnd - at)
            return NabuMiSoundResourceScanInvalid;
        end = at + payload;
        if (memcmp(data + end - 4, "ENDM", 4)) return NabuMiSoundResourceScanInvalid;
        while (at < end - 4) {
            uint32_t moduleTag, recordIndex = 0;
            if (end - 4 - at < 12 || memcmp(data + at, "MODU", 4) ||
                !tag_valid(data + at + 4)) return NabuMiSoundResourceScanInvalid;
            moduleTag = word(data + at + 4); at += 8;
            if (++info.ModuleCount > 64) return NabuMiSoundResourceScanUnsupported;
            for (;;) {
                NabuMiSoundParameterRecord record;
                if (end - 4 - at < 4) return NabuMiSoundResourceScanInvalid;
                if (!memcmp(data + at, "EMOD", 4)) { at += 4; break; }
                if (end - 4 - at < 16 || word(data + at) != moduleTag)
                    return NabuMiSoundResourceScanInvalid;
                record.ProfileIndex = p; record.ModuleIndex = moduleIndex;
                record.RecordIndex = recordIndex++; record.ProfileTag = profileTag;
                record.ModuleTag = moduleTag; record.SelectorA = word(data + at + 4);
                record.SelectorB = word(data + at + 8); record.ValueBits = word(data + at + 12);
                record.SourceOffset = (uint32_t)at;
                if ((record.ValueBits & 0x7f800000U) == 0x7f800000U)
                    return NabuMiSoundResourceScanInvalid;
                if (++info.ParameterCount > 4096) return NabuMiSoundResourceScanUnsupported;
                if (visitor && !visitor(&record, context)) return NabuMiSoundResourceScanStopped;
                at += 16;
            }
            ++moduleIndex;
        }
        if (at != end - 4) return NabuMiSoundResourceScanInvalid;
        at = end;
    }
    if (at != rootEnd) return NabuMiSoundResourceScanInvalid;
    if (result) *result = info;
    return NabuMiSoundResourceScanOk;
}
int NabuMiSoundScanResource(const void *resource, size_t bytes,
    NabuMiSoundParameterVisitor visitor, void *context, NabuMiSoundResourceInfo *output)
{
    NabuMiSoundResourceInfo validated;
    int status;
    if (!resource) return NabuMiSoundResourceScanArgument;
    status = walk((const uint8_t *)resource, bytes, NULL, NULL, &validated);
    if (status != NabuMiSoundResourceScanOk) return status;
    if (visitor) {
        status = walk((const uint8_t *)resource, bytes, visitor, context, &validated);
        if (status != NabuMiSoundResourceScanOk) return status;
    }
    if (output) *output = validated;
    return NabuMiSoundResourceScanOk;
}
