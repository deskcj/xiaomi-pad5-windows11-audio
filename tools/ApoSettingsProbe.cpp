// Read-only CAPX settings probe. Does not activate an APO or audio stream.
#include <windows.h>
#include <mmdeviceapi.h>
#include <stdio.h>
template<class T> struct Owner {
    T *p=nullptr;
    ~Owner(){if(p)p->Release();}
    T **out(){return &p;}
    T *operator->(){return p;}
};
constexpr GUID Context={0x9df8834a,0x209e,0x4cc1,{0xbc,0x73,0xbb,0x43,0x53,0xad,0xfa,0x9f}};
constexpr PROPERTYKEY Bank={{0x1a5bcc9e,0x61ee,0x41b7,{0x9d,0xc2,0x1b,0x58,0x0d,0xca,0xfa,0x1b}},1};
static void text(const WCHAR *value) {
    putchar('"');
    if(value)for(const WCHAR *p=value;*p;++p){
        if(*p=='"'||*p=='\\')putchar('\\');
        if(*p<32||*p>126)printf("\\u%04x",unsigned(*p));else putchar(int(*p));
    }
    putchar('"');
}
int wmain() {
    HRESULT hr=CoInitializeEx(nullptr,COINIT_MULTITHREADED);
    if(FAILED(hr))return 1;
    int result=0;
    {
        Owner<IMMDeviceEnumerator> enumerator;Owner<IMMDevice> device;
        Owner<IAudioSystemEffectsPropertyStore> settings;Owner<IPropertyStore> defaults;
        LPWSTR id=nullptr;PROPVARIANT context={},value={};
        hr=CoCreateInstance(__uuidof(MMDeviceEnumerator),nullptr,CLSCTX_ALL,
            __uuidof(IMMDeviceEnumerator),(void **)enumerator.out());
        if(SUCCEEDED(hr))hr=enumerator->GetDefaultAudioEndpoint(eRender,eConsole,device.out());
        if(SUCCEEDED(hr))hr=device->GetId(&id);
        context.vt=VT_CLSID;context.puuid=const_cast<GUID *>(&Context);
        HRESULT activate=FAILED(hr)?hr:device->Activate(__uuidof(IAudioSystemEffectsPropertyStore),
            CLSCTX_ALL,&context,(void **)settings.out());
        if(SUCCEEDED(activate)&&!settings.p)activate=E_NOINTERFACE;
        HRESULT open=FAILED(activate)?activate:settings->OpenDefaultPropertyStore(STGM_READ,defaults.out());
        if(SUCCEEDED(open)&&!defaults.p)open=E_NOINTERFACE;
        HRESULT read=FAILED(open)?open:defaults->GetValue(Bank,&value);
        constexpr WCHAR expected[]=L"Global\\NabuDolbyBank-9df8834a-209e-4cc1-bc73-bb4353adfa9f";
        bool matched=SUCCEEDED(read)&&value.vt==VT_LPWSTR&&value.pwszVal&&wcscmp(value.pwszVal,expected)==0;
        printf("{\"audio_started\":false,\"apo_activated\":false,\"property_written\":false,\"system_dolby_proven\":false,\"endpoint\":");text(id);
        printf(",\"activate_hr\":\"0x%08lx\",\"open_default_hr\":\"0x%08lx\",\"read_bank_hr\":\"0x%08lx\",\"bank_vt\":%u,\"expected_bank_matched\":%s,\"bank\":",
            ULONG(activate),ULONG(open),ULONG(read),unsigned(value.vt),matched?"true":"false");
        if(SUCCEEDED(read)&&value.vt==VT_LPWSTR)text(value.pwszVal);else printf("null");
        printf("}\n");
        PropVariantClear(&value);CoTaskMemFree(id);
        // An absent context is a measured negative result, not a crash.
        result=FAILED(hr)?1:0;
    }
    CoUninitialize();return result;
}
