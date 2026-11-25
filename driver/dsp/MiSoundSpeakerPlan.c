#include "MiSoundSpeakerPlan.h"
#include <string.h>
static uint32_t le32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
        ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}
static void encode_pair(const uint8_t *data, char *output)
{
    static const char alphabet[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    size_t at=0;
    for (size_t i=0; i<8; i+=3) {
        const uint8_t a=data[i], b=i+1<8?data[i+1]:0, c=i+2<8?data[i+2]:0;
        output[at++]=alphabet[a>>2];
        output[at++]=alphabet[((a&3)<<4)|(b>>4)];
        if(i+1<8)output[at++]=alphabet[((b&15)<<2)|(c>>6)];
        if(i+2<8)output[at++]=alphabet[c&63];
    }
    output[at]=0;
}
static int append(NabuMiSoundSpeakerPlan *plan, NabuMiSoundRoute *route,
    uint32_t type, uint32_t mask, const uint8_t *data, size_t bytes)
{
    NabuMiSoundSpeakerCommand *command;
    if(plan->CommandCount>=NABU_MISOUND_SPEAKER_PLAN_MAX_COMMANDS)
        return NabuMiSoundInvalidArgument;
    command=&plan->Commands[plan->CommandCount];
    if(bytes==4) {
        NabuMiSoundI32Command scalar;
        int status=NabuMiSoundEncodeI32(type,mask,route,le32(data),&scalar);
        if(status!=NabuMiSoundEncoded)return status;
        *route=scalar.Route;
        command->PayloadWords[0]=scalar.Value;
        memcpy(command->PayloadBase64,scalar.PayloadBase64,sizeof(scalar.PayloadBase64));
    } else {
        route->Parameter=0x1000d200U+type-11U;
        command->PayloadWords[0]=le32(data);command->PayloadWords[1]=le32(data+4);
        encode_pair(data,command->PayloadBase64);
    }
    command->Route=*route;command->PayloadBytes=(uint32_t)bytes;
    ++plan->CommandCount;
    return NabuMiSoundEncoded;
}
int NabuMiSoundBuildSpeakerPlan(uint32_t type, uint32_t mask,
    const NabuMiSoundRoute *route, const void *payload, size_t bytes,
    const uint32_t rates[NABU_MISOUND_PERSISTENT_RATE_COUNT], NabuMiSoundSpeakerPlan *output)
{
    NabuMiSoundSpeakerPlan plan={0};
    NabuMiSoundRoute current;
    const uint8_t *data=(const uint8_t *)payload;
    int status;
    if(!route || !data || !output || (bytes!=4 && bytes!=8))
        return NabuMiSoundInvalidArgument;
    current=*route;
    if(bytes==4) {
        NabuMiSoundI32Command checked;
        const uint32_t value=le32(data);
        const int skipActive=type==18 && value==5;
        const int skipPersistent=type==18 && (value==4 || value==60 || value==61);
        status=NabuMiSoundEncodeI32(type,mask,&current,value,&checked);
        if(status!=NabuMiSoundEncoded)return status;
        if(!rates)return NabuMiSoundInvalidArgument;
        for(unsigned i=0;i<NABU_MISOUND_PERSISTENT_RATE_COUNT;++i)
            if(!rates[i])return NabuMiSoundInvalidArgument;
        current.Persist=0;current.AppType=0x11130;
        if(!skipActive && (status=append(&plan,&current,type,mask,data,bytes))!=NabuMiSoundEncoded)return status;
        current.Persist=1;
        if(!skipPersistent)for(unsigned i=0;i<NABU_MISOUND_PERSISTENT_RATE_COUNT;++i) {
            current.Rate=rates[i];
            if((status=append(&plan,&current,type,mask,data,bytes))!=NabuMiSoundEncoded)return status;
        }
        current.AppType=0x11134;
        if(!skipActive) {
            current.Persist=0;
            if((status=append(&plan,&current,type,mask,data,bytes))!=NabuMiSoundEncoded)return status;
        }
        if(!skipPersistent) {
            current.Persist=1;
            for(unsigned i=0;i<NABU_MISOUND_PERSISTENT_RATE_COUNT;++i) {
                current.Rate=rates[i];
                if((status=append(&plan,&current,type,mask,data,bytes))!=NabuMiSoundEncoded)return status;
            }
        }
    } else {
        if(type<11 || type>14)return NabuMiSoundUnsupported;
        for(unsigned app=0;app<2;++app)for(unsigned persist=0;persist<2;++persist) {
            current.AppType=app?0x11134:0x11130;current.Persist=persist;
            if((status=append(&plan,&current,type,mask,data,bytes))!=NabuMiSoundEncoded)return status;
        }
    }
    plan.FinalRoute=current;*output=plan;
    return NabuMiSoundEncoded;
}
