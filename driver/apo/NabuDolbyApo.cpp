#include "NabuDolbyApo.h"

#include <new>
#include "DolbyStream.h"
#include "DolbySharedTransport.h"
#include "DolbyBank.h"
#include "ApoDiagnostics.h"

namespace {

constexpr GUID PcmFloat = {3, 0, 0x10, {0x80, 0, 0, 0xaa, 0, 0x38, 0x9b, 0x71}};
constexpr UINT32 SampleRate = 48000;
constexpr UINT32 Channels = 2;
constexpr DWORD StereoChannelMask = 3;
volatile LONG Objects;
volatile LONG ServerLocks;

bool stereo_float(IAudioMediaType *type)
{
    UNCOMPRESSEDAUDIOFORMAT format = {};
    return type && SUCCEEDED(type->GetUncompressedAudioFormat(&format)) &&
        format.guidFormatType == PcmFloat && format.dwSamplesPerFrame == Channels &&
        format.dwBytesPerSampleContainer == sizeof(float) && format.dwValidBitsPerSample == 32 &&
        format.fFramesPerSecond == SampleRate &&
        (format.dwChannelMask == StereoChannelMask || format.dwChannelMask == 0);
}

HRESULT supported_format(IAudioMediaType *opposite, IAudioMediaType *requested, IAudioMediaType **result)
{
    if (!result)
        return E_POINTER;
    if (!stereo_float(requested) || (opposite && !stereo_float(opposite)))
        return APOERR_FORMAT_NOT_SUPPORTED;
    requested->AddRef();
    *result = requested;
    return S_OK;
}

bool connection_signature(UINT32 signature)
{
    return signature == APO_CONNECTION_PROPERTY_SIGNATURE || signature == APO_CONNECTION_PROPERTY_V2_SIGNATURE;
}

class NabuDolbyApo final : public IAudioProcessingObject, public IAudioProcessingObjectConfiguration,
                          public IAudioProcessingObjectRT, public IAudioSystemEffects3 {
public:
    NabuDolbyApo(DolbyBlockQueue *requests, DolbyBlockQueue *replies, UINT32 delay) noexcept
        : requests_(requests), replies_(replies), delay_(delay)
    {
        InterlockedIncrement(&Objects);
        nabu_diag(0,0,true); nabu_diag(1,LONG(GetCurrentProcessId()));
    }

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void **result) override
    {
        if (!result)
            return E_POINTER;
        *result = nullptr;
        if (iid == __uuidof(IUnknown) || iid == __uuidof(IAudioProcessingObject))
            *result = static_cast<IAudioProcessingObject *>(this);
        else if (iid == __uuidof(IAudioProcessingObjectRT))
            *result = static_cast<IAudioProcessingObjectRT *>(this);
        else if (iid == __uuidof(IAudioProcessingObjectConfiguration))
            *result = static_cast<IAudioProcessingObjectConfiguration *>(this);
        else if (iid == __uuidof(IAudioSystemEffects))
            *result = static_cast<IAudioSystemEffects *>(this);
        else if (iid == __uuidof(IAudioSystemEffects2))
            *result = static_cast<IAudioSystemEffects2 *>(this);
        else if (iid == __uuidof(IAudioSystemEffects3))
            *result = static_cast<IAudioSystemEffects3 *>(this);
        else
            return E_NOINTERFACE;
        AddRef();
        return S_OK;
    }

    ULONG STDMETHODCALLTYPE AddRef() override { return InterlockedIncrement(&references_); }

    ULONG STDMETHODCALLTYPE Release() override
    {
        ULONG remaining = InterlockedDecrement(&references_);
        if (!remaining) {
            free_delay();
            if (transport_) {
                InterlockedExchange(&transport_->shared->apo_claim, 0);
                nabu_close_transport(transport_);
            }
            this->~NabuDolbyApo();
            HeapFree(GetProcessHeap(), 0, this);
            InterlockedDecrement(&Objects);
        }
        return remaining;
    }

    HRESULT STDMETHODCALLTYPE Initialize(UINT32 bytes, BYTE *data) override
    {
        nabu_diag(2,0,true);nabu_diag(3,LONG(bytes));nabu_diag(16,LONG(GetCurrentProcessId()));
        if (initialized_)
            return APOERR_ALREADY_INITIALIZED;
        if (!data || bytes < sizeof(APOInitBaseStruct))
            return E_INVALIDARG;
        auto *init = reinterpret_cast<APOInitBaseStruct *>(data);
        if (init->cbSize != bytes)
            return E_INVALIDARG;
        if (init->clsid != NabuDolbyApoId)
            return APOERR_INVALID_APO_CLSID;
        if (bytes == sizeof(NabuApoTransportInit)) {
            auto *config = reinterpret_cast<NabuApoTransportInit *>(data);
            if (config->signature != NabuTransportInitSignature ||
                config->name[NabuTransportNameChars - 1] != L'\0')
                return E_INVALIDARG;
            transport_ = nabu_open_transport(config->name);
            if (!transport_)
                return HRESULT_FROM_WIN32(GetLastError());
            if (InterlockedCompareExchange(&transport_->shared->apo_claim, 1, 0)) {
                nabu_close_transport(transport_);
                transport_ = nullptr;
                return HRESULT_FROM_WIN32(ERROR_BUSY);
            }
            requests_ = &transport_->shared->requests;
            replies_ = &transport_->shared->replies;
            delay_ = transport_->shared->delay_frames;
            init_kind_ = 1;
        } else if (bytes == sizeof(APOInitSystemEffects2) || bytes == sizeof(APOInitSystemEffects) || bytes == sizeof(APOInitSystemEffects3)) {
            system_init_ = true;
            init_kind_ = bytes == sizeof(APOInitSystemEffects3) ? 4 : bytes == sizeof(APOInitSystemEffects2) ? 2 : 3;
            bool raw = false, discovering = false;
            IPropertyStore *properties = nullptr;
            bool owns_properties = false;
            constexpr GUID Raw = {0x9e90ea20, 0xb493, 0x4fd1, {0xa1, 0xa8, 0x7e, 0x13, 0x61, 0xa9, 0x56, 0xcf}};
            if (bytes == sizeof(APOInitSystemEffects3)) {
                auto *fx3 = reinterpret_cast<APOInitSystemEffects3 *>(data);
                discovering = fx3->InitializeForDiscoveryOnly != FALSE;
                raw = fx3->AudioProcessingMode == Raw;
                nabu_diag(6,fx3->InitializeForDiscoveryOnly);nabu_diag(7,raw);
                // Windows 11's settings framework uses a context-specific store
                // activated on the last collection device, not the init2 pointer.
                if (!raw && fx3->pDeviceCollection) {
                    UINT count = 0;
                    IMMDevice *endpoint = nullptr;
                    IAudioSystemEffectsPropertyStore *store = nullptr;
                    HRESULT hr = fx3->pDeviceCollection->GetCount(&count);
                    if (SUCCEEDED(hr) && count) hr = fx3->pDeviceCollection->Item(count - 1, &endpoint);
                    if (SUCCEEDED(hr) && endpoint) {
                        PROPVARIANT context = {};
                        context.vt = VT_CLSID;
                        context.puuid = const_cast<GUID *>(&NabuDolbyApoId);
                        hr = endpoint->Activate(__uuidof(IAudioSystemEffectsPropertyStore), CLSCTX_ALL, &context, reinterpret_cast<void **>(&store));
                        if (SUCCEEDED(hr)) hr = store ? store->OpenDefaultPropertyStore(STGM_READ, &properties) : E_NOINTERFACE;
                        owns_properties = properties != nullptr;
                    }
                    nabu_diag(4,hr);
                    if (store) store->Release();
                    if (endpoint) endpoint->Release();
                }
            } else properties = reinterpret_cast<APOInitSystemEffects *>(data)->pAPOSystemEffectsProperties;
            if (bytes == sizeof(APOInitSystemEffects2)) {
                auto *fx2 = reinterpret_cast<APOInitSystemEffects2 *>(data);
                discovering = fx2->InitializeForDiscoveryOnly != FALSE;
                raw = fx2->AudioProcessingMode == Raw;
                nabu_diag(6,fx2->InitializeForDiscoveryOnly);nabu_diag(7,fx2->AudioProcessingMode==Raw);
            }
            if (!raw && (properties || bytes == sizeof(APOInitSystemEffects3))) {
                PROPVARIANT value = {};
                HRESULT hr = properties ? properties->GetValue(PKEY_NabuDolbyBank, &value) : E_NOINTERFACE;
                nabu_diag(4,hr);nabu_diag(5,value.vt);
                if (SUCCEEDED(hr) && value.vt == VT_LPWSTR && value.pwszVal && wcsnlen(value.pwszVal, 128) < 128) {
                    if (discovering) discovery_effect_ = nabu_bank_ready(value.pwszVal);
                    else transport_ = nabu_bank_acquire(value.pwszVal);
                }
                PropVariantClear(&value);
                if (discovering) {
                    if (!discovery_effect_) discovery_effect_ = nabu_bank_ready(L"Global\\NabuDolbyBank-9df8834a-209e-4cc1-bc73-bb4353adfa9f");
                } else if (!transport_) transport_=nabu_bank_acquire(L"Global\\NabuDolbyBank-9df8834a-209e-4cc1-bc73-bb4353adfa9f");
                nabu_diag(17,transport_ ? 0 : LONG(GetLastError()));
            }
            if (owns_properties) properties->Release();
            if (transport_) {
                nabu_diag(8,0,true);
                requests_ = &transport_->shared->requests;
                replies_ = &transport_->shared->replies;
                delay_ = transport_->shared->delay_frames;
            }
            // Missing/occupied broker slots use zero-latency bypass, not a graph error.
        } else if (bytes != sizeof(APOInitBaseStruct)) {
            return E_INVALIDARG;
        }
        if (transport_) {
            InterlockedExchange(&transport_->shared->apo_pid, LONG(GetCurrentProcessId()));
            InterlockedExchange(&transport_->shared->init_kind, init_kind_);
        }
        initialized_ = true;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE Reset() override
    {
        if (!initialized_)
            return APOERR_NOT_INITIALIZED;
        if (delay_frames_)
            reset_stream();
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE GetEffectsList(GUID **effects, UINT *count, HANDLE) override
    {
        if (!effects || !count) return E_POINTER;
        *effects = nullptr; *count = 0;
        if (transport_ || discovery_effect_) {
            *effects = static_cast<GUID *>(CoTaskMemAlloc(sizeof(GUID)));
            if (!*effects) return E_OUTOFMEMORY;
            **effects = NabuDolbyEffectId; *count = 1;
        }
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE GetControllableSystemEffectsList(AUDIO_SYSTEMEFFECT **effects, UINT *count, HANDLE) override
    {
        if (!effects || !count) return E_POINTER;
        *effects = nullptr; *count = 0;
        if (transport_ || discovery_effect_) {
            auto *entry = static_cast<AUDIO_SYSTEMEFFECT *>(CoTaskMemAlloc(sizeof(AUDIO_SYSTEMEFFECT)));
            if (!entry) return E_OUTOFMEMORY;
            *entry = {NabuDolbyEffectId, FALSE, AUDIO_SYSTEMEFFECT_STATE_ON};
            *effects = entry; *count = 1;
        }
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE SetAudioSystemEffectState(GUID id, AUDIO_SYSTEMEFFECT_STATE state) override
    {
        if (id != NabuDolbyEffectId) return E_INVALIDARG;
        // Original immutable profile: do not advertise a toggle we cannot apply.
        const auto actual = transport_ || discovery_effect_ ? AUDIO_SYSTEMEFFECT_STATE_ON : AUDIO_SYSTEMEFFECT_STATE_OFF;
        return state == actual ? S_OK : HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
    }

    HRESULT STDMETHODCALLTYPE GetLatency(HNSTIME *latency) override
    {
        if (!latency)
            return E_POINTER;
        *latency = (HNSTIME(delay_) * 10000000 + SampleRate - 1) / SampleRate;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE GetRegistrationProperties(APO_REG_PROPERTIES **result) override
    {
        if (!result)
            return E_POINTER;
        auto *properties = static_cast<APO_REG_PROPERTIES *>(CoTaskMemAlloc(sizeof(APO_REG_PROPERTIES)));
        *result = properties;
        if (!properties)
            return E_OUTOFMEMORY;
        *properties = {};
        properties->clsid = NabuDolbyApoId;
        properties->Flags = APO_FLAG(APO_FLAG_DEFAULT | APO_FLAG_INPLACE);
        const wchar_t name[] = L"Nabu original Dolby speaker APO";
        memcpy(properties->szFriendlyName, name, sizeof(name));
        properties->u32MajorVersion = 2;
        properties->u32MinInputConnections = properties->u32MaxInputConnections = 1;
        properties->u32MinOutputConnections = properties->u32MaxOutputConnections = 1;
        properties->u32MaxInstances = UINT32_MAX;
        properties->u32NumAPOInterfaces = 1;
        properties->iidAPOInterfaceList[0] = __uuidof(IAudioProcessingObject);
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE IsInputFormatSupported(IAudioMediaType *opposite, IAudioMediaType *requested,
        IAudioMediaType **result) override
    {
        return supported_format(opposite, requested, result);
    }

    HRESULT STDMETHODCALLTYPE IsOutputFormatSupported(IAudioMediaType *opposite, IAudioMediaType *requested,
        IAudioMediaType **result) override
    {
        return supported_format(opposite, requested, result);
    }

    HRESULT STDMETHODCALLTYPE GetInputChannelCount(UINT32 *count) override
    {
        if (!count)
            return E_POINTER;
        *count = Channels;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE LockForProcess(UINT32 input_count, APO_CONNECTION_DESCRIPTOR **inputs,
        UINT32 output_count, APO_CONNECTION_DESCRIPTOR **outputs) override
    {
        nabu_diag(9,0,true);nabu_diag(15,LONG(delay_));
        if (!initialized_)
            return APOERR_NOT_INITIALIZED;
        if (max_frames_)
            return APOERR_APO_LOCKED;
        if (input_count != 1 || output_count != 1)
            return APOERR_NUM_CONNECTIONS_INVALID;
        if (!inputs || !outputs || !inputs[0] || !outputs[0])
            return E_POINTER;
        const auto &input = *inputs[0];
        const auto &output = *outputs[0];
        if (input.u32Signature != APO_CONNECTION_DESCRIPTOR_SIGNATURE ||
            output.u32Signature != APO_CONNECTION_DESCRIPTOR_SIGNATURE ||
            !stereo_float(input.pFormat) || !stereo_float(output.pFormat))
            return APOERR_INVALID_CONNECTION_FORMAT;
        if (!input.u32MaxFrameCount || output.u32MaxFrameCount < input.u32MaxFrameCount)
            return APOERR_INVALID_OUTPUT_MAXFRAMECOUNT;

        if (delay_) {
            if (transport_ && !nabu_pin_transport(transport_))
                return lock_error(GetLastError());
            if (!VirtualLock(this, sizeof(*this)))
                return lock_error(GetLastError());
            self_pinned_ = true;
            delay_frames_ = static_cast<StereoFrame *>(VirtualAlloc(nullptr, delay_bytes(),
                MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
            if (!delay_frames_)
                return E_OUTOFMEMORY;
            if (!VirtualLock(delay_frames_, delay_bytes())) {
                HRESULT error = HRESULT_FROM_WIN32(GetLastError());
                free_delay();
                return error;
            }
            reset_stream();
        }
        max_frames_ = input.u32MaxFrameCount;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE UnlockForProcess() override
    {
        if (!max_frames_)
            return APOERR_ALREADY_UNLOCKED;
        max_frames_ = 0;
        free_delay();
        return S_OK;
    }

    UINT32 STDMETHODCALLTYPE CalcInputFrames(UINT32 outputs) override { return outputs; }
    UINT32 STDMETHODCALLTYPE CalcOutputFrames(UINT32 inputs) override { return inputs; }

    void STDMETHODCALLTYPE APOProcess(UINT32 input_count, APO_CONNECTION_PROPERTY **inputs,
        UINT32 output_count, APO_CONNECTION_PROPERTY **outputs) override
    {
        if (output_count != 1 || !outputs || !outputs[0] || !connection_signature(outputs[0]->u32Signature))
            return;

        const auto input = input_count == 1 && inputs && inputs[0] ? *inputs[0] : APO_CONNECTION_PROPERTY{};
        auto &output = *outputs[0];
        output.u32ValidFrameCount = 0;
        output.u32BufferFlags = BUFFER_INVALID;
        UINT32 count = input.u32ValidFrameCount;
        if (!max_frames_ || !count || count > max_frames_ || !connection_signature(input.u32Signature) ||
            (input.u32BufferFlags != BUFFER_VALID && input.u32BufferFlags != BUFFER_SILENT) ||
            !output.pBuffer || (input.u32BufferFlags == BUFFER_VALID && !input.pBuffer))
            return;

        auto *data = reinterpret_cast<const StereoFrame *>(input.pBuffer);
        auto *result = reinterpret_cast<StereoFrame *>(output.pBuffer);
        output.u32ValidFrameCount = count;
        if (!delay_frames_) {
            if (input.u32BufferFlags == BUFFER_VALID && data != result)
                memmove(result, data, size_t(count) * sizeof(*result));
            output.u32BufferFlags = input.u32BufferFlags;
            return;
        }

        if (input.u32BufferFlags == BUFFER_SILENT) {
            // The output can still contain a delayed tail from an earlier callback.
            memset(result, 0, size_t(count) * sizeof(*result));
            data = result;
        }
        dolby_stream_process(&stream_, data, result, count);
        if (transport_) {
            auto *s = transport_->shared;
            InterlockedIncrement(&s->callbacks);
            InterlockedExchange64(&s->frames_processed, stream_.fallback.next_frame);
            InterlockedExchange64(&s->frames_wet, stream_.fallback.wet_count);
            InterlockedExchange64(&s->frames_dry, stream_.fallback.dry_count);
        }
        output.u32BufferFlags = BUFFER_VALID;
    }

private:
    HRESULT lock_error(DWORD error)
    {
        nabu_diag(10,LONG(error));
        if (transport_) InterlockedExchange(&transport_->shared->lock_error, LONG(error));
        return HRESULT_FROM_WIN32(error);
    }
    size_t delay_bytes() const { return size_t(delay_) * sizeof(StereoFrame); }

    void reset_stream()
    {
        // The epoch belongs to the mapping, not this COM instance. Reopening an
        // instance while its worker is alive must never reuse old queue replies.
        generation_ = transport_ ? UINT32(InterlockedIncrement(&transport_->shared->generation)) : generation_ + 1;
        dolby_stream_init(&stream_, delay_frames_, delay_, requests_, replies_, generation_);
        if (transport_) {
            stream_.requests_dropped = &transport_->shared->requests_dropped;
        }
    }

    void free_delay()
    {
        if (delay_frames_)
            VirtualFree(delay_frames_, 0, MEM_RELEASE);
        delay_frames_ = nullptr;
        if (self_pinned_) {
            VirtualUnlock(this, sizeof(*this));
            self_pinned_ = false;
        }
    }

    volatile LONG references_ = 1;
    bool initialized_ = false;
    UINT32 max_frames_ = 0;
    UINT32 generation_ = 0;
    DolbyBlockQueue *requests_;
    DolbyBlockQueue *replies_;
    UINT32 delay_;
    NabuTransportView *transport_ = nullptr;
    bool self_pinned_ = false;
    bool system_init_ = false;
    bool discovery_effect_ = false;
    LONG init_kind_ = 0;
    StereoFrame *delay_frames_ = nullptr;
    DolbyStream stream_ = {};
};

HRESULT create_apo(REFIID iid, void **result, DolbyBlockQueue *requests = nullptr,
    DolbyBlockQueue *replies = nullptr, UINT32 delay = 0)
{
    if (!result)
        return E_POINTER;
    *result = nullptr;
    void *storage = HeapAlloc(GetProcessHeap(), 0, sizeof(NabuDolbyApo));
    if (!storage)
        return E_OUTOFMEMORY;
    auto *apo = new (storage) NabuDolbyApo(requests, replies, delay);
    HRESULT status = apo->QueryInterface(iid, result);
    apo->Release();
    return status;
}

class Factory final : public IClassFactory {
public:
    Factory() noexcept { InterlockedIncrement(&Objects); }

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void **result) override
    {
        if (!result)
            return E_POINTER;
        *result = nullptr;
        if (iid != __uuidof(IUnknown) && iid != __uuidof(IClassFactory))
            return E_NOINTERFACE;
        *result = static_cast<IClassFactory *>(this);
        AddRef();
        return S_OK;
    }

    ULONG STDMETHODCALLTYPE AddRef() override { return InterlockedIncrement(&references_); }
    ULONG STDMETHODCALLTYPE Release() override
    {
        ULONG remaining = InterlockedDecrement(&references_);
        if (!remaining) {
            this->~Factory();
            HeapFree(GetProcessHeap(), 0, this);
            InterlockedDecrement(&Objects);
        }
        return remaining;
    }

    HRESULT STDMETHODCALLTYPE CreateInstance(IUnknown *outer, REFIID iid, void **result) override
    {
        if (outer)
            return CLASS_E_NOAGGREGATION;
        return create_apo(iid, result);
    }

    HRESULT STDMETHODCALLTYPE LockServer(BOOL lock) override
    {
        InterlockedExchangeAdd(&ServerLocks, lock ? 1 : -1);
        return S_OK;
    }

private:
    volatile LONG references_ = 1;
};

}

STDAPI DllGetClassObject(REFCLSID clsid, REFIID iid, void **result)
{
    if (!result)
        return E_POINTER;
    *result = nullptr;
    if (clsid != NabuDolbyApoId)
        return CLASS_E_CLASSNOTAVAILABLE;
    void *storage = HeapAlloc(GetProcessHeap(), 0, sizeof(Factory));
    if (!storage)
        return E_OUTOFMEMORY;
    auto *factory = new (storage) Factory();
    HRESULT status = factory->QueryInterface(iid, result);
    factory->Release();
    return status;
}

STDAPI DllCanUnloadNow()
{
    return InterlockedCompareExchange(&Objects, 0, 0) || InterlockedCompareExchange(&ServerLocks, 0, 0)
        ? S_FALSE : S_OK;
}

#ifdef NABU_APO_RESEARCH
// Lab-only factory. The harness owns the queues and keeps them resident for the APO lifetime.
extern "C" __declspec(dllexport) HRESULT WINAPI NabuCreateApoForResearch(DolbyBlockQueue *requests,
    DolbyBlockQueue *replies, UINT32 delay, IAudioProcessingObject **result)
{
    if (!requests || !replies || requests == replies || !delay || delay > SampleRate)
        return E_INVALIDARG;
    return create_apo(__uuidof(IAudioProcessingObject), reinterpret_cast<void **>(result), requests, replies, delay);
}
#endif
