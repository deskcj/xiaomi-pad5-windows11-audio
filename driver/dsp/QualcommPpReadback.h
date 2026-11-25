#pragma once
#include "QualcommPpParameter.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct NabuQcomAdmGetBuffer {
    uint64_t Address;
    uint32_t Handle;
    uint32_t Bytes;
} NabuQcomAdmGetBuffer;

typedef struct NabuQcomPpReadback {
    const void *Payload;
    size_t Bytes;
} NabuQcomPpReadback;

enum NabuQcomPpReadResult {
    NabuQcomPpReadValid = 0,
    NabuQcomPpReadInvalidArgument = 1,
    NabuQcomPpReadShortBuffer = 2,
    NabuQcomPpReadWrongParameter = 3,
    NabuQcomPpReadReservedBits = 4,
    NabuQcomPpReadBadLength = 5
};

/* Original Windows QCADCM ADM_GET_PP_PARAMS_V6 out-of-band body (28 bytes).
 * The caller must obtain an owned, live DSP mapping and COPP separately. Bytes
 * is the OEM shared-response capacity INCLUDING the 16-byte parameter header;
 * it is not an ACDB payload size. Address/handle are never fabricated here.
 * This does not pack an APR envelope, create a mapping or dispatch a command.
 * Rejects zero IDs/handles/addresses, instance truncation and address overflow.
 * Invalid/short input leaves output and written untouched; written must not
 * alias the input structures or output buffer.
 */
int NabuQcomPackAdmGetBody(const NabuQcomPpParameter *parameter,
    const NabuQcomAdmGetBuffer *mapping, void *output, size_t capacity, size_t *written);

/* Check an OEM out-of-band param_hdr_v3 + payload after transport completion.
 * Requires matching module/instance/parameter, zero reserved halfword, a full
 * header and exact bounded returned size. Invalid input leaves view untouched.
 * A validated view only describes bytes: the caller MUST independently check
 * the APR transaction token, response opcode/status and mapping ownership.
 * It is not a DSP ACK, factory-value approval or proof of active processing.
 * Input remains immutable while the returned view is used; view must not alias
 * response/parameter. Transport capacity includes the 16-byte header.
 */
int NabuQcomValidatePpReadback(const NabuQcomPpParameter *parameter,
    size_t transportCapacity, const void *response, size_t bytes, NabuQcomPpReadback *view);

#ifdef __cplusplus
}
#endif
