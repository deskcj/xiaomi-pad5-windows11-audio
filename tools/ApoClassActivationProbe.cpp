// Real in-process COM activation only. No Initialize, transport, audio or writes.
#include <windows.h>
#include <audioenginebaseapo.h>
#include <stdio.h>
constexpr GUID ApoId={0x9df8834a,0x209e,0x4cc1,{0xbc,0x73,0xbb,0x43,0x53,0xad,0xfa,0x9f}};
static void text(const WCHAR *value) {
    putchar('"');
    if(value)for(const WCHAR *p=value;*p;++p){
        if(*p=='"'||*p=='\\')putchar('\\');
        if(*p<32||*p>126)printf("\\u%04x",unsigned(*p));else putchar(int(*p));
    }
    putchar('"');
}
int wmain() {
    HRESULT init=CoInitializeEx(nullptr,COINIT_MULTITHREADED);
    IAudioProcessingObject *apo=nullptr;APO_REG_PROPERTIES *properties=nullptr;
    WCHAR modulePath[32768]={};HMODULE module=nullptr;DWORD moduleError=0;
    HRESULT activation=FAILED(init)?init:CoCreateInstance(ApoId,nullptr,CLSCTX_INPROC_SERVER,
        __uuidof(IAudioProcessingObject),(void **)&apo);
    if(SUCCEEDED(activation)&&!apo)activation=E_NOINTERFACE;
    HRESULT registration=FAILED(activation)?activation:apo->GetRegistrationProperties(&properties);
    if(SUCCEEDED(registration)&&!properties)registration=E_UNEXPECTED;
    if(apo){
        const auto *vtable=*reinterpret_cast<const void *const *const *>(apo);
        if(GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                reinterpret_cast<LPCWSTR>(vtable[0]),&module)){
            DWORD count=GetModuleFileNameW(module,modulePath,ARRAYSIZE(modulePath));
            if(!count||count>=ARRAYSIZE(modulePath))moduleError=GetLastError();
        }else moduleError=GetLastError();
    }
    printf("{\"audio_started\":false,\"apo_initialized\":false,\"registry_modified\":false,\"system_dolby_proven\":false,\"probe_pid\":%lu,\"activation_hr\":\"0x%08lx\",\"registration_hr\":\"0x%08lx\",\"module_error\":%lu,\"module_path\":",
        GetCurrentProcessId(),ULONG(activation),ULONG(registration),moduleError);
    text(modulePath);
    if(SUCCEEDED(registration))printf(",\"flags\":%u,\"interfaces\":%u,\"minimum_inputs\":%u,\"minimum_outputs\":%u",
        unsigned(properties->Flags),unsigned(properties->u32NumAPOInterfaces),
        unsigned(properties->u32MinInputConnections),unsigned(properties->u32MinOutputConnections));
    printf("}\n");
    CoTaskMemFree(properties);if(apo)apo->Release();if(SUCCEEDED(init))CoUninitialize();
    return FAILED(init)?1:0;
}
