// Explicit COM fixture only. Opens no endpoint and registers no driver.
#include <windows.h>
#include <stdio.h>
#include <thread>
#include <atomic>
#include "NabuDolbyApo.h"
constexpr GUID Effect={0x2330de07,0x4ae3,0x45e1,{0xb2,0x4a,0xb0,0x68,0xa8,0x6f,0x9b,0x6a}};
constexpr GUID Raw={0x9e90ea20,0xb493,0x4fd1,{0xa1,0xa8,0x7e,0x13,0x61,0xa9,0x56,0xcf}};
struct TransportInit {APOInitBaseStruct base;UINT32 signature;WCHAR name[128];};
using Create=void*(WINAPI*)(const WCHAR*,UINT32);
using Close=void(WINAPI*)(void*);
static unsigned cases=0,failures=0;
static void check(const char* name,bool ok) {
 if(cases++)putchar(',');printf("{\"name\":\"%s\",\"passed\":%s}",name,ok?"true":"false");if(!ok)++failures;
}
int wmain(int argc,WCHAR**argv) {
 if(argc!=2)return 2;
 if(FAILED(CoInitializeEx(nullptr,COINIT_MULTITHREADED)))return 3;
 HMODULE dll=LoadLibraryExW(argv[1],nullptr,LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|LOAD_LIBRARY_SEARCH_SYSTEM32);
 if(!dll)return 4;
 auto get=reinterpret_cast<HRESULT(WINAPI*)(REFCLSID,REFIID,void**)>(GetProcAddress(dll,"DllGetClassObject"));
 auto create=reinterpret_cast<Create>(GetProcAddress(dll,"NabuTransportCreate"));
 auto close=reinterpret_cast<Close>(GetProcAddress(dll,"NabuTransportClose"));
 if(!get||!create||!close)return 5;
 IClassFactory*factory=nullptr;if(FAILED(get(NabuDolbyApoId,IID_PPV_ARGS(&factory))))return 6;
 printf("{\"audio_opened\":false,\"driver_installed\":false,\"checks\":[");
 for(unsigned mode=0;mode<5;++mode) {
  IAudioProcessingObject*apo=nullptr;IAudioSystemEffects3*fx=nullptr;
  if(FAILED(factory->CreateInstance(nullptr,IID_PPV_ARGS(&apo)))||FAILED(apo->QueryInterface(IID_PPV_ARGS(&fx))))return 7;
  HRESULT hr;
  if(mode==0) {APOInitBaseStruct init={sizeof(init),NabuDolbyApoId};hr=apo->Initialize(sizeof(init),reinterpret_cast<BYTE*>(&init));}
  else if(mode<3) {
   APOInitSystemEffects2 init={};init.APOInit.cbSize=sizeof(init);init.APOInit.clsid=NabuDolbyApoId;
   init.AudioProcessingMode=mode==1?Raw:GUID_NULL;init.InitializeForDiscoveryOnly=mode==2;
   hr=apo->Initialize(sizeof(init),reinterpret_cast<BYTE*>(&init));
  }else {
   APOInitSystemEffects3 init={};init.APOInit.cbSize=sizeof(init);init.APOInit.clsid=NabuDolbyApoId;
   init.AudioProcessingMode=mode==3?Raw:GUID_NULL;init.InitializeForDiscoveryOnly=mode==4;
   hr=apo->Initialize(sizeof(init),reinterpret_cast<BYTE*>(&init));
  }
  check("unavailable_initialize",hr==S_OK);
  AUDIO_SYSTEMEFFECT*effects=nullptr;UINT count=1;
  check("unavailable_control_list_empty",fx->GetControllableSystemEffectsList(&effects,&count,nullptr)==S_OK&&!effects&&!count);CoTaskMemFree(effects);
  check("unavailable_on_rejected",fx->SetAudioSystemEffectState(Effect,AUDIO_SYSTEMEFFECT_STATE_ON)==HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED));
  check("unavailable_off_rejected",fx->SetAudioSystemEffectState(Effect,AUDIO_SYSTEMEFFECT_STATE_OFF)==HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED));
  GUID*ids=nullptr;count=1;
  check("unavailable_legacy_list_empty",fx->GetEffectsList(&ids,&count,nullptr)==S_OK&&!ids&&!count);CoTaskMemFree(ids);
  HNSTIME latency=-1;check("unavailable_zero_latency",apo->GetLatency(&latency)==S_OK&&latency==0);
  fx->Release();apo->Release();
 }
 WCHAR name[128];swprintf_s(name,L"Local\\NabuDolbyLab-control-%lu-%llu",GetCurrentProcessId(),GetTickCount64());
 void*transport=create(name,64);if(!transport)return 8;
 IAudioProcessingObject*apo=nullptr;IAudioSystemEffects3*fx=nullptr;
 if(FAILED(factory->CreateInstance(nullptr,IID_PPV_ARGS(&apo)))||FAILED(apo->QueryInterface(IID_PPV_ARGS(&fx))))return 9;
 TransportInit init={{sizeof(init),NabuDolbyApoId},0x3149424e,{}};wcscpy_s(init.name,name);
 if(FAILED(apo->Initialize(sizeof(init),reinterpret_cast<BYTE*>(&init))))return 10;
 DWORD before=0,after=0;GetProcessHandleCount(GetCurrentProcess(),&before);
 bool replaced=true;
 for(unsigned i=0;i<10000;++i) {
  HANDLE event=CreateEventW(nullptr,TRUE,FALSE,nullptr);AUDIO_SYSTEMEFFECT*effects=nullptr;GUID*ids=nullptr;UINT count=0;
  if(!event)return 11;
  replaced &= fx->GetControllableSystemEffectsList(&effects,&count,event)==S_OK&&count==1&&effects&&effects->canSetState;
  CoTaskMemFree(effects);
  replaced &= fx->GetEffectsList(&ids,&count,event)==S_OK&&count==1;CoTaskMemFree(ids);CloseHandle(event);
 }
 AUDIO_SYSTEMEFFECT*effects=nullptr;GUID*ids=nullptr;UINT count=0;
 fx->GetControllableSystemEffectsList(&effects,&count,nullptr);CoTaskMemFree(effects);
 fx->GetEffectsList(&ids,&count,nullptr);CoTaskMemFree(ids);GetProcessHandleCount(GetCurrentProcess(),&after);
 check("10000_event_replacements",replaced);check("event_replacements_no_handle_leak",before==after);
 HANDLE start=CreateEventW(nullptr,FALSE,FALSE,nullptr),done=CreateEventW(nullptr,FALSE,FALSE,nullptr);
 if(!start||!done)return 12;
 std::atomic<bool>stop{false},bad{false};std::atomic<unsigned>target{0};
 std::thread setter([&] {
  for(;;) {
   if(WaitForSingleObject(start,10000)!=WAIT_OBJECT_0){bad=true;break;}if(stop)break;
   if(fx->SetAudioSystemEffectState(Effect,target?AUDIO_SYSTEMEFFECT_STATE_ON:AUDIO_SYSTEMEFFECT_STATE_OFF)!=S_OK)bad=true;
   SetEvent(done);
  }
 });
 bool raced=true;
 for(unsigned i=0;i<2048;++i) {
  HANDLE event=CreateEventW(nullptr,TRUE,FALSE,nullptr),observer=nullptr;
  if(!event||!DuplicateHandle(GetCurrentProcess(),event,GetCurrentProcess(),&observer,0,FALSE,DUPLICATE_SAME_ACCESS))return 13;
  target=i%2;SetEvent(start);
  effects=nullptr;count=0;HRESULT hr=fx->GetControllableSystemEffectsList(&effects,&count,event);CloseHandle(event);
  bool valid=hr==S_OK&&count==1&&effects;
  bool snapshotTarget=valid&&effects->state==(target?AUDIO_SYSTEMEFFECT_STATE_ON:AUDIO_SYSTEMEFFECT_STATE_OFF);CoTaskMemFree(effects);
  if(WaitForSingleObject(done,10000)!=WAIT_OBJECT_0)valid=false;
  raced &= valid&&(snapshotTarget||WaitForSingleObject(observer,0)==WAIT_OBJECT_0);CloseHandle(observer);
 }
 stop=true;SetEvent(start);setter.join();CloseHandle(start);CloseHandle(done);
 check("2048_register_snapshot_races",raced&&!bad);
 fx->GetControllableSystemEffectsList(&effects,&count,nullptr);CoTaskMemFree(effects);
 GetProcessHandleCount(GetCurrentProcess(),&after);check("race_notifications_no_handle_leak",before==after);
 fx->Release();apo->Release();close(transport);factory->Release();FreeLibrary(dll);CoUninitialize();
 printf("],\"cases_total\":%u,\"failures\":%u,\"passed\":%s}\n",cases,failures,failures?"false":"true");return failures?1:0;
}
