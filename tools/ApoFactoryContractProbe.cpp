// Explicit DLL factory tests only: no registration, Initialize or audio.
#include <windows.h>
#include <audioenginebaseapo.h>
#include <audioengineextensionapo.h>
#include <stdio.h>
#include <initializer_list>
constexpr GUID ApoId={0x9df8834a,0x209e,0x4cc1,{0xbc,0x73,0xbb,0x43,0x53,0xad,0xfa,0x9f}};
constexpr GUID UnknownId={0x49fd4b39,0x6253,0x42bd,{0xa7,0x41,0x39,0x74,0x33,0x72,0x29,0x1a}};
using GetClass=HRESULT(WINAPI *)(REFCLSID,REFIID,void **);
using CanUnload=HRESULT(WINAPI *)();
static unsigned count=0,failures=0;
class Outer final : public IUnknown {
public:
    IUnknown *inner=nullptr;
    ULONG refs=1;
    unsigned adds=0,releases=0,queries=0;
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid,void **result) override {
        ++queries;if(!result)return E_POINTER;*result=nullptr;
        if(iid==__uuidof(IUnknown)||iid==UnknownId){*result=this;AddRef();return S_OK;}
        return inner?inner->QueryInterface(iid,result):E_NOINTERFACE;
    }
    ULONG STDMETHODCALLTYPE AddRef() override {++adds;return ++refs;}
    ULONG STDMETHODCALLTYPE Release() override {++releases;return --refs;}
};
static void check(const char *name,HRESULT actual,HRESULT expected,bool pointer_ok=true){
    if(count++)putchar(',');const bool pass=actual==expected&&pointer_ok;
    printf("{\"name\":\"%s\",\"hr\":\"0x%08lx\",\"expected\":\"0x%08lx\",\"pointer_ok\":%s,\"passed\":%s}",
        name,ULONG(actual),ULONG(expected),pointer_ok?"true":"false",pass?"true":"false");
    if(!pass)++failures;
}
int wmain(int argc,WCHAR **argv){
    if(argc!=2)return 2;
    HMODULE dll=LoadLibraryExW(argv[1],nullptr,LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|LOAD_LIBRARY_SEARCH_SYSTEM32);
    if(!dll){printf("{\"load_error\":%lu,\"audio_started\":false}\n",GetLastError());return 1;}
    auto get=reinterpret_cast<GetClass>(GetProcAddress(dll,"DllGetClassObject"));
    auto unload=reinterpret_cast<CanUnload>(GetProcAddress(dll,"DllCanUnloadNow"));
    if(!get||!unload){FreeLibrary(dll);return 1;}
    printf("{\"audio_started\":false,\"apo_initialized\":false,\"registry_modified\":false,\"cases\":[");
    check("initial_unload",unload(),S_OK);
    void *p=reinterpret_cast<void *>(1);
    HRESULT hr=get(UnknownId,__uuidof(IClassFactory),&p);check("unknown_class",hr,CLASS_E_CLASSNOTAVAILABLE,p==nullptr);
    p=reinterpret_cast<void *>(1);hr=get(ApoId,UnknownId,&p);check("unknown_factory_iid",hr,E_NOINTERFACE,p==nullptr);
    check("null_factory_output",get(ApoId,__uuidof(IClassFactory),nullptr),E_POINTER);
    IClassFactory *factory=nullptr;hr=get(ApoId,__uuidof(IClassFactory),reinterpret_cast<void **>(&factory));
    check("factory",hr,S_OK,factory!=nullptr);
    if(factory){
        check("factory_prevents_unload",unload(),S_FALSE);
        p=reinterpret_cast<void *>(1);hr=factory->CreateInstance(factory,__uuidof(IAudioProcessingObject),&p);
        check("no_aggregation_clears_output",hr,CLASS_E_NOAGGREGATION,p==nullptr);
        check("null_instance_output",factory->CreateInstance(nullptr,__uuidof(IAudioProcessingObject),nullptr),E_POINTER);
        p=reinterpret_cast<void *>(1);hr=factory->CreateInstance(nullptr,UnknownId,&p);check("unknown_instance_iid",hr,E_NOINTERFACE,p==nullptr);
        IAudioProcessingObject *apo=nullptr;hr=factory->CreateInstance(nullptr,__uuidof(IAudioProcessingObject),reinterpret_cast<void **>(&apo));
        check("instance",hr,S_OK,apo!=nullptr);
        if(apo){
            for(const GUID &iid:{__uuidof(IAudioProcessingObjectRT),__uuidof(IAudioProcessingObjectConfiguration),__uuidof(IAudioSystemEffects3)}){
                IUnknown *iface=nullptr;hr=apo->QueryInterface(iid,reinterpret_cast<void **>(&iface));check("required_interface",hr,S_OK,iface!=nullptr);if(iface)iface->Release();
            }
            APO_REG_PROPERTIES *props=nullptr;hr=apo->GetRegistrationProperties(&props);
            check("registration",hr,S_OK,props&&props->clsid==ApoId&&props->u32NumAPOInterfaces==1);CoTaskMemFree(props);apo->Release();
        }
        Outer outer;
        hr=factory->CreateInstance(&outer,__uuidof(IUnknown),reinterpret_cast<void **>(&outer.inner));
        check("aggregate_create",hr,S_OK,outer.inner&&outer.inner!=&outer&&outer.adds==0&&outer.refs==1);
        if(outer.inner){
            IUnknown *identity=nullptr;hr=outer.inner->QueryInterface(__uuidof(IUnknown),reinterpret_cast<void **>(&identity));
            check("private_identity",hr,S_OK,identity==outer.inner&&outer.refs==1);if(identity)identity->Release();
            IAudioProcessingObject *aggregated=nullptr;hr=outer.inner->QueryInterface(__uuidof(IAudioProcessingObject),reinterpret_cast<void **>(&aggregated));
            check("aggregate_interface",hr,S_OK,aggregated&&outer.refs==2);
            if(aggregated){
                identity=nullptr;hr=aggregated->QueryInterface(__uuidof(IUnknown),reinterpret_cast<void **>(&identity));
                check("public_controlling_identity",hr,S_OK,identity==&outer&&outer.refs==3);if(identity)identity->Release();
                identity=nullptr;hr=aggregated->QueryInterface(UnknownId,reinterpret_cast<void **>(&identity));
                check("outer_interface_delegation",hr,S_OK,identity==&outer&&outer.refs==3);if(identity)identity->Release();
                const unsigned adds=outer.adds,releases=outer.releases;
                const ULONG added=aggregated->AddRef(),released=aggregated->Release();
                check("public_reference_delegation",S_OK,S_OK,added==3&&released==2&&outer.adds==adds+1&&outer.releases==releases+1);
                bool repeated=true;
                for(unsigned round=0;round<64;++round){
                    for(const GUID &iid:{__uuidof(IAudioProcessingObjectRT),__uuidof(IAudioProcessingObjectConfiguration),__uuidof(IAudioSystemEffects3)}){
                        IUnknown *iface=nullptr;hr=aggregated->QueryInterface(iid,reinterpret_cast<void **>(&iface));
                        if(FAILED(hr)||!iface){repeated=false;continue;}
                        identity=nullptr;hr=iface->QueryInterface(__uuidof(IUnknown),reinterpret_cast<void **>(&identity));
                        repeated &= SUCCEEDED(hr)&&identity==&outer&&outer.refs==4;
                        if(identity)identity->Release();iface->Release();repeated &= outer.refs==2;
                    }
                }
                check("64_interface_identity_cycles",S_OK,S_OK,repeated);
                aggregated->Release();check("balanced_outer_references",S_OK,S_OK,outer.refs==1&&outer.adds==outer.releases);
            }
            check("private_final_release",HRESULT(outer.inner->Release()),0,outer.refs==1);outer.inner=nullptr;
        }
        factory->Release();
    }
    check("final_unload",unload(),S_OK);
    printf("],\"passed\":%s,\"cases_total\":%u,\"failures\":%u,\"system_dolby_proven\":false}\n",failures?"false":"true",count,failures);
    FreeLibrary(dll);return failures?1:0;
}
