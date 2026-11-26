// Fake CAPX stores/devices + owned local mappings; opens no hardware endpoint.
#include <windows.h>
#include <audioengineextensionapo.h>
#include <mmdeviceapi.h>
#include <strsafe.h>
#include <stdio.h>
#include "NabuDolbyApo.h"
#include "DolbyBank.h"
struct Ref {LONG refs=1;ULONG add(){return ULONG(++refs);}ULONG release(){return ULONG(--refs);}};
struct Store final: IPropertyStore,Ref {
    bool defaults=false;VARTYPE kind=VT_EMPTY;ULONG enabled=0;WCHAR bank[128]={};HRESULT read=S_OK;
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID id,void**p)override{if(!p)return E_POINTER;*p=nullptr;if(id!=__uuidof(IUnknown)&&id!=__uuidof(IPropertyStore))return E_NOINTERFACE;*p=static_cast<IPropertyStore*>(this);AddRef();return S_OK;}
    ULONG STDMETHODCALLTYPE AddRef()override{return add();}ULONG STDMETHODCALLTYPE Release()override{return release();}
    HRESULT STDMETHODCALLTYPE GetCount(DWORD*n)override{if(!n)return E_POINTER;*n=0;return S_OK;}
    HRESULT STDMETHODCALLTYPE GetAt(DWORD,PROPERTYKEY*)override{return E_INVALIDARG;}
    HRESULT STDMETHODCALLTYPE GetValue(REFPROPERTYKEY key,PROPVARIANT*v)override{
        if(!v)return E_POINTER;*v={};if(FAILED(read))return read;
        if(defaults&&key.fmtid==PKEY_NabuDolbyBank.fmtid&&key.pid==PKEY_NabuDolbyBank.pid){v->vt=VT_LPWSTR;size_t bytes=(wcslen(bank)+1)*sizeof(WCHAR);v->pwszVal=static_cast<LPWSTR>(CoTaskMemAlloc(bytes));if(!v->pwszVal)return E_OUTOFMEMORY;memcpy(v->pwszVal,bank,bytes);}
        else if(!defaults&&key.fmtid==PKEY_NabuDolbyEnabled.fmtid&&key.pid==PKEY_NabuDolbyEnabled.pid){v->vt=kind;if(kind==VT_UI4)v->ulVal=enabled;else if(kind==VT_BOOL)v->boolVal=enabled?VARIANT_TRUE:VARIANT_FALSE;}
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE SetValue(REFPROPERTYKEY,REFPROPVARIANT)override{return E_NOTIMPL;}
    HRESULT STDMETHODCALLTYPE Commit()override{return E_NOTIMPL;}
};
struct Settings final:IAudioSystemEffectsPropertyStore,Ref {
    Store*defaults,*user;
    Settings(Store*d,Store*u):defaults(d),user(u){}
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID id,void**p)override{if(!p)return E_POINTER;*p=nullptr;if(id!=__uuidof(IUnknown)&&id!=__uuidof(IAudioSystemEffectsPropertyStore))return E_NOINTERFACE;*p=static_cast<IAudioSystemEffectsPropertyStore*>(this);AddRef();return S_OK;}
    ULONG STDMETHODCALLTYPE AddRef()override{return add();}ULONG STDMETHODCALLTYPE Release()override{return release();}
    HRESULT STDMETHODCALLTYPE OpenDefaultPropertyStore(DWORD,IPropertyStore**p)override{*p=defaults;defaults->AddRef();return S_OK;}
    HRESULT STDMETHODCALLTYPE OpenUserPropertyStore(DWORD,IPropertyStore**p)override{*p=user;user->AddRef();return S_OK;}
    HRESULT STDMETHODCALLTYPE OpenVolatilePropertyStore(DWORD,IPropertyStore**)override{return E_NOTIMPL;}
    HRESULT STDMETHODCALLTYPE ResetUserPropertyStore()override{return E_NOTIMPL;}
    HRESULT STDMETHODCALLTYPE ResetVolatilePropertyStore()override{return E_NOTIMPL;}
    HRESULT STDMETHODCALLTYPE RegisterPropertyChangeNotification(IAudioSystemEffectsPropertyChangeNotificationClient*)override{return E_NOTIMPL;}
    HRESULT STDMETHODCALLTYPE UnregisterPropertyChangeNotification(IAudioSystemEffectsPropertyChangeNotificationClient*)override{return E_NOTIMPL;}
};
struct Device final:IMMDevice,Ref {
    Settings*settings;const WCHAR*id;
    Device(Settings*s,const WCHAR*n):settings(s),id(n){}
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid,void**p)override{if(!p)return E_POINTER;*p=nullptr;if(iid!=__uuidof(IUnknown)&&iid!=__uuidof(IMMDevice))return E_NOINTERFACE;*p=static_cast<IMMDevice*>(this);AddRef();return S_OK;}
    ULONG STDMETHODCALLTYPE AddRef()override{return add();}ULONG STDMETHODCALLTYPE Release()override{return release();}
    HRESULT STDMETHODCALLTYPE Activate(REFIID iid,DWORD,PROPVARIANT*,void**p)override{*p=nullptr;if(iid!=__uuidof(IAudioSystemEffectsPropertyStore))return E_NOINTERFACE;*p=settings;settings->AddRef();return S_OK;}
    HRESULT STDMETHODCALLTYPE OpenPropertyStore(DWORD,IPropertyStore**)override{return E_NOTIMPL;}
    HRESULT STDMETHODCALLTYPE GetId(LPWSTR*p)override{size_t bytes=(wcslen(id)+1)*sizeof(WCHAR);*p=static_cast<LPWSTR>(CoTaskMemAlloc(bytes));if(!*p)return E_OUTOFMEMORY;memcpy(*p,id,bytes);return S_OK;}
    HRESULT STDMETHODCALLTYPE GetState(DWORD*p)override{*p=DEVICE_STATE_ACTIVE;return S_OK;}
};
struct Collection final:IMMDeviceCollection,Ref {
    Device*device;explicit Collection(Device*d):device(d){}
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID id,void**p)override{if(!p)return E_POINTER;*p=nullptr;if(id!=__uuidof(IUnknown)&&id!=__uuidof(IMMDeviceCollection))return E_NOINTERFACE;*p=static_cast<IMMDeviceCollection*>(this);AddRef();return S_OK;}
    ULONG STDMETHODCALLTYPE AddRef()override{return add();}ULONG STDMETHODCALLTYPE Release()override{return release();}
    HRESULT STDMETHODCALLTYPE GetCount(UINT*p)override{*p=1;return S_OK;}
    HRESULT STDMETHODCALLTYPE Item(UINT n,IMMDevice**p)override{if(n)return E_INVALIDARG;*p=device;device->AddRef();return S_OK;}
};
unsigned cases=0,failures=0;
void check(const char*name,bool ok){printf("%s{\"name\":\"%s\",\"passed\":%s}",cases++?",":"",name,ok?"true":"false");if(!ok)++failures;}
int wmain(int argc,WCHAR**argv){
    if(argc!=2||FAILED(CoInitializeEx(nullptr,COINIT_MULTITHREADED)))return 2;
    HMODULE dll=LoadLibraryExW(argv[1],nullptr,LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|LOAD_LIBRARY_SEARCH_SYSTEM32);if(!dll)return 3;
    auto get=reinterpret_cast<HRESULT(WINAPI*)(REFCLSID,REFIID,void**)>(GetProcAddress(dll,"DllGetClassObject"));
    auto create=reinterpret_cast<void*(WINAPI*)(const WCHAR*,UINT32)>(GetProcAddress(dll,"NabuTransportCreate"));
    auto close=reinterpret_cast<void(WINAPI*)(void*)>(GetProcAddress(dll,"NabuTransportClose"));
    auto ready=reinterpret_cast<void(WINAPI*)(void*,LONG)>(GetProcAddress(dll,"NabuWorkerReady"));
    auto worker_open=reinterpret_cast<void*(WINAPI*)(const WCHAR*)>(GetProcAddress(dll,"NabuWorkerOpen"));
    auto bank_create=reinterpret_cast<void*(WINAPI*)(const WCHAR*,const WCHAR*,UINT32,const WCHAR*)>(GetProcAddress(dll,"NabuBankCreate"));
    auto bank_close=reinterpret_cast<void(WINAPI*)(void*)>(GetProcAddress(dll,"NabuBankClose"));
    auto unload=reinterpret_cast<HRESULT(WINAPI*)()>(GetProcAddress(dll,"DllCanUnloadNow"));
    if(!get||!create||!close||!ready||!worker_open||!bank_create||!bank_close||!unload)return 4;
    WCHAR queue[128],bankname[128];swprintf_s(queue,L"Local\\NabuDolbyLab-preference-%lu-%llu",GetCurrentProcessId(),GetTickCount64());swprintf_s(bankname,L"Local\\NabuDolbyBank-preference-%lu-%llu",GetCurrentProcessId(),GetTickCount64());
    void*transport=create(queue,6144);if(!transport)return 5;
    void*worker=worker_open(queue);if(!worker)return 5;ready(worker,1);
    void*bank=bank_create(bankname,queue,1,nullptr);if(!bank)return 6;
    Store defaults,user;defaults.defaults=true;wcscpy_s(defaults.bank,bankname);Settings settings(&defaults,&user);Device device(&settings,L"test-speaker"),foreign(&settings,L"other-speaker");Collection collection(&device);
    IClassFactory*factory=nullptr;if(FAILED(get(NabuDolbyApoId,IID_PPV_ARGS(&factory))))return 7;
    printf("{\"audio_opened\":false,\"registry_changed\":false,\"checks\":[");
    for(unsigned scenario=0;scenario<7;++scenario){
        user.kind=VARTYPE(scenario==0?VT_EMPTY:scenario==4?VT_BOOL:VT_UI4);user.enabled=scenario==1?0:scenario==3?2:1;user.read=scenario==5?E_ACCESSDENIED:scenario==6?HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND):S_OK;
        IAudioProcessingObject*apo=nullptr;IAudioSystemEffects3*fx=nullptr;IAudioProcessingObjectNotifications*notify=nullptr;
        if(FAILED(factory->CreateInstance(nullptr,IID_PPV_ARGS(&apo)))||FAILED(apo->QueryInterface(IID_PPV_ARGS(&fx)))||FAILED(apo->QueryInterface(IID_PPV_ARGS(&notify))))return 8;
        APOInitSystemEffects3 init={};init.APOInit={sizeof(init),NabuDolbyApoId};init.pDeviceCollection=&collection;
        HRESULT hr=apo->Initialize(sizeof(init),reinterpret_cast<BYTE*>(&init));check("initialize",hr==S_OK);
        auto state=[&](){AUDIO_SYSTEMEFFECT*list=nullptr;UINT count=0;HRESULT h=fx->GetControllableSystemEffectsList(&list,&count,nullptr);int value=h==S_OK&&count==1&&list?int(list->state):-1;CoTaskMemFree(list);return value;};
        check("strict_initial_preference",state()==(scenario==0||scenario==2||scenario==6?1:0));
        APO_NOTIFICATION_DESCRIPTOR*list=nullptr;DWORD count=0;
        hr=notify->GetApoNotificationRegistrationInfo(&list,&count);check("notification_context",hr==S_OK&&count==1&&list&&list->type==APO_NOTIFICATION_TYPE_SYSTEM_EFFECTS_PROPERTY_CHANGE&&list->audioSystemEffectsPropertyChange.propertyStoreContext==NabuDolbyApoId&&list->audioSystemEffectsPropertyChange.device==&device);
        if(list){list->audioSystemEffectsPropertyChange.device->Release();CoTaskMemFree(list);}
        if(scenario==1){
            APO_NOTIFICATION n={};n.type=APO_NOTIFICATION_TYPE_SYSTEM_EFFECTS_PROPERTY_CHANGE;
            auto &c=n.audioSystemEffectsPropertyChange;c.endpoint=&device;c.propertyStoreContext=NabuDolbyApoId;c.propertyStoreType=AUDIO_SYSTEMEFFECTS_PROPERTYSTORE_TYPE_USER;c.propertyStore=&user;c.propertyKey=PKEY_NabuDolbyEnabled;
            user.enabled=1;notify->HandleNotification(&n);check("user_on_live",state()==1);
            user.enabled=0;notify->HandleNotification(&n);check("user_off_live",state()==0);
            user.enabled=1;c.endpoint=&foreign;notify->HandleNotification(&n);check("foreign_endpoint_ignored",state()==0);c.endpoint=&device;
            c.propertyStoreContext=GUID_NULL;notify->HandleNotification(&n);check("foreign_context_ignored",state()==0);c.propertyStoreContext=NabuDolbyApoId;
            c.propertyStoreType=AUDIO_SYSTEMEFFECTS_PROPERTYSTORE_TYPE_DEFAULT;notify->HandleNotification(&n);check("default_store_ignored",state()==0);c.propertyStoreType=AUDIO_SYSTEMEFFECTS_PROPERTYSTORE_TYPE_USER;
            c.propertyKey=PKEY_NabuDolbyBank;notify->HandleNotification(&n);check("other_property_ignored",state()==0);c.propertyKey=PKEY_NabuDolbyEnabled;
            user.enabled=2;notify->HandleNotification(&n);check("invalid_notification_retains_state",state()==0);
            user.kind=VT_EMPTY;notify->HandleNotification(&n);check("removed_preference_restores_default",state()==1);
            notify->HandleNotification(nullptr);check("null_notification_ignored",state()==1);
            check("stream_control_does_not_write_preference",fx->SetAudioSystemEffectState(NabuDolbyEffectId,AUDIO_SYSTEMEFFECT_STATE_OFF)==S_OK&&state()==0&&user.kind==VT_EMPTY);
        }
        notify->Release();fx->Release();apo->Release();check("settings_device_store_refs_released",user.refs==1&&defaults.refs==1&&device.refs==1&&settings.refs==1);
    }
    factory->Release();bank_close(bank);close(worker);close(transport);check("dll_can_unload",unload()==S_OK);
    printf("],\"cases\":%u,\"failures\":%u,\"passed\":%s}\n",cases,failures,failures?"false":"true");FreeLibrary(dll);CoUninitialize();return failures?1:0;
}
