#include "QualcommAcdbRoute.h"
#include <string.h>

typedef struct Span { const uint8_t *Data; size_t Bytes; } Span;
typedef struct Tables { Span Pool, Device, Global, Calibration, Definitions, Offsets; } Tables;

static uint32_t word(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
        ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}
static int take(Span source, size_t at, size_t bytes, Span *result)
{
    if(at > source.Bytes || bytes > source.Bytes-at) return 0;
    result->Data=source.Data+at; result->Bytes=bytes; return 1;
}
static int counted(Span source, size_t width, uint32_t *count)
{
    if(!source.Data || source.Bytes<4) return 0;
    *count=word(source.Data);
    return (source.Bytes-4)%width==0 && (source.Bytes-4)/width==*count;
}
static int pool_value(Span pool, uint32_t at, Span *value)
{
    Span length;
    if(!take(pool,at,4,&length)) return 0;
    return take(pool,(size_t)at+4,word(length.Data),value);
}
static int read_tables(const void *input, size_t bytes, Tables *tables)
{
    Span file={(const uint8_t *)input,bytes};
    size_t at=32;
    if(bytes<32 || memcmp(input,"QCMSNDDB",8)!=0) return NabuAcdbMalformed;
    if(bytes-32!=word(file.Data+24)) return NabuAcdbMalformed;
    if(word(file.Data+24)!=word(file.Data+28)) return NabuAcdbUnsupportedFormat;
    while(at<bytes) {
        Span header,body,*destination=NULL;
        if(!take(file,at,12,&header)) return NabuAcdbMalformed;
        at+=12;
        if(!take(file,at,word(header.Data+8),&body)) return NabuAcdbMalformed;
        at+=body.Bytes;
        if(memcmp(header.Data,"DATAPOOL",8)==0) destination=&tables->Pool;
        else if(memcmp(header.Data,"DPROPLUT",8)==0) destination=&tables->Device;
        else if(memcmp(header.Data,"GPROPLUT",8)==0) destination=&tables->Global;
        else if(memcmp(header.Data,"AGIILUT0",8)==0) destination=&tables->Calibration;
        else if(memcmp(header.Data,"AGIICDFT",8)==0) destination=&tables->Definitions;
        else if(memcmp(header.Data,"AGIICDOT",8)==0) destination=&tables->Offsets;
        if(destination) {
            if(destination->Data) return NabuAcdbMalformed;
            *destination=body;
        }
    }
    return tables->Pool.Data ? NabuAcdbFileRouteMatches : NabuAcdbMalformed;
}
int NabuAcdbGetGlobalPropertyView(const void *globalFile, size_t globalBytes,
    uint32_t property, NabuAcdbGlobalPropertyView *output)
{
    Tables tables={0};
    Span selected={0};
    uint32_t rows,matches=0;
    int status;
    uintptr_t inputAt=(uintptr_t)globalFile,outputAt=(uintptr_t)output;
    if(!globalFile || !output || !property || globalBytes>UINT32_MAX)
        return NabuAcdbInvalidArgument;
    if(outputAt>=inputAt ? outputAt-inputAt<globalBytes : inputAt-outputAt<sizeof(*output))
        return NabuAcdbInvalidArgument;
    status=read_tables(globalFile,globalBytes,&tables);
    if(status!=NabuAcdbFileRouteMatches) return status;
    /* Confirm the captured original loader's uncompressed global-file header.
     * Do not reinterpret device/calibration databases as the global resource.
     */
    if(word((const uint8_t *)globalFile+8) || word((const uint8_t *)globalFile+12) ||
        word((const uint8_t *)globalFile+16)!=0x42444347)
        return NabuAcdbUnsupportedFormat;
    if(!tables.Global.Data) return NabuAcdbGlobalPropertyMissing;
    if(!counted(tables.Global,8,&rows)) return NabuAcdbMalformed;
    for(uint32_t i=0;i<rows;++i) {
        const uint8_t *row=tables.Global.Data+4+(size_t)i*8;
        Span value;
        if(!pool_value(tables.Pool,word(row+4),&value)) return NabuAcdbMalformed;
        if(word(row)==property) {
            if(matches<2) ++matches;
            selected=value;
        }
    }
    if(!matches) return NabuAcdbGlobalPropertyMissing;
    if(matches>1) return NabuAcdbAmbiguousRoute;
    NabuAcdbGlobalPropertyView result={selected.Data,selected.Bytes,property,
        (uint32_t)(selected.Data-(const uint8_t *)globalFile)};
    *output=result;return NabuAcdbFileRouteMatches;
}
static int device_routes(const Tables *tables, const NabuAcdbRouteQuery *query,
    NabuAcdbRouteReport *report)
{
    uint32_t rows;
    if(!counted(tables->Device,12,&rows)) return 0;
    for(uint32_t i=0;i<rows;++i) {
        const uint8_t *row=tables->Device.Data+4+(size_t)i*12;
        Span value;
        if(!pool_value(tables->Pool,word(row+8),&value)) return 0;
        if(word(row+4)!=0x113ad) continue;
        if(value.Bytes%8!=0) return 0;
        if(word(row)!=query->Device) continue;
        for(size_t at=0;at<value.Bytes;at+=8) if(word(value.Data+at)==query->AppType) {
            /* Only uniqueness matters; saturate so duplicate input cannot wrap. */
            if(report->DeviceAppMatches<2) ++report->DeviceAppMatches;
            report->Topology=word(value.Data+at+4);
        }
    }
    return 1;
}
static int calibration_keys(const Tables *tables, const NabuAcdbRouteQuery *query,
    NabuAcdbRouteReport *report)
{
    uint32_t rows;
    if(!tables->Calibration.Data) return 1;
    if(!counted(tables->Calibration,20,&rows)) return 0;
    for(uint32_t i=0;i<rows;++i) {
        const uint8_t *row=tables->Calibration.Data+4+(size_t)i*20;
        Span definitions,offsets;
        uint32_t count,offsetCount;
        const uint32_t definitionAt=word(row+12),offsetAt=word(row+16);
        if(!take(tables->Definitions,definitionAt,4,&definitions) ||
            !take(tables->Offsets,offsetAt,4,&offsets)) return 0;
        count=word(definitions.Data);offsetCount=word(offsets.Data);
        if(count!=offsetCount || count>(tables->Definitions.Bytes-definitionAt-4)/12 ||
            count>(tables->Offsets.Bytes-offsetAt-4)/4) return 0;
        for(uint32_t j=0;j<count;++j) {
            Span payload;
            const uint8_t *position=tables->Offsets.Data+(size_t)offsetAt+4+(size_t)j*4;
            if(!pool_value(tables->Pool,word(position),&payload)) return 0;
        }
        if(word(row)==query->Device && word(row+4)==query->Rate && word(row+8)==query->AppType)
            if(report->CalibrationMatches<2) ++report->CalibrationMatches;
    }
    return 1;
}
static int modern_graphs(const Tables *tables, const NabuAcdbRouteQuery *query,
    NabuAcdbRouteReport *report)
{
    uint32_t rows,propertyMatches=0;
    if(!counted(tables->Global,8,&rows)) return NabuAcdbMalformed;
    for(uint32_t i=0;i<rows;++i) {
        const uint8_t *row=tables->Global.Data+4+(size_t)i*8;
        Span value;
        if(!pool_value(tables->Pool,word(row+4),&value)) return NabuAcdbMalformed;
        if(word(row)!=0x132ac) continue;
        ++propertyMatches;
        if(propertyMatches>1) return NabuAcdbMalformed;
        if(value.Bytes<4) return NabuAcdbMalformed;
        const uint32_t graphs=word(value.Data);
        size_t at=4;
        /* Version 2 has a 16-byte header and 8-byte module records. */
        if(graphs>(value.Bytes-4)/16) return NabuAcdbMalformed;
        for(uint32_t j=0;j<graphs;++j) {
            Span header,modules;
            if(!take(value,at,16,&header)) return NabuAcdbMalformed;
            const uint32_t version=word(header.Data),topology=word(header.Data+4);
            const uint32_t count=word(header.Data+12);
            if(version!=2) return NabuAcdbUnsupportedFormat;
            at+=16;
            if(count>(value.Bytes-at)/8 || !take(value,at,(size_t)count*8,&modules))
                return NabuAcdbMalformed;
            at+=modules.Bytes;
            if(report->DeviceAppMatches && topology==report->Topology) {
                if(report->TopologyMatches<2) ++report->TopologyMatches;
                report->GraphVersion=version;report->GraphConfig=word(header.Data+8);
                report->ModuleCount=count;
                for(uint32_t k=0;k<count;++k) {
                    const uint8_t *module=modules.Data+(size_t)k*8;
                    if(word(module)==query->Module && word(module+4)==query->ModuleQualifierWord)
                        if(report->ModuleMatches<2) ++report->ModuleMatches;
                }
            }
        }
        if(at!=value.Bytes) return NabuAcdbMalformed;
    }
    return NabuAcdbFileRouteMatches;
}
int NabuAcdbCheckFileRoute(const void *deviceFile, size_t deviceBytes,
    const void *globalFile, size_t globalBytes, const NabuAcdbRouteQuery *query,
    NabuAcdbRouteReport *output)
{
    Tables device={0},global={0};
    NabuAcdbRouteReport report={0};
    NabuAcdbRouteQuery requested;
    int status;
    if(!deviceFile || !globalFile || !query || !output) return NabuAcdbInvalidArgument;
    requested=*query;
    if(!requested.Rate) return NabuAcdbInvalidArgument;
    status=read_tables(deviceFile,deviceBytes,&device);
    if(status!=NabuAcdbFileRouteMatches) return status;
    status=read_tables(globalFile,globalBytes,&global);
    if(status!=NabuAcdbFileRouteMatches) return status;
    if(!device_routes(&device,&requested,&report) || !calibration_keys(&device,&requested,&report))
        return NabuAcdbMalformed;
    status=modern_graphs(&global,&requested,&report);
    if(status!=NabuAcdbFileRouteMatches) return status;
    if(!report.DeviceAppMatches) status=NabuAcdbDeviceAppMissing;
    else if(report.DeviceAppMatches>1 || report.CalibrationMatches>1 ||
        report.TopologyMatches>1 || report.ModuleMatches>1) status=NabuAcdbAmbiguousRoute;
    else if(report.Topology!=requested.Topology) status=NabuAcdbTopologyMismatch;
    else if(!report.TopologyMatches) status=NabuAcdbTopologyMissing;
    else if(!report.ModuleMatches) status=NabuAcdbModuleMissing;
    else if(!report.CalibrationMatches) status=NabuAcdbCalibrationMissing;
    *output=report;return status;
}
