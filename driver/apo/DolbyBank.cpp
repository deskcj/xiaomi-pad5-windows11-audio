#include "DolbyBank.h"
#include <sddl.h>
#include <strsafe.h>

namespace {
struct BankView { HANDLE mapping; NabuBank *bank; };
bool valid_name(const WCHAR *name)
{
    return name && (wcsncmp(name, L"Global\\NabuDolbyBank-", 21) == 0 ||
        wcsncmp(name, L"Local\\NabuDolbyBank-", 20) == 0) && wcsnlen(name, 128) < 128;
}
BankView *map(HANDLE mapping, DWORD access = FILE_MAP_WRITE)
{
    if (!mapping) return nullptr;
    auto *bank = static_cast<NabuBank *>(MapViewOfFile(mapping, access, 0, 0, sizeof(NabuBank)));
    if (!bank) { CloseHandle(mapping); return nullptr; }
    auto *view = static_cast<BankView *>(HeapAlloc(GetProcessHeap(), 0, sizeof(BankView)));
    if (!view) { UnmapViewOfFile(bank); CloseHandle(mapping); return nullptr; }
    *view = {mapping, bank}; return view;
}
}
extern "C" void *WINAPI NabuBankCreate(const WCHAR *name, const WCHAR *names, UINT32 count, const WCHAR *sddl)
{
    if (!valid_name(name) || !names || !count || count > NabuBankSlots) { SetLastError(ERROR_INVALID_PARAMETER); return nullptr; }
    PSECURITY_DESCRIPTOR security = nullptr;
    SECURITY_ATTRIBUTES attributes = {sizeof(attributes), nullptr, FALSE};
    if (sddl && !ConvertStringSecurityDescriptorToSecurityDescriptorW(sddl, SDDL_REVISION_1, &security, nullptr)) return nullptr;
    attributes.lpSecurityDescriptor = security;
    HANDLE handle = CreateFileMappingW(INVALID_HANDLE_VALUE, sddl ? &attributes : nullptr, PAGE_READWRITE, 0, sizeof(NabuBank), name);
    DWORD error = GetLastError();
    if (security) LocalFree(security);
    if (handle && error == ERROR_ALREADY_EXISTS) { CloseHandle(handle); SetLastError(error); return nullptr; }
    auto *view = map(handle);
    if (view) {
        auto *bank = view->bank;
        bank->signature = NabuBankSignature; bank->version = 2; bank->bytes = sizeof(*bank); bank->count = count;
        for (UINT32 i = 0; i < count; ++i) {
            HRESULT hr = StringCchCopyW(bank->names[i], NabuTransportNameChars, names + i*NabuTransportNameChars);
            if (FAILED(hr)) { NabuBankClose(view); SetLastError(ERROR_INVALID_NAME); return nullptr; }
        }
    }
    return view;
}
extern "C" void WINAPI NabuBankClose(void *pointer)
{
    auto *view = static_cast<BankView *>(pointer);
    if (!view) return;
    UnmapViewOfFile(view->bank); CloseHandle(view->mapping); HeapFree(GetProcessHeap(), 0, view);
}
NabuTransportView *nabu_bank_acquire(const WCHAR *name)
{
    if (!valid_name(name)) return nullptr;
    auto *view = map(OpenFileMappingW(FILE_MAP_WRITE, FALSE, name));
    if (!view) return nullptr;
    NabuTransportView *chosen = nullptr;
    auto *bank = view->bank;
    if (bank->signature == NabuBankSignature && bank->version == 2 && bank->bytes == sizeof(*bank) && bank->count <= NabuBankSlots) {
        for (UINT32 i = 0; i < bank->count; ++i) {
            if (bank->names[i][127]) continue;
            auto *candidate = nabu_open_transport(bank->names[i]);
            if (!candidate) continue;
            auto *s = candidate->shared;
            if (InterlockedCompareExchange(&s->worker_ready, 0, 0) && !InterlockedCompareExchange(&s->stop, 0, 0) &&
                !InterlockedCompareExchange(&s->apo_claim, 1, 0)) { chosen = candidate; break; }
            nabu_close_transport(candidate);
        }
    }
    NabuBankClose(view); return chosen;
}
bool nabu_bank_ready(const WCHAR *name)
{
    if (!valid_name(name)) return false;
    auto *view = map(OpenFileMappingW(FILE_MAP_READ, FALSE, name), FILE_MAP_READ);
    if (!view) return false;
    bool ready = false;
    auto *bank = view->bank;
    if (bank->signature == NabuBankSignature && bank->version == 2 &&
        bank->bytes == sizeof(*bank) && bank->count <= NabuBankSlots) {
        for (UINT32 i = 0; i < bank->count; ++i) {
            if (bank->names[i][127]) continue;
            auto *candidate = nabu_open_transport(bank->names[i]);
            if (!candidate) continue;
            ready = InterlockedCompareExchange(&candidate->shared->worker_ready, 0, 0) &&
                !InterlockedCompareExchange(&candidate->shared->stop, 0, 0);
            nabu_close_transport(candidate);
            if (ready) break;
        }
    }
    NabuBankClose(view);
    return ready;
}
