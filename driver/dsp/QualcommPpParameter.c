#include "QualcommPpParameter.h"
#include <string.h>
static void write_word(uint8_t *p,uint32_t value)
{
    p[0]=(uint8_t)value;p[1]=(uint8_t)(value>>8);
    p[2]=(uint8_t)(value>>16);p[3]=(uint8_t)(value>>24);
}
int NabuQcomPackPpParameter(const NabuQcomPpParameter *parameter,
    const void *payload,size_t bytes,void *output,size_t capacity,size_t *written)
{
    NabuQcomPpParameter requested;
    uint8_t *data=(uint8_t *)output;
    size_t total;
    if(!parameter || (!payload && bytes) || !output || !written || bytes>UINT32_MAX-16U)
        return NabuQcomPpInvalidArgument;
    requested=*parameter;
    if(requested.Instance>UINT16_MAX) return NabuQcomPpInvalidArgument;
    total=bytes+16;
    if(capacity<total) return NabuQcomPpInsufficientBuffer;
    if(bytes) memmove(data+16,payload,bytes);
    write_word(data,requested.Module);
    write_word(data+4,requested.Instance); /* high reserved half is zero */
    write_word(data+8,requested.Parameter);write_word(data+12,(uint32_t)bytes);
    *written=total;return NabuQcomPpPacked;
}
