#include "QualcommHostCalibration.h"
#include <string.h>
static void write_word(uint8_t *p,uint32_t value)
{
    p[0]=(uint8_t)value;p[1]=(uint8_t)(value>>8);
    p[2]=(uint8_t)(value>>16);p[3]=(uint8_t)(value>>24);
}
int NabuQcomPackHostCalGet(const NabuQcomHostCalQuery *query,size_t payloadCapacity,
    void *output,size_t capacity,size_t *written)
{
    NabuQcomHostCalQuery requested;
    uint8_t *data=(uint8_t *)output;
    size_t total;
    if(!query || !output || !written || payloadCapacity>UINT32_MAX-39U)return NabuQcomHostCalInvalid;
    requested=*query;
    if(!requested.Device || !requested.Rate || !requested.AppType || !requested.Module ||
        !requested.Parameter || requested.Instance>UINT16_MAX)return NabuQcomHostCalInvalid;
    total=payloadCapacity+39;
    if(capacity<total)return NabuQcomHostCalShortBuffer;
    write_word(data,2);write_word(data+4,requested.Module);write_word(data+8,requested.Parameter);
    write_word(data+12,requested.Instance);write_word(data+16,requested.AppType);
    write_word(data+20,requested.Device);write_word(data+24,requested.Rate);
    write_word(data+28,0);write_word(data+32,(uint32_t)payloadCapacity);
    memset(data+36,0,payloadCapacity+3);*written=total;return NabuQcomHostCalPacked;
}
