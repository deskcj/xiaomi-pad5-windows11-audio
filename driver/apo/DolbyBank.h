#pragma once
#include "DolbySharedTransport.h"
#include <propsys.h>

constexpr PROPERTYKEY PKEY_NabuDolbyBank = {{0x1a5bcc9e, 0x61ee, 0x41b7,
    {0x9d, 0xc2, 0x1b, 0x58, 0x0d, 0xca, 0xfa, 0x1b}}, 1};
// CAPX User store: absent means the original enabled default; UI4 is strictly 0/1.
constexpr PROPERTYKEY PKEY_NabuDolbyEnabled = {{0x1a5bcc9e, 0x61ee, 0x41b7,
    {0x9d, 0xc2, 0x1b, 0x58, 0x0d, 0xca, 0xfa, 0x1b}}, 2};
constexpr GUID NabuDolbyEffectId = {0x2330de07, 0x4ae3, 0x45e1, {0xb2, 0x4a, 0xb0, 0x68, 0xa8, 0x6f, 0x9b, 0x6a}};
constexpr UINT32 NabuBankSignature = 0x324b424e;
constexpr UINT32 NabuBankSlots = 4;
struct NabuBank {
    UINT32 signature;
    UINT32 version;
    UINT32 bytes;
    UINT32 count;
    WCHAR names[NabuBankSlots][NabuTransportNameChars];
};
NabuTransportView *nabu_bank_acquire(const WCHAR *name);
bool nabu_bank_ready(const WCHAR *name);
extern "C" __declspec(dllexport) void *WINAPI NabuBankCreate(const WCHAR *name,
    const WCHAR *flat_names, UINT32 count, const WCHAR *sddl);
extern "C" __declspec(dllexport) void WINAPI NabuBankClose(void *view);
