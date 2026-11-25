#include "DolbySharedTransport.h"
#include "DolbyBank.h"
#include <cmath>
#include <strsafe.h>

STDAPI DllGetClassObject(REFCLSID, REFIID, void **);
STDAPI DllCanUnloadNow();

namespace {
constexpr GUID FloatPcm = {3, 0, 0x10, {0x80, 0, 0, 0xaa, 0, 0x38, 0x9b, 0x71}};

class FloatMedia final : public IAudioMediaType {
public:
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void **out) override
    {
        if (!out) return E_POINTER;
        *out = nullptr;
        if (iid != __uuidof(IUnknown) && iid != __uuidof(IAudioMediaType)) return E_NOINTERFACE;
        *out = this; AddRef(); return S_OK;
    }
    ULONG STDMETHODCALLTYPE AddRef() override { return ++refs_; }
    ULONG STDMETHODCALLTYPE Release() override { return --refs_; }
    HRESULT STDMETHODCALLTYPE IsCompressedFormat(BOOL *out) override { *out = FALSE; return S_OK; }
    HRESULT STDMETHODCALLTYPE IsEqual(IAudioMediaType *, DWORD *) override { return E_NOTIMPL; }
    const WAVEFORMATEX *STDMETHODCALLTYPE GetAudioFormat() override { return nullptr; }
    HRESULT STDMETHODCALLTYPE GetUncompressedAudioFormat(UNCOMPRESSEDAUDIOFORMAT *out) override
    {
        *out = {FloatPcm, 2, 4, 32, 48000.f, 3}; return S_OK;
    }
private:
    ULONG refs_ = 1;
};

// In-memory Win11 settings objects. These never activate a real audio endpoint.
class SettingsStore final : public IAudioSystemEffectsPropertyStore {
public:
    explicit SettingsStore(IPropertyStore *store) : store_(store) {}
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID id, void **out) override {
        if (!out) return E_POINTER; *out=nullptr;
        if(id!=__uuidof(IUnknown)&&id!=__uuidof(IAudioSystemEffectsPropertyStore))return E_NOINTERFACE;
        *out=this;AddRef();return S_OK;
    }
    ULONG STDMETHODCALLTYPE AddRef() override {return ++refs_;}
    ULONG STDMETHODCALLTYPE Release() override {return --refs_;}
    HRESULT STDMETHODCALLTYPE OpenDefaultPropertyStore(DWORD mode,IPropertyStore **out) override {
        if(!out)return E_POINTER;*out=nullptr;
        if(mode!=STGM_READ)return E_ACCESSDENIED;
        store_->AddRef();*out=store_;return S_OK;
    }
    HRESULT STDMETHODCALLTYPE OpenUserPropertyStore(DWORD,IPropertyStore **) override {return E_NOTIMPL;}
    HRESULT STDMETHODCALLTYPE OpenVolatilePropertyStore(DWORD,IPropertyStore **) override {return E_NOTIMPL;}
    HRESULT STDMETHODCALLTYPE ResetUserPropertyStore() override {return E_NOTIMPL;}
    HRESULT STDMETHODCALLTYPE ResetVolatilePropertyStore() override {return E_NOTIMPL;}
    HRESULT STDMETHODCALLTYPE RegisterPropertyChangeNotification(IAudioSystemEffectsPropertyChangeNotificationClient *) override {return E_NOTIMPL;}
    HRESULT STDMETHODCALLTYPE UnregisterPropertyChangeNotification(IAudioSystemEffectsPropertyChangeNotificationClient *) override {return E_NOTIMPL;}
private: ULONG refs_=1;IPropertyStore *store_;
};
class SettingsEndpoint final : public IMMDevice {
public:
    explicit SettingsEndpoint(SettingsStore *store):store_(store){}
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID id,void **out) override {
        if(!out)return E_POINTER;*out=nullptr;
        if(id!=__uuidof(IUnknown)&&id!=__uuidof(IMMDevice))return E_NOINTERFACE;
        *out=this;AddRef();return S_OK;
    }
    ULONG STDMETHODCALLTYPE AddRef() override {return ++refs_;}
    ULONG STDMETHODCALLTYPE Release() override {return --refs_;}
    HRESULT STDMETHODCALLTYPE Activate(REFIID id,DWORD,PROPVARIANT *context,void **out) override {
        if(!out)return E_POINTER;*out=nullptr;
        if(id!=__uuidof(IAudioSystemEffectsPropertyStore))return E_NOINTERFACE;
        if(!context||context->vt!=VT_CLSID||!context->puuid||*context->puuid!=NabuDolbyApoId)return E_INVALIDARG;
        store_->AddRef();*out=store_;return S_OK;
    }
    HRESULT STDMETHODCALLTYPE OpenPropertyStore(DWORD,IPropertyStore **) override {return E_NOTIMPL;}
    HRESULT STDMETHODCALLTYPE GetId(LPWSTR *) override {return E_NOTIMPL;}
    HRESULT STDMETHODCALLTYPE GetState(DWORD *state) override {if(!state)return E_POINTER;*state=DEVICE_STATE_ACTIVE;return S_OK;}
private: ULONG refs_=1;SettingsStore *store_;
};
class SettingsCollection final : public IMMDeviceCollection {
public:
    explicit SettingsCollection(SettingsEndpoint *endpoint):endpoint_(endpoint){}
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID id,void **out) override {
        if(!out)return E_POINTER;*out=nullptr;
        if(id!=__uuidof(IUnknown)&&id!=__uuidof(IMMDeviceCollection))return E_NOINTERFACE;
        *out=this;AddRef();return S_OK;
    }
    ULONG STDMETHODCALLTYPE AddRef() override {return ++refs_;}
    ULONG STDMETHODCALLTYPE Release() override {return --refs_;}
    HRESULT STDMETHODCALLTYPE GetCount(UINT *count) override {if(!count)return E_POINTER;*count=1;return S_OK;}
    HRESULT STDMETHODCALLTYPE Item(UINT index,IMMDevice **out) override {
        if(!out)return E_POINTER;*out=nullptr;if(index)return E_INVALIDARG;
        endpoint_->AddRef();*out=endpoint_;return S_OK;
    }
private:ULONG refs_=1;SettingsEndpoint *endpoint_;
};

HRESULT make_apo(const WCHAR *name, IAudioProcessingObject **out, UINT32 system_flags = 0)
{
    IClassFactory *factory = nullptr;
    HRESULT status = DllGetClassObject(NabuDolbyApoId, IID_PPV_ARGS(&factory));
    if (FAILED(status)) return status;
    status = factory->CreateInstance(nullptr, IID_PPV_ARGS(out));
    factory->Release();
    if (FAILED(status)) return status;
    if (system_flags) {
        IPropertyStore *store = nullptr;
        HMODULE propsys = LoadLibraryExW(L"propsys.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
        if (!propsys) { (*out)->Release(); *out = nullptr; return HRESULT_FROM_WIN32(GetLastError()); }
        using CreateStore = HRESULT (WINAPI *)(REFIID, void **);
        auto create = reinterpret_cast<CreateStore>(GetProcAddress(propsys, "PSCreateMemoryPropertyStore"));
        status = create ? create(IID_PPV_ARGS(&store)) : E_NOINTERFACE;
        if (SUCCEEDED(status) && name) {
            PROPVARIANT value = {};
            value.vt = VT_LPWSTR;
            value.pwszVal = const_cast<WCHAR *>(name);
            status = store->SetValue(PKEY_NabuDolbyBank, value);
        }
        if (SUCCEEDED(status)) {
            SettingsStore settings(store);SettingsEndpoint endpoint(&settings);SettingsCollection collection(&endpoint);
            if (system_flags & 8) {
                APOInitSystemEffects3 init = {};
                init.APOInit = {sizeof(init),NabuDolbyApoId};
                init.pDeviceCollection = &collection;
                init.InitializeForDiscoveryOnly = (system_flags & 2) != 0;
                if(system_flags & 4)init.AudioProcessingMode={0x9e90ea20,0xb493,0x4fd1,{0xa1,0xa8,0x7e,0x13,0x61,0xa9,0x56,0xcf}};
                status=(*out)->Initialize(sizeof(init),reinterpret_cast<BYTE *>(&init));
            } else {
            APOInitSystemEffects2 init = {};
            init.APOInit = {sizeof(init), NabuDolbyApoId};
            init.pAPOSystemEffectsProperties = store;
            init.InitializeForDiscoveryOnly = (system_flags & 2) != 0;
            if (system_flags & 4) init.AudioProcessingMode = {0x9e90ea20, 0xb493, 0x4fd1, {0xa1, 0xa8, 0x7e, 0x13, 0x61, 0xa9, 0x56, 0xcf}};
            status = (*out)->Initialize(sizeof(init), reinterpret_cast<BYTE *>(&init));
            }
        }
        if (store) store->Release();
        FreeLibrary(propsys);
        if (FAILED(status)) { (*out)->Release(); *out = nullptr; }
        return status;
    }
    NabuApoTransportInit init = {};
    init.base = {sizeof(init), NabuDolbyApoId};
    init.signature = NabuTransportInitSignature;
    if (!name) {
        init.base.cbSize = sizeof(init.base);
        status = (*out)->Initialize(sizeof(init.base), reinterpret_cast<BYTE *>(&init.base));
    } else {
        status = StringCchCopyW(init.name, ARRAYSIZE(init.name), name);
        if (SUCCEEDED(status)) status = (*out)->Initialize(sizeof(init), reinterpret_cast<BYTE *>(&init));
    }
    if (FAILED(status)) { (*out)->Release(); *out = nullptr; }
    return status;
}
}

struct NabuDigitalStats {
    UINT32 bytes;
    UINT32 callbacks;
    UINT32 frames_out;
    UINT32 reset_frame;
    HNSTIME latency;
    double maximum_callback_us;
    double maximum_schedule_late_ms;
    UINT32 invalid_callbacks;
    UINT32 nonfinite_frames;
    UINT32 duplicate_attach_rejected;
    UINT32 dll_unloadable;
};

extern "C" __declspec(dllexport) HRESULT WINAPI NabuTryAttach(const WCHAR *name)
{
    IAudioProcessingObject *apo = nullptr;
    HRESULT status = make_apo(name, &apo);
    if (apo) apo->Release();
    return status;
}

extern "C" __declspec(dllexport) HRESULT WINAPI NabuProbeSystemInit(const WCHAR *bank, UINT32 flags, HNSTIME *latency, UINT32 *effects)
{
    if (!latency || !effects) return E_POINTER;
    IAudioProcessingObject *apo = nullptr;
    HRESULT hr = make_apo(bank, &apo, flags | 1);
    if (FAILED(hr)) return hr;
    hr = apo->GetLatency(latency);
    IAudioSystemEffects2 *fx = nullptr;
    if (SUCCEEDED(hr)) hr = apo->QueryInterface(IID_PPV_ARGS(&fx));
    GUID *ids = nullptr;
    if (SUCCEEDED(hr)) hr = fx->GetEffectsList(&ids, effects, nullptr);
    if (SUCCEEDED(hr) && (flags & 8)) {
        IAudioSystemEffects3 *modern = nullptr;AUDIO_SYSTEMEFFECT *entries=nullptr;UINT count=0;
        hr=apo->QueryInterface(IID_PPV_ARGS(&modern));
        if(SUCCEEDED(hr))hr=modern->GetControllableSystemEffectsList(&entries,&count,nullptr);
        if(SUCCEEDED(hr) && count!=*effects)hr=E_UNEXPECTED;
        if(SUCCEEDED(hr) && count && (!entries[0].canSetState || entries[0].state!=AUDIO_SYSTEMEFFECT_STATE_ON || entries[0].id!=ids[0]))hr=E_UNEXPECTED;
        if(SUCCEEDED(hr) && count) {
            hr=modern->SetAudioSystemEffectState(ids[0],AUDIO_SYSTEMEFFECT_STATE_OFF);
            if(entries)CoTaskMemFree(entries);entries=nullptr;
            if(SUCCEEDED(hr))hr=modern->GetControllableSystemEffectsList(&entries,&count,nullptr);
            if(SUCCEEDED(hr) && (count!=*effects || !entries || !entries[0].canSetState || entries[0].state!=AUDIO_SYSTEMEFFECT_STATE_OFF))hr=E_UNEXPECTED;
            GUID *disabled=nullptr;UINT disabled_count=0;
            if(SUCCEEDED(hr))hr=modern->GetEffectsList(&disabled,&disabled_count,nullptr);
            if(SUCCEEDED(hr) && disabled_count)hr=E_UNEXPECTED;
            if(disabled)CoTaskMemFree(disabled);
            if(SUCCEEDED(hr))hr=modern->SetAudioSystemEffectState(ids[0],AUDIO_SYSTEMEFFECT_STATE_ON);
        }
        if(entries)CoTaskMemFree(entries);
        if(modern)modern->Release();
    }
    if (ids) CoTaskMemFree(ids);
    if (fx) fx->Release();
    apo->Release(); return hr;
}

extern "C" __declspec(dllexport) HRESULT WINAPI NabuRunDigital(const WCHAR *name,
    const StereoFrame *input, UINT32 frames, StereoFrame *output, UINT32 output_capacity,
    UINT32 pattern, UINT32 reset_at, NabuDigitalStats *stats)
{
    if (!input || !output || !stats || stats->bytes != sizeof(*stats) || !frames) return E_INVALIDARG;
    IAudioProcessingObject *apo = nullptr;
    UINT32 system_flags = pattern >> 8;
    HRESULT status = make_apo(name, &apo, system_flags);
    if (FAILED(status)) return status;
    apo->GetLatency(&stats->latency);
    UINT32 delay = UINT32((stats->latency * 48000 + 5000000) / 10000000);
    if (output_capacity < frames + delay || (reset_at && reset_at >= frames)) {
        apo->Release(); return E_INVALIDARG;
    }
    if (name && !system_flags) {
        stats->duplicate_attach_rejected = NabuTryAttach(name) == HRESULT_FROM_WIN32(ERROR_BUSY);
        if (!stats->duplicate_attach_rejected) { apo->Release(); return E_UNEXPECTED; }
    }
    IAudioProcessingObjectConfiguration *configuration = nullptr;
    IAudioProcessingObjectRT *rt = nullptr;
    status = apo->QueryInterface(IID_PPV_ARGS(&configuration));
    if (SUCCEEDED(status)) status = apo->QueryInterface(IID_PPV_ARGS(&rt));
    FloatMedia format;
    APO_CONNECTION_DESCRIPTOR descriptor = {APO_CONNECTION_BUFFER_TYPE_EXTERNAL, 0, 1024,
        &format, APO_CONNECTION_DESCRIPTOR_SIGNATURE};
    APO_CONNECTION_DESCRIPTOR *descriptors[] = {&descriptor};
    bool locked = false;
    if (SUCCEEDED(status)) {
        status = configuration->LockForProcess(1, descriptors, 1, descriptors);
        locked = SUCCEEDED(status);
    }
    if (locked) {
        LARGE_INTEGER frequency, start, now, begin, end;
        QueryPerformanceFrequency(&frequency); QueryPerformanceCounter(&start);
        constexpr UINT32 irregular[] = {137, 343, 512, 256, 672};
        UINT32 cursor = 0;
        bool reset_done = false;
        alignas(16) StereoFrame scratch[1024];
        while (cursor < frames + delay) {
            if (reset_at && cursor == reset_at && !reset_done) {
                status = apo->Reset();
                if (FAILED(status)) break;
                stats->reset_frame = cursor;
                reset_done = true;
            }
            UINT32 count = (pattern & 1) ? irregular[stats->callbacks % ARRAYSIZE(irregular)] : 480;
            if (count > frames + delay - cursor) count = frames + delay - cursor;
            if (reset_at && !reset_done && cursor < reset_at && cursor + count > reset_at) count = reset_at - cursor;
            for (UINT32 i = 0; i < count; ++i) scratch[i] = cursor + i < frames ? input[cursor + i] : StereoFrame{};
            // Processing may be in-place; the APO must preserve the original request data.
            APO_CONNECTION_PROPERTY in = {reinterpret_cast<UINT_PTR>(scratch), count,
                cursor < frames ? BUFFER_VALID : BUFFER_SILENT, APO_CONNECTION_PROPERTY_SIGNATURE};
            APO_CONNECTION_PROPERTY out = {reinterpret_cast<UINT_PTR>(scratch), 0,
                BUFFER_INVALID, APO_CONNECTION_PROPERTY_SIGNATURE};
            APO_CONNECTION_PROPERTY *inputs[] = {&in}, *outputs[] = {&out};
            QueryPerformanceCounter(&begin);
            rt->APOProcess(1, inputs, 1, outputs);
            QueryPerformanceCounter(&end);
            double us = double(end.QuadPart - begin.QuadPart) * 1000000 / frequency.QuadPart;
            if (us > stats->maximum_callback_us) stats->maximum_callback_us = us;
            if (out.u32ValidFrameCount != count || (out.u32BufferFlags != BUFFER_VALID && out.u32BufferFlags != BUFFER_SILENT)) {
                ++stats->invalid_callbacks; status = E_UNEXPECTED; break;
            }
            if (out.u32BufferFlags == BUFFER_SILENT) memset(scratch, 0, size_t(count) * sizeof(*scratch));
            for (UINT32 i = 0; i < count; ++i)
                if (!std::isfinite(scratch[i].left) || !std::isfinite(scratch[i].right)) ++stats->nonfinite_frames;
            memcpy(output + cursor, scratch, size_t(count) * sizeof(*scratch));
            cursor += count; ++stats->callbacks;
            LONGLONG deadline = start.QuadPart + LONGLONG(double(cursor) * frequency.QuadPart / 48000);
            do {
                QueryPerformanceCounter(&now);
                if (now.QuadPart < deadline) Sleep(1);
            } while (now.QuadPart < deadline);
            double late = double(now.QuadPart - deadline) * 1000 / frequency.QuadPart;
            if (late > stats->maximum_schedule_late_ms) stats->maximum_schedule_late_ms = late;
        }
        stats->frames_out = cursor;
        configuration->UnlockForProcess();
    }
    if (rt) rt->Release();
    if (configuration) configuration->Release();
    apo->Release();
    stats->dll_unloadable = DllCanUnloadNow() == S_OK;
    return status;
}
