// Real Windows stream effects API. Digital control/source evidence only.
// --inspect-{normal,raw}: Initialize/GetService/GetAudioEffects, never Start.
// --normal: quiet same-stream ON/OFF/ON, never endpoint/session volume writes.
#include <windows.h>
#include <initguid.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <endpointvolume.h>
#include <functiondiscoverykeys_devpkey.h>
#include <stdio.h>
#include <math.h>
template<class T>struct Owner {
 T*p=nullptr;~Owner(){if(p)p->Release();}T**out(){return &p;}T*operator->(){return p;}
 Owner()=default;Owner(const Owner&)=delete;Owner&operator=(const Owner&)=delete;
};
struct Scope {HRESULT hr=CoInitializeEx(nullptr,COINIT_MULTITHREADED);~Scope(){if(SUCCEEDED(hr))CoUninitialize();}};
struct Stream {IAudioClient*p=nullptr;bool started=false;~Stream(){if(p){if(started)p->Stop();p->Release();}}};
constexpr GUID Effect={0x2330de07,0x4ae3,0x45e1,{0xb2,0x4a,0xb0,0x68,0xa8,0x6f,0x9b,0x6a}};
constexpr GUID Float32={3,0,0x10,{0x80,0,0,0xaa,0,0x38,0x9b,0x71}};
constexpr float Gain=0.01f;constexpr double Peak=0.001;
class Observer final:public IAudioEffectsChangedNotificationClient {
 volatile LONG refs_=1,count_=0;
public:
 HRESULT STDMETHODCALLTYPE QueryInterface(REFIID id,void**p)override{if(!p)return E_POINTER;*p=nullptr;if(id!=__uuidof(IUnknown)&&id!=__uuidof(IAudioEffectsChangedNotificationClient))return E_NOINTERFACE;*p=this;AddRef();return S_OK;}
 ULONG STDMETHODCALLTYPE AddRef()override{return ULONG(InterlockedIncrement(&refs_));}
 ULONG STDMETHODCALLTYPE Release()override{LONG refs=InterlockedDecrement(&refs_);if(!refs)delete this;return ULONG(refs);}
 HRESULT STDMETHODCALLTYPE OnAudioEffectsChanged()override{InterlockedIncrement(&count_);return S_OK;}
 LONG count(){return InterlockedCompareExchange(&count_,0,0);}
};
static HRESULT effect(IAudioEffectsManager*manager,AUDIO_EFFECT*found,UINT32*total) {
 AUDIO_EFFECT*list=nullptr;UINT32 count=0;HRESULT hr=manager->GetAudioEffects(&list,&count);
 if(SUCCEEDED(hr)){if(total)*total=count;hr=S_FALSE;for(UINT32 i=0;i<count;++i)if(list[i].id==Effect){if(found)*found=list[i];hr=S_OK;break;}}
 CoTaskMemFree(list);return hr;
}
int wmain(int argc,WCHAR**argv) {
 if(argc!=2)return 2;
 bool inspect=wcscmp(argv[1],L"--inspect-normal")==0||wcscmp(argv[1],L"--inspect-raw")==0;
 bool raw=wcscmp(argv[1],L"--inspect-raw")==0;
 if(!inspect&&wcscmp(argv[1],L"--normal"))return 2;
 Scope com;if(FAILED(com.hr))return 3;
 Owner<IMMDeviceEnumerator>enumerator;Owner<IMMDevice>device;Owner<IPropertyStore>store;
 Owner<IAudioEndpointVolume>volume;Owner<IAudioMeterInformation>meter;Owner<IAudioClient2>properties;
 Owner<IAudioEffectsManager>manager;Owner<IAudioRenderClient>writer;Owner<IAudioStreamVolume>levels;
 Owner<Observer>observer;Stream stream;WAVEFORMATEX*format=nullptr;LPWSTR id=nullptr;
 HRESULT hr=CoCreateInstance(__uuidof(MMDeviceEnumerator),nullptr,CLSCTX_ALL,IID_PPV_ARGS(enumerator.out()));
 if(SUCCEEDED(hr))hr=enumerator->GetDefaultAudioEndpoint(eRender,eConsole,device.out());
 if(SUCCEEDED(hr))hr=device->GetId(&id);
 if(SUCCEEDED(hr))hr=device->OpenPropertyStore(STGM_READ,store.out());
 PROPVARIANT name={};if(SUCCEEDED(hr))hr=store->GetValue(PKEY_Device_FriendlyName,&name);
 constexpr WCHAR Prefix[]=L"Speakers (Mi Pad 5 ";
 if(SUCCEEDED(hr)&&(name.vt!=VT_LPWSTR||!name.pwszVal||wcsncmp(name.pwszVal,Prefix,wcslen(Prefix))))hr=E_INVALIDARG;
 PropVariantClear(&name);
 if(SUCCEEDED(hr))hr=device->Activate(__uuidof(IAudioEndpointVolume),CLSCTX_ALL,nullptr,reinterpret_cast<void**>(volume.out()));
 if(SUCCEEDED(hr))hr=device->Activate(__uuidof(IAudioMeterInformation),CLSCTX_ALL,nullptr,reinterpret_cast<void**>(meter.out()));
 float before=0,after=0,initialPeak=0;BOOL mutedBefore=TRUE,mutedAfter=TRUE;
 if(SUCCEEDED(hr))hr=volume->GetMasterVolumeLevelScalar(&before);
 if(SUCCEEDED(hr))hr=volume->GetMute(&mutedBefore);
 if(SUCCEEDED(hr))hr=meter->GetPeakValue(&initialPeak);
 if(SUCCEEDED(hr)&&(!isfinite(before)||!isfinite(initialPeak)))hr=E_INVALIDARG;
 if(SUCCEEDED(hr)&&!inspect&&(mutedBefore||before<=0||initialPeak>0.000001f))hr=HRESULT_FROM_WIN32(ERROR_BUSY);
 if(SUCCEEDED(hr))hr=device->Activate(__uuidof(IAudioClient),CLSCTX_ALL,nullptr,reinterpret_cast<void**>(&stream.p));
 if(SUCCEEDED(hr))hr=stream.p->QueryInterface(IID_PPV_ARGS(properties.out()));
 AudioClientProperties props={};props.cbSize=sizeof(props);props.eCategory=AudioCategory_Media;props.Options=raw?AUDCLNT_STREAMOPTIONS_RAW:AUDCLNT_STREAMOPTIONS_NONE;
 if(SUCCEEDED(hr))hr=properties->SetClientProperties(&props);
 if(SUCCEEDED(hr))hr=stream.p->GetMixFormat(&format);
 if(SUCCEEDED(hr)&&(!format||format->wFormatTag!=WAVE_FORMAT_EXTENSIBLE||format->cbSize<22||format->nChannels!=2||format->nSamplesPerSec!=48000||format->wBitsPerSample!=32||format->nBlockAlign!=8||format->nAvgBytesPerSec!=384000))hr=E_INVALIDARG;
 if(SUCCEEDED(hr)){auto*ext=reinterpret_cast<WAVEFORMATEXTENSIBLE*>(format);if(ext->SubFormat!=Float32||ext->Samples.wValidBitsPerSample!=32||(ext->dwChannelMask!=0&&ext->dwChannelMask!=3))hr=E_INVALIDARG;}
 GUID session={};if(SUCCEEDED(hr))hr=CoCreateGuid(&session);
 if(SUCCEEDED(hr))hr=stream.p->Initialize(AUDCLNT_SHAREMODE_SHARED,0,1000000,0,format,&session);
 if(SUCCEEDED(hr))hr=stream.p->GetService(IID_PPV_ARGS(manager.out()));
 HRESULT managerStatus=hr;AUDIO_EFFECT found={};UINT32 count=0;
 HRESULT present=SUCCEEDED(hr)?effect(manager.p,&found,&count):hr;
 if(SUCCEEDED(hr)&&FAILED(present))hr=present;
 if(inspect){
  wprintf(L"{\"mode\":\"%ls\",\"endpoint\":\"%ls\",\"initialize_and_manager_hr\":\"0x%08lx\",\"effects_query_hr\":\"0x%08lx\",\"effect_count\":%u,\"nabu_effect_present\":%s,\"nabu_controllable\":%s,\"nabu_state\":%d,\"audio_started\":false,\"samples_submitted\":0,\"settings_written\":false,\"physical_acoustic_test\":false}\n",raw?L"RAW":L"Normal",id?id:L"",ULONG(managerStatus),ULONG(present),count,present==S_OK?L"true":L"false",present==S_OK&&found.canSetState?L"true":L"false",present==S_OK?int(found.state):-1);
  CoTaskMemFree(format);CoTaskMemFree(id);return SUCCEEDED(hr)?0:4;
 }
 if(SUCCEEDED(hr)&&(present!=S_OK||!found.canSetState))hr=AUDCLNT_E_EFFECT_NOT_AVAILABLE;
 AUDIO_EFFECT_STATE original=found.state;bool registered=false,changed=false;
 HRESULT control[3]={E_PENDING,E_PENDING,E_PENDING},query[3]={E_PENDING,E_PENDING,E_PENDING};
 bool observed[3]={false,false,false};int observedState[3]={-1,-1,-1};UINT64 markers[3]={},observedMarkers[3]={};
 ULONGLONG pendingUntil=0;int pending=-1;
 UINT32 buffer=0;UINT64 submitted=0;float maximumPeak=initialPeak;bool gainVerified=false;DWORD audioPid=GetCurrentProcessId();
 if(SUCCEEDED(hr)){observer.p=new Observer;hr=manager->RegisterAudioEffectsChangedNotificationCallback(observer.p);registered=SUCCEEDED(hr);}
 if(SUCCEEDED(hr))hr=stream.p->GetService(IID_PPV_ARGS(levels.out()));
 UINT32 channels=0;float gains[2]={Gain,Gain};
 if(SUCCEEDED(hr))hr=levels->GetChannelCount(&channels);
 if(SUCCEEDED(hr)&&channels!=2)hr=E_INVALIDARG;
 if(SUCCEEDED(hr))hr=levels->SetAllVolumes(2,gains);
 if(SUCCEEDED(hr))hr=levels->GetAllVolumes(2,gains);
 gainVerified=SUCCEEDED(hr)&&gains[0]==Gain&&gains[1]==Gain;if(SUCCEEDED(hr)&&!gainVerified)hr=E_FAIL;
 if(SUCCEEDED(hr))hr=stream.p->GetService(IID_PPV_ARGS(writer.out()));
 if(SUCCEEDED(hr))hr=stream.p->GetBufferSize(&buffer);
 auto snapshot=[&](unsigned phase,AUDIO_EFFECT_STATE desired){
  AUDIO_EFFECT current={};query[phase]=effect(manager.p,&current,nullptr);
  observedState[phase]=query[phase]==S_OK?int(current.state):-1;
  observed[phase]=query[phase]==S_OK&&current.canSetState&&current.state==desired;
  if(observed[phase])observedMarkers[phase]=submitted;
  return FAILED(query[phase])?query[phase]:S_OK;
 };
 auto set=[&](unsigned phase,AUDIO_EFFECT_STATE desired){
  markers[phase]=submitted;control[phase]=manager->SetAudioEffectState(Effect,desired);
  if(FAILED(control[phase]))return control[phase];changed=true;
  pending=int(phase);pendingUntil=GetTickCount64()+1000;
  // The manager can acknowledge the request before the engine has applied it.
  // Poll while feeding the active stream; an immediate old snapshot is not failure.
  return snapshot(phase,desired);
 };
 auto supply=[&](UINT32 n){
  BYTE*data=nullptr;HRESULT s=writer->GetBuffer(n,&data);if(FAILED(s))return s;
  auto*f=reinterpret_cast<float*>(data);
  for(UINT32 i=0;i<n;++i){double t=double(submitted+i)/48000.;double edge=fmin(1.,fmin(t/.02,(5.-t)/.02));if(edge<0)edge=0;
   f[i*2]=float(Peak*edge*sin(2*3.14159265358979323846*800*t));f[i*2+1]=float(Peak*edge*sin(2*3.14159265358979323846*1100*t));}
  s=writer->ReleaseBuffer(n,0);if(SUCCEEDED(s))submitted+=n;return s;
 };
 if(SUCCEEDED(hr))hr=set(0,AUDIO_EFFECT_STATE_ON);
 while(SUCCEEDED(hr)&&!observed[0]&&GetTickCount64()<pendingUntil){Sleep(5);hr=snapshot(0,AUDIO_EFFECT_STATE_ON);}
 if(SUCCEEDED(hr)&&!observed[0])hr=HRESULT_FROM_WIN32(WAIT_TIMEOUT);
 if(observed[0])pending=-1;
 if(SUCCEEDED(hr))hr=supply(buffer);
 if(SUCCEEDED(hr)){hr=stream.p->Start();stream.started=SUCCEEDED(hr);}
 bool started=stream.started;unsigned phase=0;ULONGLONG start=GetTickCount64();
 while(SUCCEEDED(hr)&&started&&GetTickCount64()-start<5000){
  Sleep(4);ULONGLONG elapsed=GetTickCount64()-start;
  if(phase==0&&elapsed>=2000){hr=set(1,AUDIO_EFFECT_STATE_OFF);phase=1;}
  if(phase==1&&elapsed>=3000&&observed[1]){hr=set(2,AUDIO_EFFECT_STATE_ON);phase=2;}
  UINT32 padding=0;if(SUCCEEDED(hr))hr=stream.p->GetCurrentPadding(&padding);
  if(SUCCEEDED(hr)&&padding<buffer)hr=supply(buffer-padding);
  if(SUCCEEDED(hr)&&pending>=0){
   hr=snapshot(unsigned(pending),pending==1?AUDIO_EFFECT_STATE_OFF:AUDIO_EFFECT_STATE_ON);
   if(observed[pending])pending=-1;
   else if(GetTickCount64()>=pendingUntil)hr=HRESULT_FROM_WIN32(WAIT_TIMEOUT);
  }
  float peak=0;if(SUCCEEDED(hr))hr=meter->GetPeakValue(&peak);
  if(SUCCEEDED(hr)){if(!isfinite(peak)||peak>0.002f)hr=E_ABORT;else maximumPeak=fmaxf(maximumPeak,peak);}
 }
 HRESULT restored=S_OK,unregistered=S_OK;
 if(changed)restored=manager->SetAudioEffectState(Effect,original);
 if(stream.started){HRESULT stop=stream.p->Stop();stream.started=false;if(SUCCEEDED(hr))hr=stop;}
 if(registered)unregistered=manager->UnregisterAudioEffectsChangedNotificationCallback(observer.p);
 HRESULT volumeRead=volume.p?volume->GetMasterVolumeLevelScalar(&after):E_POINTER;
 if(SUCCEEDED(volumeRead))volumeRead=volume->GetMute(&mutedAfter);
 bool unchanged=SUCCEEDED(volumeRead)&&before==after&&mutedBefore==mutedAfter;
 bool passed=SUCCEEDED(hr)&&SUCCEEDED(restored)&&SUCCEEDED(unregistered)&&started&&gainVerified&&unchanged&&phase==2&&observed[0]&&observed[1]&&observed[2];
 wprintf(L"{\"hr\":\"0x%08lx\",\"endpoint\":\"%ls\",\"source_pid\":%lu,\"audio_started\":%s,\"submitted_frames\":%llu,\"source_peak\":0.001,\"owned_stream_gain\":0.01,\"stream_gain_verified\":%s,\"volume_unchanged\":%s,\"maximum_meter_peak\":%.9g,\"notifications\":%ld,\"controls\":[",ULONG(hr),id?id:L"",audioPid,started?L"true":L"false",submitted,gainVerified?L"true":L"false",unchanged?L"true":L"false",double(maximumPeak),observer.p?observer->count():0);
 for(unsigned i=0;i<3;++i)wprintf(L"%ls{\"state\":\"%ls\",\"hr\":\"0x%08lx\",\"query_hr\":\"0x%08lx\",\"observed_state\":%d,\"observed\":%s,\"source_frames_before_request\":%llu,\"source_frames_at_observation\":%llu}",i?L",":L"",i==1?L"OFF":L"ON",ULONG(control[i]),ULONG(query[i]),observedState[i],observed[i]?L"true":L"false",markers[i],observedMarkers[i]);
 wprintf(L"],\"restore_hr\":\"0x%08lx\",\"unregister_hr\":\"0x%08lx\",\"physical_acoustic_test\":false,\"dolby_sample_processing_proven\":false,\"passed\":%s}\n",ULONG(restored),ULONG(unregistered),passed?L"true":L"false");
 CoTaskMemFree(format);CoTaskMemFree(id);return passed?0:5;
}
