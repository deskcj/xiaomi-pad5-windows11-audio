// Quiet WASAPI L/R probe. Limits only its own stream, at any endpoint volume.
// Acoustic corner routing, temperature and loud-volume safety are not measured.
#include <windows.h>
#include <initguid.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <endpointvolume.h>
#include <functiondiscoverykeys_devpkey.h>
#include <stdio.h>
#include <math.h>
template<class T> struct Owner { T *p=nullptr;~Owner(){if(p)p->Release();}T **out(){return &p;}T *operator->(){return p;} };
struct ComScope { HRESULT hr=CoInitializeEx(nullptr,COINIT_MULTITHREADED);~ComScope(){if(SUCCEEDED(hr))CoUninitialize();} };
struct Stream { IAudioClient *p=nullptr;bool started=false;~Stream(){if(p){if(started)p->Stop();p->Release();}} };
constexpr double Rate=48000.0,Amplitude=0.001;
constexpr float StreamGain=0.01f; // -40 dB in this stream only; never device volume.
static float sample(UINT64 frame) {
 double t=double(frame)/Rate;if(t<.15||t>=1.15)return 0;
 double edge=fmin(1.0,fmin((t-.15)/.02,(1.15-t)/.02));
 return float(Amplitude*edge*sin(2.0*3.14159265358979323846*800.0*t));
}
static bool run(IMMDevice *device,IAudioMeterInformation *meter,int channel,bool raw) {
 Stream render,capture;Owner<IAudioClient2> props;Owner<IAudioRenderClient> writer;Owner<IAudioCaptureClient> reader;Owner<IAudioStreamVolume> streamVolume;
 GUID session={};FLOAT streamLevels[2]={StreamGain,StreamGain};
 bool streamGainVerified=false;
 WAVEFORMATEX *format=nullptr;UINT32 buffer=0;UINT64 submitted=0,captured=0;UINT32 discontinuities=0;
 double power[2]={},peak[2]={},sineSum[2]={},cosineSum[2]={};UINT64 active=0;UINT32 nonfinite=0;
 HRESULT hr=device->Activate(__uuidof(IAudioClient),CLSCTX_ALL,nullptr,(void **)&render.p);
 if(SUCCEEDED(hr))hr=render.p->QueryInterface(__uuidof(IAudioClient2),(void **)props.out());
 AudioClientProperties ap={};ap.cbSize=sizeof(ap);ap.eCategory=AudioCategory_Media;ap.Options=raw?AUDCLNT_STREAMOPTIONS_RAW:AUDCLNT_STREAMOPTIONS_NONE;
 if(SUCCEEDED(hr))hr=props->SetClientProperties(&ap);
 if(SUCCEEDED(hr))hr=render.p->GetMixFormat(&format);
 constexpr GUID Float32={3,0,0x10,{0x80,0,0,0xaa,0,0x38,0x9b,0x71}};
 if(SUCCEEDED(hr)&&(!format||format->wFormatTag!=WAVE_FORMAT_EXTENSIBLE||format->cbSize<22||format->nChannels!=2||
    format->wBitsPerSample!=32||format->nSamplesPerSec!=48000||format->nBlockAlign!=8||format->nAvgBytesPerSec!=384000))hr=E_INVALIDARG;
 if(SUCCEEDED(hr)){
  const auto *ext=reinterpret_cast<const WAVEFORMATEXTENSIBLE *>(format);
  if(ext->SubFormat!=Float32||ext->Samples.wValidBitsPerSample!=32||(ext->dwChannelMask!=3&&ext->dwChannelMask!=0))hr=E_INVALIDARG;
 }
 if(SUCCEEDED(hr))hr=CoCreateGuid(&session);
 if(SUCCEEDED(hr))hr=render.p->Initialize(AUDCLNT_SHAREMODE_SHARED,0,1000000,0,format,&session);
 // Set/verify gain before submitting or starting audio. This interface adjusts
 // only the owned render stream; endpoint/session/mute settings are not written.
 if(SUCCEEDED(hr))hr=render.p->GetService(__uuidof(IAudioStreamVolume),(void **)streamVolume.out());
 UINT32 volumeChannels=0;
 if(SUCCEEDED(hr))hr=streamVolume->GetChannelCount(&volumeChannels);
 if(SUCCEEDED(hr)&&volumeChannels!=2)hr=E_INVALIDARG;
 if(SUCCEEDED(hr))hr=streamVolume->SetAllVolumes(2,streamLevels);
 if(SUCCEEDED(hr))hr=streamVolume->GetAllVolumes(2,streamLevels);
 streamGainVerified=SUCCEEDED(hr)&&streamLevels[0]==StreamGain&&streamLevels[1]==StreamGain;
 if(SUCCEEDED(hr)&&!streamGainVerified)hr=E_FAIL;
 if(SUCCEEDED(hr))hr=render.p->GetService(__uuidof(IAudioRenderClient),(void **)writer.out());
 if(SUCCEEDED(hr))hr=render.p->GetBufferSize(&buffer);
 if(SUCCEEDED(hr))hr=device->Activate(__uuidof(IAudioClient),CLSCTX_ALL,nullptr,(void **)&capture.p);
 if(SUCCEEDED(hr))hr=capture.p->Initialize(AUDCLNT_SHAREMODE_SHARED,AUDCLNT_STREAMFLAGS_LOOPBACK,1000000,0,format,nullptr);
 if(SUCCEEDED(hr))hr=capture.p->GetService(__uuidof(IAudioCaptureClient),(void **)reader.out());
 auto supply=[&](UINT32 n) {
  BYTE *data=nullptr;HRESULT s=writer->GetBuffer(n,&data);
  if(SUCCEEDED(s)){auto f=reinterpret_cast<float *>(data);for(UINT32 i=0;i<n;i++){f[2*i]=channel==0?sample(submitted+i):0;f[2*i+1]=channel==1?sample(submitted+i):0;}
   s=writer->ReleaseBuffer(n,0);if(SUCCEEDED(s))submitted+=n;}
  return s;
 };
 if(SUCCEEDED(hr))hr=supply(buffer);
 if(SUCCEEDED(hr)){hr=capture.p->Start();capture.started=SUCCEEDED(hr);}
 if(SUCCEEDED(hr)){hr=render.p->Start();render.started=SUCCEEDED(hr);}
 const bool started=render.started;ULONGLONG start=GetTickCount64();float maximumMeter=0;
 while(SUCCEEDED(hr)&&GetTickCount64()-start<1600) {
  Sleep(4);UINT32 padding=0;hr=render.p->GetCurrentPadding(&padding);
  if(SUCCEEDED(hr)&&padding<buffer)hr=supply(buffer-padding);
  UINT32 packet=0;if(SUCCEEDED(hr))hr=reader->GetNextPacketSize(&packet);
  while(SUCCEEDED(hr)&&packet) {
   BYTE *data=nullptr;UINT32 n=0;DWORD flags=0;UINT64 position=0,qpc=0;
   hr=reader->GetBuffer(&data,&n,&flags,&position,&qpc);
   if(FAILED(hr))break;
   if(flags&AUDCLNT_BUFFERFLAGS_DATA_DISCONTINUITY)++discontinuities;
   if(!(flags&AUDCLNT_BUFFERFLAGS_SILENT)) {
    auto f=reinterpret_cast<const float *>(data);
    for(UINT32 i=0;i<n;i++) {
     double v[2]={f[2*i],f[2*i+1]};
     if(!isfinite(v[0])||!isfinite(v[1])){++nonfinite;continue;}
     // Loopback is attenuated by the user's volume. Accumulate every frame;
     // an input-amplitude gate incorrectly rejects a valid quiet output.
     if(fabs(v[channel])>1e-9)++active;
     const double angle=2.0*3.14159265358979323846*800.0*double(captured+i)/Rate;
     const double s=sin(angle),co=cos(angle);
     for(int c=0;c<2;c++){power[c]+=v[c]*v[c];peak[c]=fmax(peak[c],fabs(v[c]));sineSum[c]+=v[c]*s;cosineSum[c]+=v[c]*co;}
    }
   }
   captured+=n;HRESULT release=reader->ReleaseBuffer(n);if(SUCCEEDED(hr))hr=release;
   if(SUCCEEDED(hr))hr=reader->GetNextPacketSize(&packet);
  }
  float now=0;HRESULT meterStatus=meter->GetPeakValue(&now);
  if(FAILED(meterStatus))hr=meterStatus;
  else {if(!isfinite(now))hr=E_ABORT;else {maximumMeter=fmaxf(maximumMeter,now);if(now>.002f)hr=E_ABORT;}}
 }
 if(render.started){HRESULT s=render.p->Stop();render.started=false;if(SUCCEEDED(hr))hr=s;}
 if(capture.started){HRESULT s=capture.p->Stop();capture.started=false;if(SUCCEEDED(hr))hr=s;}
 if(SUCCEEDED(hr))hr=streamVolume->GetAllVolumes(2,streamLevels);
 const bool streamGainUnchanged=streamGainVerified&&SUCCEEDED(hr)&&streamLevels[0]==StreamGain&&streamLevels[1]==StreamGain;
 double rms[2]={captured?sqrt(power[0]/double(captured)):0,captured?sqrt(power[1]/double(captured)):0};
 wprintf(L"{\"stream_gain_verified\":%s,\"stream_gain\":%.9g,\"measurement\":",streamGainUnchanged?L"true":L"false",double(StreamGain));
 double ratio=rms[channel]>0?rms[1-channel]/rms[channel]:1;
 double tone[2]={captured?2.0*hypot(sineSum[0],cosineSum[0])/double(captured):0,captured?2.0*hypot(sineSum[1],cosineSum[1])/double(captured):0};
 double toneRatio=tone[channel]>0?tone[1-channel]/tone[channel]:1;
 // At this deliberately quiet level, PCM16 quantization/dither can be a
 // considerable fraction of total RMS. Compare the actual 800Hz tone, keep
 // the unfiltered RMS as evidence, and do not count dither as channel mixing.
 bool separated=active>1000&&tone[channel]>1e-7&&toneRatio<.06;
 bool passed=SUCCEEDED(hr)&&started&&streamGainUnchanged&&nonfinite==0&&separated;
 wprintf(L"{\"channel\":\"%ls\",\"raw\":%s,\"hr\":\"0x%08lx\",\"submitted_frames\":%llu,\"captured_frames\":%llu,\"active_frames\":%llu,\"rms\":[%.12g,%.12g],\"peak\":[%.12g,%.12g],\"opposite_rms_ratio\":%.12g,\"tone_800hz_amplitudes\":[%.12g,%.12g],\"opposite_tone_ratio\":%.12g,\"digital_separation_passed\":%s,\"capture_discontinuities\":%u,\"nonfinite_frames\":%u,\"maximum_meter_peak\":%.9g,\"passed\":%s}",channel==0?L"L":L"R",raw?L"true":L"false",ULONG(hr),submitted,captured,active,rms[0],rms[1],peak[0],peak[1],ratio,tone[0],tone[1],toneRatio,separated?L"true":L"false",discontinuities,nonfinite,double(maximumMeter),passed?L"true":L"false");fflush(stdout);
 wprintf(L"}");CoTaskMemFree(format);return passed;
}
int wmain() {
 ComScope com;HRESULT hr=com.hr;if(FAILED(hr))return 1;
 Owner<IMMDeviceEnumerator> enumerator;Owner<IMMDevice> device;Owner<IPropertyStore> store;
 Owner<IAudioMeterInformation> meter;Owner<IAudioEndpointVolume> volume;LPWSTR id=nullptr;
 hr=CoCreateInstance(__uuidof(MMDeviceEnumerator),nullptr,CLSCTX_ALL,__uuidof(IMMDeviceEnumerator),(void **)enumerator.out());
 if(SUCCEEDED(hr))hr=enumerator->GetDefaultAudioEndpoint(eRender,eConsole,device.out());
 if(SUCCEEDED(hr))hr=device->GetId(&id);
 if(SUCCEEDED(hr))hr=device->OpenPropertyStore(STGM_READ,store.out());
 PROPVARIANT name={};if(SUCCEEDED(hr))hr=store->GetValue(PKEY_Device_FriendlyName,&name);
 const WCHAR prefix[]=L"Speakers (Mi Pad 5 ";if(SUCCEEDED(hr)&&(name.vt!=VT_LPWSTR||wcsncmp(name.pwszVal,prefix,wcslen(prefix))!=0))hr=E_INVALIDARG;
 PropVariantClear(&name);
 if(SUCCEEDED(hr))hr=device->Activate(__uuidof(IAudioMeterInformation),CLSCTX_ALL,nullptr,(void **)meter.out());
 if(SUCCEEDED(hr))hr=device->Activate(__uuidof(IAudioEndpointVolume),CLSCTX_ALL,nullptr,(void **)volume.out());
 float before=0,after=0,initialPeak=0;BOOL mutedBefore=TRUE,mutedAfter=TRUE;
 if(SUCCEEDED(hr))hr=volume->GetMasterVolumeLevelScalar(&before);
 if(SUCCEEDED(hr))hr=volume->GetMute(&mutedBefore);
 if(SUCCEEDED(hr))hr=meter->GetPeakValue(&initialPeak);
 if(FAILED(hr)||mutedBefore||!isfinite(before)||before<=0||before>1.0f||!isfinite(initialPeak)||initialPeak>.000001f){wprintf(L"{\"preflight_hr\":\"0x%08lx\",\"muted\":%s,\"volume\":%.9g,\"initial_peak\":%.9g,\"audio_started\":false}\n",ULONG(hr),mutedBefore?L"true":L"false",double(before),double(initialPeak));CoTaskMemFree(id);return 2;}
 wprintf(L"{\"endpoint\":\"%ls\",\"input_peak_limit\":0.001,\"input_dbfs\":-60,\"stream_gain_db\":-40,\"source_peak_times_stream_gain\":0.00001,\"endpoint_volume_written\":false,\"registry_modified\":false,\"acoustic_lr_proven\":false,\"maximum_volume_safety_proven\":false,\"cases\":[",id);fflush(stdout);
 bool passed=true;for(int raw=0;raw<2&&passed;raw++)for(int channel=0;channel<2&&passed;channel++){if(raw||channel)wprintf(L",");passed=run(device.p,meter.p,channel,raw!=0);Sleep(250);}
 hr=volume->GetMasterVolumeLevelScalar(&after);if(SUCCEEDED(hr))hr=volume->GetMute(&mutedAfter);
 bool unchanged=SUCCEEDED(hr)&&before==after&&mutedBefore==mutedAfter;passed &= unchanged;
 wprintf(L"],\"volume_before\":%.9g,\"volume_after\":%.9g,\"volume_unchanged\":%s,\"passed\":%s}\n",double(before),double(after),unchanged?L"true":L"false",passed?L"true":L"false");
 CoTaskMemFree(id);return passed?0:3;
}
