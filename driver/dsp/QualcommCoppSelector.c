#include "QualcommCoppSelector.h"
static int single_bit(uint32_t bits)
{
    return bits && !(bits&(bits-1));
}
static uint32_t first_bit(uint32_t bits)
{
    uint32_t index=0;
    while(!(bits&1U)){bits>>=1;++index;}
    return index;
}
int NabuQcomDecodeCoppSelector(uint32_t selector,NabuQcomCoppSelector *output)
{
    NabuQcomCoppSelector value={0};
    uint32_t primary=selector&255U,secondary=(selector>>8)&255U;
    if(!output || !single_bit(primary))return 0;
    value.Raw=selector;value.PrimaryIndex=first_bit(primary);value.SecondaryIndex=UINT32_MAX;
    if(selector&(1U<<18)) {
        value.Table=NabuQcomCoppFlag18;value.ByteOffset=value.PrimaryIndex*0x220U;
    } else if(selector&(1U<<16)) {
        if(!single_bit(secondary))return 0;
        value.Table=NabuQcomCoppPairedFlag16;value.SecondaryIndex=first_bit(secondary);
        value.ByteOffset=(value.PrimaryIndex+value.SecondaryIndex*8U)*0x228U;
    } else {
        value.Table=NabuQcomCoppSingle;value.ByteOffset=value.PrimaryIndex*0x258U;
    }
    *output=value;return 1;
}
int NabuQcomCheckAdmSelector(uint32_t selector,NabuQcomCoppSelector *output)
{
    if(selector&0xffff0000U)return 0;
    return NabuQcomDecodeCoppSelector(selector,output);
}
