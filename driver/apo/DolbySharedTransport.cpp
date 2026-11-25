#include "DolbySharedTransport.h"
#include <sddl.h>
#include <strsafe.h>

namespace {
bool valid_name(const WCHAR *name)
{
    return name && (wcsncmp(name, L"Local\\NabuDolbyLab-", 19) == 0 || wcsncmp(name, L"Global\\NabuDolbyLab-", 20) == 0) &&
        wcsnlen(name, NabuTransportNameChars) < NabuTransportNameChars;
}

NabuTransportView *map_handle(HANDLE handle)
{
    if (!handle)
        return nullptr;
    auto *shared = static_cast<NabuSharedTransport *>(MapViewOfFile(handle, FILE_MAP_WRITE,
        0, 0, sizeof(NabuSharedTransport)));
    if (!shared) {
        CloseHandle(handle);
        return nullptr;
    }
    auto *view = static_cast<NabuTransportView *>(HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(NabuTransportView)));
    if (!view) {
        UnmapViewOfFile(shared);
        CloseHandle(handle);
        SetLastError(ERROR_OUTOFMEMORY);
        return nullptr;
    }
    view->mapping = handle;
    view->shared = shared;
    return view;
}
}

NabuTransportView *nabu_open_transport(const WCHAR *name)
{
    if (!valid_name(name)) {
        SetLastError(ERROR_INVALID_NAME);
        return nullptr;
    }
    auto *view = map_handle(OpenFileMappingW(FILE_MAP_WRITE, FALSE, name));
    if (!view)
        return nullptr;
    const auto *s = view->shared;
    if (FAILED(StringCchCopyW(view->name,NabuTransportNameChars,name))) {
        nabu_close_transport(view);SetLastError(ERROR_INVALID_NAME);return nullptr;
    }
    if (s->signature != NabuTransportSignature || s->version != NabuTransportVersion ||
        s->bytes != sizeof(*s) || !s->delay_frames || s->delay_frames > 48000) {
        nabu_close_transport(view);
        SetLastError(ERROR_INVALID_DATA);
        return nullptr;
    }
    return view;
}

bool nabu_pin_transport(NabuTransportView *view)
{
    if (!view->pinned)
        view->pinned = VirtualLock(view->shared, sizeof(*view->shared)) != FALSE;
    return view->pinned;
}

void nabu_unpin_transport(NabuTransportView *view)
{
    if (view && view->pinned) {
        VirtualUnlock(view->shared, sizeof(*view->shared));
        view->pinned = false;
    }
}

void nabu_close_transport(NabuTransportView *view)
{
    if (!view)
        return;
    if (view->worker && DWORD(InterlockedCompareExchange(&view->shared->worker_claim, 0, 0)) == GetCurrentProcessId()) {
        InterlockedExchange(&view->shared->worker_ready, 0);
        InterlockedCompareExchange(&view->shared->worker_claim, 0, LONG(GetCurrentProcessId()));
    }
    nabu_unpin_transport(view);
    UnmapViewOfFile(view->shared);
    CloseHandle(view->mapping);
    HeapFree(GetProcessHeap(), 0, view);
}

extern "C" void *WINAPI NabuTransportCreate(const WCHAR *name, UINT32 delay)
{
    return NabuTransportCreateSecured(name, delay, nullptr);
}

extern "C" void *WINAPI NabuTransportCreateSecured(const WCHAR *name, UINT32 delay, const WCHAR *sddl)
{
    if (!valid_name(name) || !delay || delay > 48000) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return nullptr;
    }
    PSECURITY_DESCRIPTOR security = nullptr;
    SECURITY_ATTRIBUTES attributes = {sizeof(attributes), nullptr, FALSE};
    if (sddl && !ConvertStringSecurityDescriptorToSecurityDescriptorW(sddl, SDDL_REVISION_1, &security, nullptr)) return nullptr;
    attributes.lpSecurityDescriptor = security;
    HANDLE mapping = CreateFileMappingW(INVALID_HANDLE_VALUE, sddl ? &attributes : nullptr, PAGE_READWRITE, 0,
        sizeof(NabuSharedTransport), name);
    DWORD error = GetLastError();
    if (security) LocalFree(security);
    if (mapping && error == ERROR_ALREADY_EXISTS) {
        CloseHandle(mapping);
        SetLastError(error);
        return nullptr;
    }
    auto *view = map_handle(mapping);
    if (view) {
        if (FAILED(StringCchCopyW(view->name,NabuTransportNameChars,name))) {
            nabu_close_transport(view);SetLastError(ERROR_INVALID_NAME);return nullptr;
        }
        auto *s = view->shared;
        s->signature = NabuTransportSignature;
        s->version = NabuTransportVersion;
        s->bytes = sizeof(*s);
        s->delay_frames = delay;
    }
    return view;
}

extern "C" void *WINAPI NabuWorkerOpen(const WCHAR *name)
{
    auto *view = nabu_open_transport(name);
    if (view && InterlockedCompareExchange(&view->shared->stop, 0, 0)) {
        nabu_close_transport(view);SetLastError(ERROR_OPERATION_ABORTED);return nullptr;
    }
    if (view && InterlockedCompareExchange(&view->shared->worker_claim, LONG(GetCurrentProcessId()), 0)) {
        nabu_close_transport(view);
        SetLastError(ERROR_BUSY);
        return nullptr;
    }
    if (view)
        view->worker = true;
    return view;
}

extern "C" void WINAPI NabuTransportClose(void *view) { nabu_close_transport(static_cast<NabuTransportView *>(view)); }
extern "C" LONG WINAPI NabuTransportGeneration(void *view)
{
    return InterlockedCompareExchange(&static_cast<NabuTransportView *>(view)->shared->generation, 0, 0);
}
extern "C" void WINAPI NabuTransportStop(void *view) { InterlockedExchange(&static_cast<NabuTransportView *>(view)->shared->stop, 1); }
extern "C" LONG WINAPI NabuTransportStopped(void *view)
{
    return InterlockedCompareExchange(&static_cast<NabuTransportView *>(view)->shared->stop, 0, 0);
}
extern "C" BOOL WINAPI NabuWorkerRead(void *view, DolbyAudioBlock *block)
{
    return dolby_queue_read(&static_cast<NabuTransportView *>(view)->shared->requests, block);
}
extern "C" BOOL WINAPI NabuWorkerWrite(void *view, const DolbyAudioBlock *block)
{
    auto *s = static_cast<NabuTransportView *>(view)->shared;
    BOOL ok = dolby_queue_write(&s->replies, block->generation, block->first_frame, block->frames, block->count);
    if (ok)
        InterlockedIncrement(&s->blocks_replied);
    return ok;
}
extern "C" void WINAPI NabuWorkerReady(void *view, LONG ready)
{
    auto *s = static_cast<NabuTransportView *>(view)->shared;
    if (DWORD(InterlockedCompareExchange(&s->worker_claim, 0, 0)) == GetCurrentProcessId())
        InterlockedExchange(&s->worker_ready, ready);
}
extern "C" void WINAPI NabuWorkerGap(void *view)
{
    InterlockedIncrement(&static_cast<NabuTransportView *>(view)->shared->worker_gaps);
}
extern "C" BOOL WINAPI NabuSupervisorReclaimExitedWorker(void *pointer, HANDLE process)
{
    auto *view = static_cast<NabuTransportView *>(pointer);
    if (!view || view->worker || !process) { SetLastError(ERROR_INVALID_PARAMETER);return FALSE; }
    const DWORD pid = GetProcessId(process);
    if (!pid) return FALSE;
    const DWORD waited = WaitForSingleObject(process, 0);
    if (waited != WAIT_OBJECT_0) {
        if (waited == WAIT_TIMEOUT) SetLastError(ERROR_BUSY);
        return FALSE;
    }
    auto *s = view->shared;
    const LONG owner = InterlockedCompareExchange(&s->worker_claim, 0, 0);
    if (owner && DWORD(owner) != pid) { SetLastError(ERROR_INVALID_OWNER);return FALSE; }
    if (owner) InterlockedExchange(&s->worker_ready, 0);
    if (InterlockedCompareExchange(&s->stop, 0, 0)) { SetLastError(ERROR_OPERATION_ABORTED);return FALSE; }
    // Exclude new APO acquisitions while confirming quiescence. Never reset
    // SPSC cursors while a producer or the old worker can still access them.
    if (InterlockedCompareExchange(&s->apo_claim, -1, 0)) { SetLastError(ERROR_BUSY);return FALSE; }
    BOOL ok = FALSE;
    DWORD error = ERROR_INVALID_OWNER;
    if (InterlockedCompareExchange(&s->worker_claim, -1, owner) == owner) {
        InterlockedExchange(&s->worker_ready, 0);
        InterlockedExchange(&s->requests.read_index, 0);
        InterlockedExchange(&s->requests.write_index, 0);
        InterlockedExchange(&s->replies.read_index, 0);
        InterlockedExchange(&s->replies.write_index, 0);
        InterlockedIncrement(&s->generation);
        InterlockedExchange(&s->worker_claim, 0);
        ok = TRUE;
    }
    InterlockedExchange(&s->apo_claim, 0);
    if (!ok) SetLastError(error);
    return ok;
}
extern "C" LONG WINAPI NabuTransportMetric(void *view, UINT32 index)
{
    auto *s = static_cast<NabuTransportView *>(view)->shared;
    volatile LONG *fields[] = {&s->apo_claim, &s->worker_claim, &s->generation, &s->stop,
        &s->requests_dropped, &s->worker_gaps, &s->blocks_replied, &s->worker_ready,
        &s->apo_pid, &s->init_kind, &s->lock_error, &s->callbacks};
    return index < ARRAYSIZE(fields) ? InterlockedCompareExchange(fields[index], 0, 0) : -1;
}

extern "C" LONG64 WINAPI NabuTransportFrames(void *view, UINT32 index)
{
    auto *s = static_cast<NabuTransportView *>(view)->shared;
    volatile LONG64 *fields[] = {&s->frames_processed, &s->frames_wet, &s->frames_dry};
    return index < ARRAYSIZE(fields) ? InterlockedCompareExchange64(fields[index], 0, 0) : -1;
}
