// Quiet ordinary WASAPI stream. Observes saved preference; never sets it.
#include <windows.h>
#include <initguid.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <endpointvolume.h>
#include <functiondiscoverykeys_devpkey.h>
#include <stdio.h>
#include <math.h>
template<class T>struct Owner{T*p=nullptr;~Owner(){if(p)p->Release();}T**out(){return &p;}T*operator->(){return p;}};
struct Stream{IAudioClient*p=nullptr;bool started=false;~Stream(){if(p){if(started)p->Stop();p->Release();}}};
constexpr GUID Effect={0x2330de07,0x4ae3,0x45e1,{0xb2,0x4a,0xb0,0x68,0xa8,0x6f,0x9b,0x6a}};
struct Observer final:IAudioEffectsChangedNotificationClient{
    volatile LONG refs=1,notifications=0;
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID id,void**p)override{if(!p)return E_POINTER;*p=nullptr;if(id!=__uuidof(IUnknown)&&id!=__uuidof(IAudioEffectsChangedNotificationClient))return E_NOINTERFACE;*p=this;AddRef();return S_OK;}
    ULONG STDMETHODCALLTYPE AddRef()override{return ULONG(InterlockedIncrement(&refs));}
    ULONG STDMETHODCALLTYPE Release()override{LONG n=InterlockedDecrement(&refs);if(!n)delete this;return ULONG(n);}
    HRESULT STDMETHODCALLTYPE OnAudioEffectsChanged()override{InterlockedIncrement(&notifications);return S_OK;}
};
int wmain(int argc,WCHAR**argv){
    if(argc!=2)return 2;bool watch=!wcscmp(argv[1],L"--watch");if(!watch&&wcscmp(argv[1],L"--steady"))return 2;
    HRESULT hr=CoInitializeEx(nullptr,COINIT_MULTITHREADED);if(FAILED(hr))return 3;
    bool audio=false,registered=false,gain=false,unchanged=false;UINT64 submitted=0;float maximum=0;unsigned states=0;int history[16]={};DWORD pid=GetCurrentProcessId();
    LPWSTR id=nullptr;WAVEFORMATEX*format=nullptr;FLOAT before=0,after=0;BOOL muteBefore=TRUE,muteAfter=TRUE;
    Owner<Observer> observer;observer.p=new Observer;
    {
        Owner<IMMDeviceEnumerator>en;Owner<IMMDevice>device;Owner<IPropertyStore>store;Owner<IAudioEndpointVolume>volume;
        Owner<IAudioMeterInformation>meter;Owner<IAudioClient2>props;Owner<IAudioRenderClient>writer;
        Owner<IAudioStreamVolume>levels;Owner<IAudioEffectsManager>effects;Stream stream;
        hr=CoCreateInstance(__uuidof(MMDeviceEnumerator),nullptr,CLSCTX_ALL,IID_PPV_ARGS(en.out()));
        if(SUCCEEDED(hr))hr=en->GetDefaultAudioEndpoint(eRender,eConsole,device.out());
        if(SUCCEEDED(hr))hr=device->GetId(&id);
        if(SUCCEEDED(hr))hr=device->OpenPropertyStore(STGM_READ,store.out());
        PROPVARIANT label={};if(SUCCEEDED(hr))hr=store->GetValue(PKEY_Device_FriendlyName,&label);
        constexpr WCHAR prefix[]=L"Speakers (Mi Pad 5 ";
        if(SUCCEEDED(hr)&&(label.vt!=VT_LPWSTR||!label.pwszVal||wcsncmp(label.pwszVal,prefix,wcslen(prefix))))hr=E_INVALIDARG;
        PropVariantClear(&label);
        if(SUCCEEDED(hr))hr=device->Activate(__uuidof(IAudioEndpointVolume),CLSCTX_ALL,nullptr,reinterpret_cast<void**>(volume.out()));
        if(SUCCEEDED(hr))hr=device->Activate(__uuidof(IAudioMeterInformation),CLSCTX_ALL,nullptr,reinterpret_cast<void**>(meter.out()));
        if(SUCCEEDED(hr))hr=volume->GetMasterVolumeLevelScalar(&before);
        if(SUCCEEDED(hr))hr=volume->GetMute(&muteBefore);
        if(SUCCEEDED(hr))hr=meter->GetPeakValue(&maximum);
        if(SUCCEEDED(hr)&&(muteBefore||before<=0||!isfinite(before)||!isfinite(maximum)||maximum>.000001f))hr=HRESULT_FROM_WIN32(ERROR_BUSY);
        if(SUCCEEDED(hr))hr=device->Activate(__uuidof(IAudioClient),CLSCTX_ALL,nullptr,reinterpret_cast<void**>(&stream.p));
        if(SUCCEEDED(hr))hr=stream.p->QueryInterface(IID_PPV_ARGS(props.out()));
        AudioClientProperties properties={};properties.cbSize=sizeof(properties);properties.eCategory=AudioCategory_Media;
        if(SUCCEEDED(hr))hr=props->SetClientProperties(&properties);
        if(SUCCEEDED(hr))hr=stream.p->GetMixFormat(&format);
        constexpr GUID Float32={3,0,0x10,{0x80,0,0,0xaa,0,0x38,0x9b,0x71}};
        if(SUCCEEDED(hr)&&(!format||format->wFormatTag!=WAVE_FORMAT_EXTENSIBLE||format->cbSize<22||format->nChannels!=2||format->nSamplesPerSec!=48000||format->wBitsPerSample!=32||format->nBlockAlign!=8||format->nAvgBytesPerSec!=384000))hr=E_INVALIDARG;
        if(SUCCEEDED(hr)){auto*ext=reinterpret_cast<WAVEFORMATEXTENSIBLE*>(format);if(ext->SubFormat!=Float32||ext->Samples.wValidBitsPerSample!=32||(ext->dwChannelMask&&ext->dwChannelMask!=3))hr=E_INVALIDARG;}
        GUID session={};if(SUCCEEDED(hr))hr=CoCreateGuid(&session);
        if(SUCCEEDED(hr))hr=stream.p->Initialize(AUDCLNT_SHAREMODE_SHARED,0,1000000,0,format,&session);
        if(SUCCEEDED(hr))hr=stream.p->GetService(IID_PPV_ARGS(effects.out()));
        auto inspect=[&](){AUDIO_EFFECT*list=nullptr;UINT32 count=0;HRESULT status=effects->GetAudioEffects(&list,&count);int state=-1;
            if(SUCCEEDED(status))for(UINT32 i=0;i<count;++i)if(list[i].id==Effect)state=int(list[i].state);
            CoTaskMemFree(list);if(FAILED(status))return status;if(state<0)return AUDCLNT_E_EFFECT_NOT_AVAILABLE;
            if(!states||history[states-1]!=state){if(states>=ARRAYSIZE(history))return E_ABORT;history[states++]=state;}return S_OK;};
        if(SUCCEEDED(hr))hr=inspect();
        if(SUCCEEDED(hr)){hr=effects->RegisterAudioEffectsChangedNotificationCallback(observer.p);registered=SUCCEEDED(hr);}
        if(SUCCEEDED(hr))hr=stream.p->GetService(IID_PPV_ARGS(levels.out()));
        UINT32 channels=0;FLOAT values[2]={.01f,.01f};
        if(SUCCEEDED(hr))hr=levels->GetChannelCount(&channels);
        if(SUCCEEDED(hr)&&channels!=2)hr=E_INVALIDARG;
        if(SUCCEEDED(hr))hr=levels->SetAllVolumes(2,values);
        if(SUCCEEDED(hr))hr=levels->GetAllVolumes(2,values);
        gain=SUCCEEDED(hr)&&values[0]==.01f&&values[1]==.01f;if(SUCCEEDED(hr)&&!gain)hr=E_FAIL;
        if(SUCCEEDED(hr))hr=stream.p->GetService(IID_PPV_ARGS(writer.out()));
        UINT32 buffer=0;if(SUCCEEDED(hr))hr=stream.p->GetBufferSize(&buffer);
        const DWORD duration=watch?10000:4000;
        auto supply=[&](UINT32 n){BYTE*data=nullptr;HRESULT status=writer->GetBuffer(n,&data);if(FAILED(status))return status;auto*f=reinterpret_cast<float*>(data);
            for(UINT32 i=0;i<n;++i){double t=double(submitted+i)/48000.;double edge=fmax(0.,fmin(1.,fmin(t/.02,(double(duration)/1000.-t)/.02)));f[i*2]=float(.001*edge*sin(2*3.14159265358979323846*800*t));f[i*2+1]=float(.001*edge*sin(2*3.14159265358979323846*1100*t));}
            status=writer->ReleaseBuffer(n,0);if(SUCCEEDED(status))submitted+=n;return status;};
        if(SUCCEEDED(hr))hr=supply(buffer);
        if(SUCCEEDED(hr)){hr=stream.p->Start();stream.started=SUCCEEDED(hr);audio=stream.started;}
        ULONGLONG start=GetTickCount64(),next=0;
        while(SUCCEEDED(hr)&&stream.started&&GetTickCount64()-start<duration){Sleep(4);UINT32 padding=0;hr=stream.p->GetCurrentPadding(&padding);if(SUCCEEDED(hr)&&padding<buffer)hr=supply(buffer-padding);
            ULONGLONG elapsed=GetTickCount64()-start;if(SUCCEEDED(hr)&&elapsed>=next){hr=inspect();next=elapsed+50;}
            FLOAT peak=0;if(SUCCEEDED(hr))hr=meter->GetPeakValue(&peak);if(SUCCEEDED(hr)){if(!isfinite(peak)||peak>.002f)hr=E_ABORT;else maximum=fmaxf(maximum,peak);}}
        if(stream.started){HRESULT stop=stream.p->Stop();stream.started=false;if(SUCCEEDED(hr))hr=stop;}
        if(registered){HRESULT status=effects->UnregisterAudioEffectsChangedNotificationCallback(observer.p);if(SUCCEEDED(hr))hr=status;}
        HRESULT check=volume.p?volume->GetMasterVolumeLevelScalar(&after):E_POINTER;if(SUCCEEDED(check))check=volume->GetMute(&muteAfter);
        unchanged=SUCCEEDED(check)&&before==after&&muteBefore==muteAfter;
    }
    bool passed=SUCCEEDED(hr)&&audio&&gain&&unchanged&&states;
    printf("{\"hr\":\"0x%08lx\",\"source_pid\":%lu,\"audio_started\":%s,\"submitted_frames\":%llu,\"source_peak\":0.001,\"owned_stream_gain\":0.01,\"volume_unchanged\":%s,\"maximum_meter_peak\":%.9g,\"notifications\":%ld,\"observed_states\":[",ULONG(hr),pid,audio?"true":"false",submitted,unchanged?"true":"false",double(maximum),InterlockedCompareExchange(&observer->notifications,0,0));
    for(unsigned i=0;i<states;++i)printf("%s%d",i?",":"",history[i]);
    printf("],\"preference_written\":false,\"physical_acoustic_test\":false,\"passed\":%s}\n",passed?"true":"false");
    CoTaskMemFree(format);CoTaskMemFree(id);CoUninitialize();return passed?0:5;
}
