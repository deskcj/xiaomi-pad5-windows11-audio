#pragma once
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct NabuQcomPpParameter {
    uint32_t Module, Instance, Parameter;
} NabuQcomPpParameter;
enum NabuQcomPpPackResult {
    NabuQcomPpPacked=0,
    NabuQcomPpInvalidArgument=1,
    NabuQcomPpInsufficientBuffer=2
};
/* Exact 16-byte little-endian param_hdr_v3 followed by unchanged payload bytes.
 * Instance comes from the captured calibration/control state, not the raw
 * second word of an ACDB custom-topology module record. No implicit truncation.
 * Invalid/short buffers leave output and written untouched. Payload/output may
 * overlap; written must not alias either. This packs bytes only: no command,
 * route selection, COPP lookup, audio I/O, DSP ACK or calibration approval.
 */
int NabuQcomPackPpParameter(const NabuQcomPpParameter *parameter,
    const void *payload, size_t bytes, void *output, size_t capacity, size_t *written);
#ifdef __cplusplus
}
#endif
