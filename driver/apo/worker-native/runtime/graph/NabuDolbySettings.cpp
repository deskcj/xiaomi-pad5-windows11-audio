// Persistent settings only; no audio stream, registry or service operations.
#include <windows.h>
#include <appmodel.h>
#include <mmdeviceapi.h>
#include <functiondiscoverykeys_devpkey.h>
#include <stdio.h>
#include "DolbyBank.h"
template<class T>struct Owner {T*p=nullptr;~Owner(){if(p)p->Release();}T**out(){return &p;}T*operator->(){return p;}};
void text(const WCHAR*s){putchar('"');if(s)for(;*s;++s){if(*s=='"'||*s=='\\')putchar('\\');if(*s<32||*s>126)printf("\\u%04x",unsigned(*s));else putchar(char(*s));}putchar('"');}
int wmain(int argc,WCHAR**argv){
    if(argc!=2)return 2;
    bool query=!wcscmp(argv[1],L"--query"),on=!wcscmp(argv[1],L"--on"),off=!wcscmp(argv[1],L"--off"),clear=!wcscmp(argv[1],L"--clear");
    if(!query&&!on&&!off&&!clear)return 2;
    HRESULT hr=CoInitializeEx(nullptr,COINIT_MULTITHREADED);if(FAILED(hr))return 3;
    bool written=false,matched=false;HRESULT commit=E_PENDING,userOpen=E_PENDING,readOld=E_PENDING;LPWSTR id=nullptr;
    PROPVARIANT old={},value={};
    {
        Owner<IMMDeviceEnumerator>enumerator;Owner<IMMDevice>device;Owner<IPropertyStore>endpoint,defaults,user;
        Owner<IAudioSystemEffectsPropertyStore>settings;
        hr=CoCreateInstance(__uuidof(MMDeviceEnumerator),nullptr,CLSCTX_ALL,IID_PPV_ARGS(enumerator.out()));
        if(SUCCEEDED(hr))hr=enumerator->GetDefaultAudioEndpoint(eRender,eConsole,device.out());
        if(SUCCEEDED(hr))hr=device->GetId(&id);
        if(SUCCEEDED(hr))hr=device->OpenPropertyStore(STGM_READ,endpoint.out());
        PROPVARIANT label={};if(SUCCEEDED(hr))hr=endpoint->GetValue(PKEY_Device_FriendlyName,&label);
        constexpr WCHAR prefix[]=L"Speakers (Mi Pad 5 ";
        if(SUCCEEDED(hr)&&(label.vt!=VT_LPWSTR||!label.pwszVal||wcsncmp(label.pwszVal,prefix,wcslen(prefix))))hr=E_INVALIDARG;
        PropVariantClear(&label);
        PROPVARIANT context={};context.vt=VT_CLSID;context.puuid=const_cast<GUID*>(&NabuDolbyApoId);
        if(SUCCEEDED(hr))hr=device->Activate(__uuidof(IAudioSystemEffectsPropertyStore),CLSCTX_ALL,&context,reinterpret_cast<void**>(settings.out()));
        if(SUCCEEDED(hr))hr=settings->OpenDefaultPropertyStore(STGM_READ,defaults.out());
        PROPVARIANT bank={};if(SUCCEEDED(hr))hr=defaults->GetValue(PKEY_NabuDolbyBank,&bank);
        constexpr WCHAR expected[]=L"Global\\NabuDolbyBank-9df8834a-209e-4cc1-bc73-bb4353adfa9f";
        if(SUCCEEDED(hr))matched=bank.vt==VT_LPWSTR&&bank.pwszVal&&!wcscmp(bank.pwszVal,expected);
        if(SUCCEEDED(hr)&&!matched)hr=E_INVALIDARG;PropVariantClear(&bank);
        if(SUCCEEDED(hr)){userOpen=settings->OpenUserPropertyStore(query?STGM_READ:STGM_READWRITE,user.out());hr=userOpen;}
        if(query&&hr==HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))hr=S_OK;
        if(SUCCEEDED(hr)&&user.p){readOld=user->GetValue(PKEY_NabuDolbyEnabled,&old);hr=readOld;if(hr==HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))hr=S_OK;}
        if(SUCCEEDED(hr)&&!query){
            PROPVARIANT setting={};if(!clear){setting.vt=VT_UI4;setting.ulVal=on?1:0;}
            hr=user->SetValue(PKEY_NabuDolbyEnabled,setting);written=SUCCEEDED(hr);
            if(SUCCEEDED(hr)){commit=user->Commit();hr=commit;}
        }
        if(SUCCEEDED(hr)&&user.p)hr=user->GetValue(PKEY_NabuDolbyEnabled,&value);
        if(hr==HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))hr=S_OK;
        if(SUCCEEDED(hr)&&!query){
            bool exact=clear?value.vt==VT_EMPTY:value.vt==VT_UI4&&value.ulVal==UINT(on?1:0);
            if(!exact)hr=E_FAIL;
        }
    }
    UINT32 identityLength=0;LONG identityStatus=GetCurrentPackageFullName(&identityLength,nullptr);
    bool packaged=identityStatus==ERROR_INSUFFICIENT_BUFFER||identityStatus==ERROR_SUCCESS;
    printf("{\"hr\":\"0x%08lx\",\"commit_hr\":\"0x%08lx\",\"open_user_hr\":\"0x%08lx\",\"read_old_hr\":\"0x%08lx\",\"packaged_identity\":%s,\"endpoint\":",ULONG(hr),ULONG(commit),ULONG(userOpen),ULONG(readOld),packaged?"true":"false");text(id);
    printf(",\"bank_matched\":%s,\"written\":%s,\"old_vt\":%u,\"old_value\":%lu,\"stored_vt\":%u,\"stored_value\":%lu,\"audio_opened\":false,\"volume_changed\":false,\"passed\":%s}\n",matched?"true":"false",written?"true":"false",unsigned(old.vt),old.vt==VT_UI4?old.ulVal:0,unsigned(value.vt),value.vt==VT_UI4?value.ulVal:0,SUCCEEDED(hr)?"true":"false");
    PropVariantClear(&old);PropVariantClear(&value);CoTaskMemFree(id);CoUninitialize();return SUCCEEDED(hr)?0:4;
}
