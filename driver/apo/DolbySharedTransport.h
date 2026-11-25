#pragma once

#include "NabuDolbyApo.h"
#include "DolbyBlockQueue.h"

// Standalone protocol only. No global mapping, service, or driver registration.
constexpr UINT32 NabuTransportSignature = 0x3454424e;
constexpr UINT32 NabuTransportVersion = 4;
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
    volatile LONG64 first_dry_frame; // -1 until a missing/invalid reply causes fallback.
    volatile LONG64 last_dry_frame;
    volatile LONG64 dry_runs;
    volatile LONG64 dry_revision;
    struct { volatile LONG64 first_frame, last_frame; } dry_ranges[DOLBY_DRY_HISTORY];
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
static_assert(sizeof(NabuSharedTransport) == 525704);
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
// Sole-consumer read for a restarted worker. Skip only records whose entire
// output interval has already passed; preserve both SPSC producer cursors.
extern "C" __declspec(dllexport) BOOL WINAPI NabuWorkerReadCurrent(void *view, DolbyAudioBlock *block, UINT32 *discarded);
// Startup only, sole request consumer. Discard the committed prefix without
// resetting the producer cursor; the next new record retains the full delay.
extern "C" __declspec(dllexport) BOOL WINAPI NabuWorkerDiscardPending(void *view, UINT32 *discarded);
extern "C" __declspec(dllexport) BOOL WINAPI NabuWorkerWrite(void *view, const DolbyAudioBlock *block);
extern "C" __declspec(dllexport) void WINAPI NabuWorkerReady(void *view, LONG ready);
extern "C" __declspec(dllexport) void WINAPI NabuWorkerGap(void *view);
// Non-RT supervisor only. A signaled owned process handle must match the
// registered worker PID. Active APO streams return ERROR_BUSY without touching
// queue cursors; worker readiness is withdrawn after confirmed process death.
extern "C" __declspec(dllexport) BOOL WINAPI NabuSupervisorReclaimExitedWorker(void *view, HANDLE process);
// Active-stream recovery changes ownership only. The signaled handle must match
// the dead worker. No APO claim, epoch, queue cursor or audio data is reset.
extern "C" __declspec(dllexport) BOOL WINAPI NabuSupervisorReleaseExitedWorker(void *view, HANDLE process);
extern "C" __declspec(dllexport) LONG WINAPI NabuTransportMetric(void *view, UINT32 index);
extern "C" __declspec(dllexport) LONG64 WINAPI NabuTransportFrames(void *view, UINT32 index);
extern "C" __declspec(dllexport) BOOL WINAPI NabuTransportDryRanges(void *view, DolbyDryRange *ranges, UINT32 capacity, UINT64 *runs);

// Single APO writer; readers retry in their non-RT thread. Never wait in audio.
inline void nabu_publish_dry_history(NabuSharedTransport *s, const DolbyFallback &f)
{
    InterlockedIncrement64(&s->dry_revision);
    InterlockedExchange64(&s->last_dry_frame, LONG64(f.last_dry_frame));
    InterlockedExchange64(&s->dry_runs, LONG64(f.dry_runs));
    for (unsigned i = 0; i < DOLBY_DRY_HISTORY; ++i) {
        InterlockedExchange64(&s->dry_ranges[i].first_frame, LONG64(f.dry_ranges[i].first_frame));
        InterlockedExchange64(&s->dry_ranges[i].last_frame, LONG64(f.dry_ranges[i].last_frame));
    }
    InterlockedIncrement64(&s->dry_revision);
}
extern "C" __declspec(dllexport) void *WINAPI NabuTransportCreateSecured(const WCHAR *name, UINT32 delay, const WCHAR *sddl);
