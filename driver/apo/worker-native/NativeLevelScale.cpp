// Execute only the captured 792-byte function at RVA 0x156370. Its complete
// call graph has no imports, indirect branches, x18, TLS or system calls.
#include <windows.h>
#include <intrin.h>
#include <arm64intr.h>
#include "NativeLevelScale.h"
#include <unicorn/arm64.h>

namespace {
constexpr UINT_PTR heapBegin = 0x40000000, heapEnd = 0x41000000;
constexpr UINT_PTR stackBegin = 0x30000000, stackEnd = 0x30081000;
constexpr UINT_PTR entryAddress = 0x10156370;
// This lab adapter supports at most eight original channel records.
bool data_span(uint64_t address, SIZE_T bytes, SIZE_T alignment, bool mappingsVerified)
{
    MEMORY_BASIC_INFORMATION info = {};
    UINT_PTR begin=address>=heapBegin && address<heapEnd ? heapBegin : stackBegin;
    UINT_PTR end=begin==heapBegin ? heapEnd : stackEnd;
    if (address < begin || address >= end || bytes > end-address || address%alignment) return false;
    if (mappingsVerified) return true;
    return VirtualQuery(reinterpret_cast<void *>(address), &info, sizeof(info)) &&
        info.AllocationBase == reinterpret_cast<void *>(begin) &&
        info.State == MEM_COMMIT && info.Protect == PAGE_READWRITE &&
        bytes <= info.RegionSize-(address-reinterpret_cast<UINT_PTR>(info.BaseAddress));
}
bool valid_arguments(const uint64_t *args, bool mappingsVerified)
{
    if (!args || args[4] >= 8 || !data_span(args[0], 17*sizeof(float), 4, mappingsVerified) ||
        !data_span(args[2], 16, 8, mappingsVerified)) return false;
    SIZE_T channels = SIZE_T(args[4])+1;
    if (!data_span(args[1], channels*16, 8, mappingsVerified) || !data_span(args[5], channels*96, 4, mappingsVerified)) return false;
    uint64_t second = *reinterpret_cast<const uint64_t *>(args[2]+8);
    if (!data_span(second, channels*16, 8, mappingsVerified)) return false;
    for (SIZE_T i=0; i<channels; ++i) {
        uint64_t firstVector = *reinterpret_cast<const uint64_t *>(args[1]+i*16);
        uint64_t secondVector = *reinterpret_cast<const uint64_t *>(second+i*16);
        if (!data_span(firstVector, 20*sizeof(float), 4, mappingsVerified) ||
            !data_span(secondVector, 20*sizeof(float), 4, mappingsVerified)) return false;
    }
    return true;
}
DWORD invoke(UINT_PTR entry, const uint64_t *args, const float *floats, uint64_t fpcr, bool mappingsVerified);
struct Hook {
    decltype(&uc_reg_read) read;
    decltype(&uc_reg_write) write;
    decltype(&uc_emu_stop) stop;
    decltype(&uc_hook_del) remove;
    uc_hook handle;
    uc_engine *guest;
    DWORD thread, nativeError;
    uc_err error;
    uint64_t calls, ticks, frequency;
};
void level_hook(uc_engine *guest, uint64_t address, uint32_t size, void *pointer)
{
    auto *ctx = static_cast<Hook *>(pointer);
    if (address != entryAddress || size != 4 || GetCurrentThreadId() != ctx->thread) {
        ctx->error = UC_ERR_ARG; ctx->stop(guest); return;
    }
    uint64_t args[6] = {}, lr=0, fpcr=0;
    float floats[2] = {};
    const int regs[] = {UC_ARM64_REG_X0,UC_ARM64_REG_X1,UC_ARM64_REG_X2,
        UC_ARM64_REG_X3,UC_ARM64_REG_X4,UC_ARM64_REG_X5};
    for (UINT i=0; i<6; ++i) {
        ctx->error=ctx->read(guest,regs[i],&args[i]);
        if (ctx->error != UC_ERR_OK) {ctx->stop(guest); return;}
    }
    if ((ctx->error=ctx->read(guest,UC_ARM64_REG_S0,&floats[0])) != UC_ERR_OK ||
        (ctx->error=ctx->read(guest,UC_ARM64_REG_S1,&floats[1])) != UC_ERR_OK ||
        (ctx->error=ctx->read(guest,UC_ARM64_REG_X30,&lr)) != UC_ERR_OK ||
        (ctx->error=ctx->read(guest,UC_ARM64_REG_FPCR,&fpcr)) != UC_ERR_OK) {
        ctx->stop(guest); return;
    }
    LARGE_INTEGER start, finish;
    QueryPerformanceCounter(&start);
    // Attach proved both full mappings. The owner keeps them committed/RW and
    // exclusive until Detach; every direct/nested pointer is still bounded.
    ctx->nativeError=invoke(entryAddress,args,floats,fpcr,true);
    QueryPerformanceCounter(&finish);
    ctx->ticks+=uint64_t(finish.QuadPart-start.QuadPart); ++ctx->calls;
    if (ctx->nativeError) {ctx->error=UC_ERR_EXCEPTION; ctx->stop(guest); return;}
    ctx->error=ctx->write(guest,UC_ARM64_REG_PC,&lr);
    if (ctx->error != UC_ERR_OK) ctx->stop(guest);
}
DWORD invoke(UINT_PTR entry, const uint64_t *args, const float *floats, uint64_t fpcr, bool mappingsVerified)
{
    if (entry != entryAddress || !floats || !valid_arguments(args,mappingsVerified)) return ERROR_INVALID_PARAMETER;
    // AAPCS64/Windows ARM64: floats in s0/s1, the remaining six arguments in
    // x0..x5. x3 is unused by the original function but retains its ABI slot.
    using Function = void (*)(float,float,void *,void *,void *,uint64_t,uint32_t,void *);
    auto function = reinterpret_cast<Function>(entry);
    uint64_t oldFpcr=_ReadStatusReg(ARM64_FPCR), oldFpsr=_ReadStatusReg(ARM64_FPSR);
    DWORD result=ERROR_SUCCESS;
    __try {
        _WriteStatusReg(ARM64_FPCR,fpcr); _WriteStatusReg(ARM64_FPSR,0);
        function(floats[0],floats[1],reinterpret_cast<void *>(args[0]),
            reinterpret_cast<void *>(args[1]),reinterpret_cast<void *>(args[2]),
            args[3],uint32_t(args[4]),reinterpret_cast<void *>(args[5]));
    } __except(EXCEPTION_EXECUTE_HANDLER) { result=GetExceptionCode(); }
    _WriteStatusReg(ARM64_FPCR,oldFpcr); _WriteStatusReg(ARM64_FPSR,oldFpsr);
    return result;
}
}
extern "C" DWORD NativeLevelScaleInvoke(UINT_PTR entry, const uint64_t *args,
    const float *floats, uint64_t fpcr)
{
    return invoke(entry,args,floats,fpcr,false);
}
extern "C" SIZE_T NativeLevelScaleHookSize() { return sizeof(Hook); }
extern "C" uc_err NativeLevelScaleAttach(HMODULE unicorn, uc_engine *guest, void *pointer)
{
    if (!unicorn || !guest || !pointer) return UC_ERR_ARG;
    auto *ctx=static_cast<Hook *>(pointer);
    if (ctx->handle) return UC_ERR_ARG;
    if (!data_span(heapBegin,heapEnd-heapBegin,8,false) ||
        !data_span(stackBegin,stackEnd-stackBegin,8,false)) return UC_ERR_ARG;
    *ctx={};
    auto add=reinterpret_cast<decltype(&uc_hook_add)>(GetProcAddress(unicorn,"uc_hook_add"));
    ctx->read=reinterpret_cast<decltype(ctx->read)>(GetProcAddress(unicorn,"uc_reg_read"));
    ctx->write=reinterpret_cast<decltype(ctx->write)>(GetProcAddress(unicorn,"uc_reg_write"));
    ctx->stop=reinterpret_cast<decltype(ctx->stop)>(GetProcAddress(unicorn,"uc_emu_stop"));
    ctx->remove=reinterpret_cast<decltype(ctx->remove)>(GetProcAddress(unicorn,"uc_hook_del"));
    if (!add || !ctx->read || !ctx->write || !ctx->stop || !ctx->remove) return UC_ERR_VERSION;
    ctx->guest=guest; ctx->thread=GetCurrentThreadId();
    LARGE_INTEGER frequency; QueryPerformanceFrequency(&frequency); ctx->frequency=uint64_t(frequency.QuadPart);
    return add(guest,&ctx->handle,UC_HOOK_CODE,reinterpret_cast<void *>(level_hook),ctx,entryAddress,entryAddress);
}
extern "C" uc_err NativeLevelScaleDetach(void *pointer)
{
    auto *ctx=static_cast<Hook *>(pointer);
    if (!ctx || !ctx->handle || GetCurrentThreadId() != ctx->thread) return UC_ERR_ARG;
    uc_err result=ctx->remove(ctx->guest,ctx->handle); if (result==UC_ERR_OK) ctx->handle=0; return result;
}
extern "C" uint64_t NativeLevelScaleMetric(void *pointer, UINT field)
{
    auto *ctx=static_cast<Hook *>(pointer); if (!ctx) return 0;
    switch (field) {case 0:return ctx->calls; case 1:return ctx->ticks; case 2:return ctx->frequency;
        case 3:return ctx->error; case 4:return ctx->nativeError; default:return 0;}
}
extern "C" void NativeLevelScaleClearMetrics(void *pointer)
{
    auto *ctx=static_cast<Hook *>(pointer); if (ctx) {ctx->calls=ctx->ticks=0; ctx->error=UC_ERR_OK; ctx->nativeError=0;}
}
