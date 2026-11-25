#pragma once

#include "NabuDolbyApo.h"
#include "DolbyBlockQueue.h"

// Standalone protocol only. No global mapping, service, or driver registration.
constexpr UINT32 NabuTransportSignature = 0x3254424e;
constexpr UINT32 NabuTransportVersion = 2;
constexpr UINT32 NabuTransportInitSignature = 0x3149424e;
constexpr UINT32 NabuTransportNameChars = 128;

struct NabuSharedTransport {
    UINT32 signature;
    UINT32 version;
    UINT32 bytes;
    UINT32 delay_frames;
    volatile LONG apo_claim;
    volatile LONG worker_claim; // Worker PID; -1 while supervisor reclaims an idle slot.
    volatile LONG generation;
    volatile LONG stop;
    volatile LONG requests_dropped;
    volatile LONG worker_gaps;
    volatile LONG blocks_replied;
    volatile LONG worker_ready;
    volatile LONG apo_pid;
    volatile LONG init_kind;
    volatile LONG lock_error;
    volatile LONG callbacks;
    volatile LONG64 frames_processed;
    volatile LONG64 frames_wet;
    volatile LONG64 frames_dry;
    DolbyBlockQueue requests;
    DolbyBlockQueue replies;
};

struct NabuTransportView {
    HANDLE mapping;
    NabuSharedTransport *shared;
    bool pinned;
    bool worker;
    WCHAR name[NabuTransportNameChars]; // Local view only; shared wire layout is unchanged.
};

struct NabuApoTransportInit {
    APOInitBaseStruct base;
    UINT32 signature;
    WCHAR name[NabuTransportNameChars];
};

static_assert(sizeof(StereoFrame) == 8);
static_assert(sizeof(DolbyAudioBlock) == 8208);
static_assert(sizeof(DolbyBlockQueue) == 262664);
static_assert(offsetof(NabuSharedTransport, requests) == 88);
static_assert(sizeof(NabuSharedTransport) == 525416);
static_assert(sizeof(NabuApoTransportInit) == 280);

NabuTransportView *nabu_open_transport(const WCHAR *name);
bool nabu_pin_transport(NabuTransportView *view);
void nabu_unpin_transport(NabuTransportView *view);
void nabu_close_transport(NabuTransportView *view);

extern "C" __declspec(dllexport) void *WINAPI NabuTransportCreate(const WCHAR *name, UINT32 delay);
extern "C" __declspec(dllexport) void *WINAPI NabuWorkerOpen(const WCHAR *name);
extern "C" __declspec(dllexport) void WINAPI NabuTransportClose(void *view);
extern "C" __declspec(dllexport) LONG WINAPI NabuTransportGeneration(void *view);
extern "C" __declspec(dllexport) void WINAPI NabuTransportStop(void *view);
extern "C" __declspec(dllexport) LONG WINAPI NabuTransportStopped(void *view);
extern "C" __declspec(dllexport) BOOL WINAPI NabuWorkerRead(void *view, DolbyAudioBlock *block);
extern "C" __declspec(dllexport) BOOL WINAPI NabuWorkerWrite(void *view, const DolbyAudioBlock *block);
extern "C" __declspec(dllexport) void WINAPI NabuWorkerReady(void *view, LONG ready);
extern "C" __declspec(dllexport) void WINAPI NabuWorkerGap(void *view);
// Non-RT supervisor only. A signaled owned process handle must match the
// registered worker PID. Active APO streams return ERROR_BUSY without touching
// queue cursors; worker readiness is withdrawn after confirmed process death.
extern "C" __declspec(dllexport) BOOL WINAPI NabuSupervisorReclaimExitedWorker(void *view, HANDLE process);
extern "C" __declspec(dllexport) LONG WINAPI NabuTransportMetric(void *view, UINT32 index);
extern "C" __declspec(dllexport) LONG64 WINAPI NabuTransportFrames(void *view, UINT32 index);
extern "C" __declspec(dllexport) void *WINAPI NabuTransportCreateSecured(const WCHAR *name, UINT32 delay, const WCHAR *sddl);
