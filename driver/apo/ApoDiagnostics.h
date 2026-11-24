#pragma once
#include <windows.h>
void nabu_diag(UINT32 field, LONG value, bool increment=false);
void nabu_diag_message(const WCHAR *message);
extern "C" __declspec(dllexport) void *WINAPI NabuDiagnosticsCreate(const WCHAR *sddl);
extern "C" __declspec(dllexport) LONG WINAPI NabuDiagnosticsRead(void *view, UINT32 field);
extern "C" __declspec(dllexport) void WINAPI NabuDiagnosticsClose(void *view);
