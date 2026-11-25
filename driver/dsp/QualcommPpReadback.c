#include "QualcommPpReadback.h"

static void write_word(uint8_t *p, uint32_t value)
{
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8);
    p[2] = (uint8_t)(value >> 16);
    p[3] = (uint8_t)(value >> 24);
}

static uint32_t read_word(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
        ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static int valid_parameter(const NabuQcomPpParameter *p)
{
    return p->Module && p->Parameter && p->Instance <= UINT16_MAX;
}

int NabuQcomPackAdmGetBody(const NabuQcomPpParameter *parameter,
    const NabuQcomAdmGetBuffer *mapping, void *output, size_t capacity, size_t *written)
{
    NabuQcomPpParameter requested;
    NabuQcomAdmGetBuffer mapped;
    uint8_t *data = (uint8_t *)output;
    if (!parameter || !mapping || !output || !written)
        return NabuQcomPpReadInvalidArgument;
    requested = *parameter;
    mapped = *mapping;
    if (!valid_parameter(&requested) || !mapped.Address || !mapped.Handle ||
        mapped.Bytes < 16 || mapped.Address > UINT64_MAX - mapped.Bytes)
        return NabuQcomPpReadInvalidArgument;
    if (capacity < 28)
        return NabuQcomPpReadShortBuffer;
    write_word(data, (uint32_t)mapped.Address);
    write_word(data + 4, (uint32_t)(mapped.Address >> 32));
    write_word(data + 8, mapped.Handle);
    write_word(data + 12, requested.Module);
    write_word(data + 16, requested.Instance);
    write_word(data + 20, requested.Parameter);
    write_word(data + 24, mapped.Bytes);
    *written = 28;
    return NabuQcomPpReadValid;
}

int NabuQcomValidatePpReadback(const NabuQcomPpParameter *parameter,
    size_t transportCapacity, const void *response, size_t bytes, NabuQcomPpReadback *view)
{
    NabuQcomPpParameter requested;
    NabuQcomPpReadback result;
    const uint8_t *data = (const uint8_t *)response;
    uint32_t instance, payloadBytes;
    if (!parameter || !response || !view || transportCapacity > UINT32_MAX)
        return NabuQcomPpReadInvalidArgument;
    requested = *parameter;
    if (!valid_parameter(&requested))
        return NabuQcomPpReadInvalidArgument;
    if (bytes < 16 || transportCapacity < 16)
        return NabuQcomPpReadShortBuffer;
    if (bytes > transportCapacity)
        return NabuQcomPpReadBadLength;
    instance = read_word(data + 4);
    if (instance & 0xffff0000U)
        return NabuQcomPpReadReservedBits;
    if (read_word(data) != requested.Module || instance != requested.Instance ||
        read_word(data + 8) != requested.Parameter)
        return NabuQcomPpReadWrongParameter;
    payloadBytes = read_word(data + 12);
    if (payloadBytes != bytes - 16)
        return NabuQcomPpReadBadLength;
    result.Payload = data + 16;
    result.Bytes = payloadBytes;
    *view = result;
    return NabuQcomPpReadValid;
}

static uint16_t read_half(const uint8_t *p)
{
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

int NabuQcomValidateAdmGetReply(const NabuQcomAprReplyKey *key,
    const NabuQcomPpParameter *parameter, size_t transportCapacity,
    unsigned int mode, const void *packet, size_t packetBytes,
    const void *mappedResponse, size_t mappedBytes, NabuQcomPpReadback *view)
{
    NabuQcomAprReplyKey expected;
    NabuQcomPpReadback result;
    const uint8_t *data = (const uint8_t *)packet;
    uint32_t opcode;
    int status;
    if (!key || !parameter || !packet || !view || transportCapacity > UINT32_MAX ||
        mode > NabuQcomAdmGetOutOfBand ||
        (mode == NabuQcomAdmGetInBand && (mappedResponse || mappedBytes)))
        return NabuQcomAdmGetReplyInvalidArgument;
    expected = *key;
    if (!expected.Source || !expected.Destination)
        return NabuQcomAdmGetReplyInvalidArgument;
    if (packetBytes < 20)
        return NabuQcomAdmGetReplyShortBuffer;
    /* Original QCADCM callback copies into a 0x90-byte response allocation. */
    if (packetBytes > 144)
        return NabuQcomAdmGetReplyBadHeader;
    if (read_half(data) != 0x150 || read_half(data + 2) != packetBytes)
        return NabuQcomAdmGetReplyBadHeader;
    if (read_half(data + 4) != expected.Source || read_half(data + 6) != expected.SourcePort ||
        read_half(data + 8) != expected.Destination || read_half(data + 10) != expected.DestinationPort)
        return NabuQcomAdmGetReplyWrongRoute;
    if (read_word(data + 12) != expected.Token)
        return NabuQcomAdmGetReplyWrongToken;
    opcode = read_word(data + 16);
    if (opcode == 0x110e8) {
        if (packetBytes != 28)
            return NabuQcomAdmGetReplyBadHeader;
        if (read_word(data + 20) != 0x1035e)
            return NabuQcomAdmGetReplyWrongOpcode;
        return read_word(data + 24) ? NabuQcomAdmGetReplyDspError :
            NabuQcomAdmGetReplyNoParameterData;
    }
    if (opcode != 0x1035f)
        return NabuQcomAdmGetReplyWrongOpcode;
    if (packetBytes < 24)
        return NabuQcomAdmGetReplyShortBuffer;
    if (read_word(data + 20))
        return NabuQcomAdmGetReplyDspError;
    if (mode == NabuQcomAdmGetOutOfBand) {
        if (packetBytes != 24)
            return NabuQcomAdmGetReplyBadHeader;
        if (!mappedResponse)
            return NabuQcomAdmGetReplyInvalidArgument;
        status = NabuQcomValidatePpReadback(parameter, transportCapacity,
            mappedResponse, mappedBytes, &result);
    } else {
        status = NabuQcomValidatePpReadback(parameter, transportCapacity,
            data + 24, packetBytes - 24, &result);
    }
    if (status != NabuQcomPpReadValid)
        return NabuQcomAdmGetReplyInvalidReadback;
    *view = result;
    return NabuQcomAdmGetReplyValid;
}
