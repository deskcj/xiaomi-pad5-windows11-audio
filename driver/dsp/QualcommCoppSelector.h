#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
enum NabuQcomCoppTable {
    NabuQcomCoppSingle=0, NabuQcomCoppFlag18=1, NabuQcomCoppPairedFlag16=2
};
typedef struct NabuQcomCoppSelector {
    uint32_t Raw, Table, PrimaryIndex, SecondaryIndex, ByteOffset;
} NabuQcomCoppSelector;
/* Reproduce original QCADCM!0x8bec8 selection, without accessing its tables.
 * Requires exactly one bit in the low byte. Flag16 also requires exactly one
 * bit in the second byte; Flag18 has original precedence over Flag16.
 * Indices describe driver context tables, not speakers, ACDB IDs or live COPPs.
 * Invalid input leaves output untouched. Raw bits are retained, not invented.
 */
int NabuQcomDecodeCoppSelector(uint32_t selector,NabuQcomCoppSelector *output);
/* The original online ADM wrapper masks selector to 16 bits before lookup.
 * This stricter check rejects such truncation instead of silently selecting a
 * different context. It still does not prove the table slot is active/live.
 */
int NabuQcomCheckAdmSelector(uint32_t selector,NabuQcomCoppSelector *output);
#ifdef __cplusplus
}
#endif
