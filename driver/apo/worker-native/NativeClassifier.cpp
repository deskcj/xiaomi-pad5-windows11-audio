// Lab-only gateway for the two audited functions in the SHA-pinned Dolby image.
// No audio/device I/O, installation, gain, profile or amplifier operations.
#include <windows.h>
#include <intrin.h>
#include <arm64intr.h>
#include <stdint.h>
#include <math.h>
#include <unicorn/unicorn.h>
#include <unicorn/arm64.h>
#include "NativeClassifier.h"

static LONG owner;
extern "C" __declspec(dllexport) int64_t NativeClassifierLrint(float value)
{
    return llrintf(value);
}
extern "C" __declspec(dllexport) void NativeClassifierUnexpectedImport()
{
    static const char message[]="Unexpected native classifier import; stopped isolated test.\r\n";
    DWORD written;
    WriteFile(GetStdHandle(STD_ERROR_HANDLE),message,sizeof(message)-1,&written,nullptr);
    ExitProcess(91);
}
extern "C" __declspec(dllexport) DWORD NativeClassifierInvoke(
    UINT_PTR entry, const uint64_t *arguments, uint64_t fpcr)
{
    // This is not a general native ELF executor. The caller verifies the exact
    // image SHA, function bytes, fixed shared mappings and complete call graph.
    if(entry!=0x1013b888 || !arguments)return ERROR_INVALID_PARAMETER;
    LONG current=LONG(GetCurrentThreadId());
    LONG prior=InterlockedCompareExchange(&owner,current,0);
    if(prior && prior!=current)return ERROR_INVALID_THREAD_ID;
    auto function=reinterpret_cast<void (*)(void *,void *,void *)>(entry);
    uint64_t oldFpcr=_ReadStatusReg(ARM64_FPCR),oldFpsr=_ReadStatusReg(ARM64_FPSR);
    DWORD result=ERROR_SUCCESS;
    __try {
        _WriteStatusReg(ARM64_FPCR,fpcr);
        _WriteStatusReg(ARM64_FPSR,0);
        function(reinterpret_cast<void *>(arguments[0]),reinterpret_cast<void *>(arguments[1]),reinterpret_cast<void *>(arguments[2]));
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        result=GetExceptionCode();
    }
    _WriteStatusReg(ARM64_FPCR,oldFpcr);
    _WriteStatusReg(ARM64_FPSR,oldFpsr);
    return result;
}


struct NativeClassifierHook {
    decltype(&uc_reg_read) read;
    decltype(&uc_reg_write) write;
    decltype(&uc_emu_stop) stop;
    decltype(&uc_hook_del) remove;
    uc_hook handle;
    uc_engine *guest;
    NativeClassifierPatch patches[128];
    UINT count;
    uc_err error;
    DWORD nativeError;
    uint64_t calls,ticks,frequency,seen,skipped;
    DWORD thread;
};
static void classifier_hook(uc_engine *guest,uint64_t address,uint32_t size,void *pointer)
{
    auto *ctx=static_cast<NativeClassifierHook *>(pointer);
    if(address!=0x1013b888 || size!=4 || GetCurrentThreadId()!=ctx->thread){ctx->error=UC_ERR_ARG;ctx->stop(guest);return;}
    uint64_t arguments[3]={},lr=0,fpcr=0;
    const int registers[]={UC_ARM64_REG_X0,UC_ARM64_REG_X1,UC_ARM64_REG_X2};
    for(UINT i=0;i<3;i++)if((ctx->error=ctx->read(guest,registers[i],&arguments[i]))!=UC_ERR_OK){ctx->stop(guest);return;}
    if((ctx->error=ctx->read(guest,UC_ARM64_REG_X30,&lr))!=UC_ERR_OK ||
       (ctx->error=ctx->read(guest,UC_ARM64_REG_FPCR,&fpcr))!=UC_ERR_OK){ctx->stop(guest);return;}
    ctx->seen++;
    if(arguments[0]<0x40000000 || arguments[0]>0x41000000-0x78c ||
       arguments[1]<0x40000000 || arguments[1]>0x41000000-4){ctx->error=UC_ERR_ARG;ctx->stop(guest);return;}
    // These are the exact two early-return gates in the captured function.
    // Keep disabled/no-input calls out of native execution and GOT patching.
    if(!*reinterpret_cast<uint32_t *>(arguments[0]+0x788) || !*reinterpret_cast<uint32_t *>(arguments[1])){
        ctx->skipped++;ctx->error=ctx->write(guest,UC_ARM64_REG_PC,&lr);
        if(ctx->error!=UC_ERR_OK)ctx->stop(guest);return;
    }
    for(UINT i=0;i<ctx->count;i++)if(*reinterpret_cast<uint64_t *>(ctx->patches[i].address)!=ctx->patches[i].oldValue){ctx->error=UC_ERR_ARG;ctx->stop(guest);return;}
    LARGE_INTEGER began,ended;QueryPerformanceCounter(&began);
    for(UINT i=0;i<ctx->count;i++)*reinterpret_cast<uint64_t *>(ctx->patches[i].address)=ctx->patches[i].nativeValue;
    ctx->nativeError=NativeClassifierInvoke(UINT_PTR(address),arguments,fpcr);
    for(UINT i=0;i<ctx->count;i++)*reinterpret_cast<uint64_t *>(ctx->patches[i].address)=ctx->patches[i].oldValue;
    QueryPerformanceCounter(&ended);ctx->ticks+=uint64_t(ended.QuadPart-began.QuadPart);ctx->calls++;
    if(ctx->nativeError){ctx->error=UC_ERR_EXCEPTION;ctx->stop(guest);return;}
    ctx->error=ctx->write(guest,UC_ARM64_REG_PC,&lr);
    if(ctx->error!=UC_ERR_OK)ctx->stop(guest);
}
extern "C" __declspec(dllexport) SIZE_T NativeClassifierHookSize(){return sizeof(NativeClassifierHook);}
extern "C" __declspec(dllexport) uc_err NativeClassifierAttach(HMODULE unicorn,uc_engine *guest,void *pointer,
    const NativeClassifierPatch *patches,UINT count)
{
    if(!unicorn||!guest||!pointer||!patches||!count||count>128)return UC_ERR_ARG;
    auto *ctx=static_cast<NativeClassifierHook *>(pointer);*ctx={};
    auto add=reinterpret_cast<decltype(&uc_hook_add)>(GetProcAddress(unicorn,"uc_hook_add"));
    ctx->read=reinterpret_cast<decltype(ctx->read)>(GetProcAddress(unicorn,"uc_reg_read"));
    ctx->write=reinterpret_cast<decltype(ctx->write)>(GetProcAddress(unicorn,"uc_reg_write"));
    ctx->stop=reinterpret_cast<decltype(ctx->stop)>(GetProcAddress(unicorn,"uc_emu_stop"));
    ctx->remove=reinterpret_cast<decltype(ctx->remove)>(GetProcAddress(unicorn,"uc_hook_del"));
    if(!add||!ctx->read||!ctx->write||!ctx->stop||!ctx->remove)return UC_ERR_VERSION;
    for(UINT i=0;i<count;i++){
        MEMORY_BASIC_INFORMATION info={};
        if(patches[i].address<0x10000000||patches[i].address>0x101f5000-8 || patches[i].address%8 ||
            !VirtualQuery(reinterpret_cast<void *>(patches[i].address),&info,sizeof(info)) ||
            info.AllocationBase!=reinterpret_cast<void *>(0x10000000) || info.State!=MEM_COMMIT || info.Protect!=PAGE_READWRITE ||
            *reinterpret_cast<uint64_t *>(patches[i].address)!=patches[i].oldValue)return UC_ERR_ARG;
        if(patches[i].nativeValue!=reinterpret_cast<UINT_PTR>(&NativeClassifierLrint) &&
            patches[i].nativeValue!=reinterpret_cast<UINT_PTR>(&NativeClassifierUnexpectedImport))return UC_ERR_ARG;
        ctx->patches[i]=patches[i];
    }
    ctx->count=count;ctx->guest=guest;ctx->thread=GetCurrentThreadId();LARGE_INTEGER frequency;QueryPerformanceFrequency(&frequency);ctx->frequency=uint64_t(frequency.QuadPart);
    return add(guest,&ctx->handle,UC_HOOK_CODE,reinterpret_cast<void *>(classifier_hook),ctx,uint64_t(0x1013b888),uint64_t(0x1013b888));
}
extern "C" __declspec(dllexport) uc_err NativeClassifierDetach(void *pointer)
{
    auto *ctx=static_cast<NativeClassifierHook *>(pointer);
    if(!ctx||!ctx->handle)return UC_ERR_ARG;
    uc_err result=ctx->remove(ctx->guest,ctx->handle);ctx->handle=0;return result;
}
extern "C" __declspec(dllexport) uint64_t NativeClassifierMetric(void *pointer,UINT field)
{
    auto *ctx=static_cast<NativeClassifierHook *>(pointer);if(!ctx)return 0;
    switch(field){case 0:return ctx->calls;case 1:return ctx->ticks;case 2:return ctx->frequency;case 3:return ctx->error;case 4:return ctx->nativeError;case 5:return ctx->seen;case 6:return ctx->skipped;default:return 0;}
}
extern "C" __declspec(dllexport) void NativeClassifierClearMetrics(void *pointer)
{
    auto *ctx=static_cast<NativeClassifierHook *>(pointer);if(ctx){ctx->calls=ctx->ticks=ctx->seen=ctx->skipped=0;ctx->error=UC_ERR_OK;ctx->nativeError=0;}
}
