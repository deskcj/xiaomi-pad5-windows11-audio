#include "ApoDiagnostics.h"
#include <sddl.h>
#include <evntprov.h>
#include <strsafe.h>
namespace {
constexpr WCHAR Name[]=L"Global\\NabuApoDiag-9df8834a-209e-4cc1-bc73-bb4353adfa9f";
constexpr UINT32 Signature=0x3244414e;
constexpr GUID Provider={0x87ad024a,0x44cc,0x4b70,{0x8a,0x14,0xf9,0x67,0x50,0x16,0x45,0xb2}};
struct Data { UINT32 signature; volatile LONG fields[32]; };
struct View { HANDLE handle; Data *data; };
View *map(HANDLE handle) {
    if (!handle) return nullptr;
    auto *data=static_cast<Data *>(MapViewOfFile(handle,FILE_MAP_WRITE,0,0,sizeof(Data)));
    if (!data) { CloseHandle(handle); return nullptr; }
    auto *view=static_cast<View *>(HeapAlloc(GetProcessHeap(),0,sizeof(View)));
    if (!view) { UnmapViewOfFile(data); CloseHandle(handle); return nullptr; }
    *view={handle,data};return view;
}
}
void nabu_diag(UINT32 field,LONG value,bool increment) {
    // This diagnostics path is used only outside APOProcess.
    DWORD error=GetLastError();
    REGHANDLE event=0;
    if (EventRegister(&Provider,nullptr,nullptr,&event)==ERROR_SUCCESS) {
        WCHAR text[96];
        if (SUCCEEDED(StringCchPrintfW(text,ARRAYSIZE(text),L"NabuApo field=%u value=%ld pid=%lu",field,value,GetCurrentProcessId())))
            EventWriteString(event,4,0,text);
        EventUnregister(event);
    }
    auto *view=map(OpenFileMappingW(FILE_MAP_WRITE,FALSE,Name));
    if (view) {
        if (view->data->signature==Signature && field<32) {
            if (increment) InterlockedIncrement(&view->data->fields[field]);
            else InterlockedExchange(&view->data->fields[field],value);
        }
        NabuDiagnosticsClose(view);
    }
    SetLastError(error);
}
extern "C" void *WINAPI NabuDiagnosticsCreate(const WCHAR *sddl) {
    PSECURITY_DESCRIPTOR descriptor=nullptr;
    if (!ConvertStringSecurityDescriptorToSecurityDescriptorW(sddl,SDDL_REVISION_1,&descriptor,nullptr)) return nullptr;
    SECURITY_ATTRIBUTES attributes={sizeof(attributes),descriptor,FALSE};
    HANDLE handle=CreateFileMappingW(INVALID_HANDLE_VALUE,&attributes,PAGE_READWRITE,0,sizeof(Data),Name);
    DWORD error=GetLastError();LocalFree(descriptor);
    if (handle && error==ERROR_ALREADY_EXISTS) { CloseHandle(handle);SetLastError(error);return nullptr; }
    auto *view=map(handle);
    if (view) view->data->signature=Signature;
    return view;
}
extern "C" LONG WINAPI NabuDiagnosticsRead(void *pointer,UINT32 field) {
    auto *view=static_cast<View *>(pointer);
    return field<32 ? InterlockedCompareExchange(&view->data->fields[field],0,0) : -1;
}
extern "C" void WINAPI NabuDiagnosticsClose(void *pointer) {
    auto *view=static_cast<View *>(pointer);if (!view) return;
    UnmapViewOfFile(view->data);CloseHandle(view->handle);HeapFree(GetProcessHeap(),0,view);
}
