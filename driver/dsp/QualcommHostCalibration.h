#pragma once
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct NabuQcomHostCalQuery {
    uint32_t Device,Rate,AppType,Module,Instance,Parameter;
} NabuQcomHostCalQuery;
enum NabuQcomHostCalPackResult {
    NabuQcomHostCalPacked=0,NabuQcomHostCalInvalid=1,NabuQcomHostCalShortBuffer=2
};
/* Pack the original ADCM 0xadae2105 common-calibration GET buffer.
 * The 36-byte header, payload capacity and three tail-padding bytes match the
 * original driver. All route/parameter IDs must be explicitly supplied from
 * captured configuration; zero IDs, instance truncation and overflow reject.
 * Invalid/short input leaves output and written untouched. The output's payload
 * space is zeroed, not initialized with calibration values. written must not
 * alias input/output. No device I/O or SET command is implemented here.
 * This queries the Windows host ACDB cache, NOT the live DSP parameter state.
 */
int NabuQcomPackHostCalGet(const NabuQcomHostCalQuery *query,size_t payloadCapacity,
    void *output,size_t capacity,size_t *written);
#ifdef __cplusplus
}
#endif
