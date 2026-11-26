#pragma once
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef struct NabuAcdbRouteQuery {
    uint32_t Device, AppType, Rate, Topology, Module;
    /* Exact second word of a captured version-2 module record. Not a
     * parameter-header instance ID, channel number or speaker position. */
    uint32_t ModuleQualifierWord;
} NabuAcdbRouteQuery;

typedef struct NabuAcdbRouteReport {
    uint32_t DeviceAppMatches, Topology, CalibrationMatches, TopologyMatches;
    uint32_t GraphVersion, GraphConfig, ModuleCount, ModuleMatches;
} NabuAcdbRouteReport;

enum NabuAcdbRouteResult {
    NabuAcdbFileRouteMatches = 0,
    NabuAcdbDeviceAppMissing = 1,
    NabuAcdbTopologyMismatch = 2,
    NabuAcdbTopologyMissing = 3,
    NabuAcdbModuleMissing = 4,
    NabuAcdbCalibrationMissing = 5,
    NabuAcdbAmbiguousRoute = 6,
    NabuAcdbMalformed = 7,
    NabuAcdbInvalidArgument = 8,
    NabuAcdbUnsupportedFormat = 9,
    NabuAcdbGlobalPropertyMissing = 10
};

typedef struct NabuAcdbGlobalPropertyView {
    const void *Data;
    size_t Bytes;
    uint32_t Property, FileOffset;
} NabuAcdbGlobalPropertyView;

/* Resolve one opaque GPROPLUT/DATAPOOL property from an unchanged uncompressed
 * global ACDB file. All table rows and their pool bounds are validated before
 * returning a view. Ambiguous, missing or invalid input leaves view untouched.
 * Caller owns the input lifetime; view must not overlap that input. This only
 * prepares bytes for inspection: it does not select a DSP graph, authenticate
 * firmware resources, register a topology or approve calibration values.
 */
int NabuAcdbGetGlobalPropertyView(const void *globalFile, size_t globalBytes,
    uint32_t property, NabuAcdbGlobalPropertyView *view);

/* Inspect unchanged, uncompressed QCMSNDDB buffers. Resolve DPROPLUT 0x113ad,
 * AGIILUT0 calibration keys and GPROPLUT 0x132ac version-2 module records.
 * All referenced calibration definitions, offsets and payload spans are checked.
 * Invalid/malformed/unsupported input leaves report untouched. Other results
 * provide file observations, including why a requested route cannot match.
 * Matching files do NOT prove live DSP state, resource loading, COPP identity,
 * parameter ACK, calibration safety or permission to dispatch a command.
 * No device/registry/audio access, allocation, fallback or route remapping.
 */
int NabuAcdbCheckFileRoute(const void *deviceFile, size_t deviceBytes,
    const void *globalFile, size_t globalBytes, const NabuAcdbRouteQuery *query,
    NabuAcdbRouteReport *report);
#ifdef __cplusplus
}
#endif
