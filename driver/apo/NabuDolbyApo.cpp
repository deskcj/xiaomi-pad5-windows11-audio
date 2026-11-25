#include "NabuDolbyApo.h"

#include <new>
#include "DolbyStream.h"
#include "DolbySharedTransport.h"
#include "DolbyBank.h"
#include "ApoDiagnostics.h"
#include <ApoBuildIdentity.h>
#include <strsafe.h>

namespace {

constexpr GUID PcmFloat = {3, 0, 0x10, {0x80, 0, 0, 0xaa, 0, 0x38, 0x9b, 0x71}};
constexpr UINT32 SampleRate = 48000;
constexpr UINT32 Channels = 2;
constexpr DWORD StereoChannelMask = 3;
// SID_AudioProcessingObjectLoggingService from the Windows 11 SDK.
constexpr GUID LoggingServiceId={0x8b8008af,0x09f9,0x456e,{0xa1,0x73,0xbd,0xb5,0x84,0x99,0xbc,0xe7}};
volatile LONG Objects;
volatile LONG ServerLocks;

// Factory calls are outside the real-time path. Record their exact contracts
// before Initialize so a mapped DLL is not mistaken for an activated APO.
void factory_event(const WCHAR *stage, REFCLSID clsid, REFIID iid, HRESULT status)
{
    const DWORD error = GetLastError();
    WCHAR class_text[40] = {}, interface_text[40] = {}, message[256] = {};
    if (StringFromGUID2(clsid, class_text, ARRAYSIZE(class_text)) &&
        StringFromGUID2(iid, interface_text, ARRAYSIZE(interface_text)) &&
        SUCCEEDED(StringCchPrintfW(message, ARRAYSIZE(message),
            L"NabuApoFactory stage=%s clsid=%s iid=%s hr=0x%08x pid=%lu",
            stage, class_text, interface_text, UINT32(status), GetCurrentProcessId())))
        nabu_diag_message(message);
    SetLastError(error);
}

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
    *result = nullptr;
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
    class InnerUnknown final : public IUnknown {
    public:
        explicit InnerUnknown(NabuDolbyApo *owner) noexcept : owner_(owner) {}
        HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void **result) override
            { return owner_->NonDelegatingQueryInterface(iid, result); }
        ULONG STDMETHODCALLTYPE AddRef() override { return owner_->NonDelegatingAddRef(); }
        ULONG STDMETHODCALLTYPE Release() override { return owner_->NonDelegatingRelease(); }
    private:
        NabuDolbyApo *owner_;
    };
public:
    NabuDolbyApo(DolbyBlockQueue *requests, DolbyBlockQueue *replies, UINT32 delay,
        IUnknown *outer) noexcept
        : inner_(this), controlling_(outer ? outer : &inner_),
          requests_(requests), replies_(replies), delay_(delay)
    {
        InterlockedIncrement(&Objects);
        nabu_diag(0,0,true); nabu_diag(1,LONG(GetCurrentProcessId()));
    }

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void **result) override
        { return controlling_->QueryInterface(iid, result); }
    ULONG STDMETHODCALLTYPE AddRef() override { return controlling_->AddRef(); }
    ULONG STDMETHODCALLTYPE Release() override { return controlling_->Release(); }

    HRESULT NonDelegatingQueryInterface(REFIID iid, void **result)
    {
        if (!result)
            return E_POINTER;
        *result = nullptr;
        if (iid == __uuidof(IUnknown))
            *result = static_cast<IUnknown *>(&inner_);
        else if (iid == __uuidof(IAudioProcessingObject))
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
        static_cast<IUnknown *>(*result)->AddRef();
        return S_OK;
    }

    ULONG NonDelegatingAddRef() { return InterlockedIncrement(&references_); }

    ULONG NonDelegatingRelease()
    {
        ULONG remaining = InterlockedDecrement(&references_);
        if (!remaining) {
            close_notifications();
            if (logging_) { logging_->Release(); logging_=nullptr; }
            free_delay();
            if (transport_) {
                InterlockedExchange(&transport_->shared->apo_claim, 0);
                nabu_close_transport(transport_);
            }
            this->~NabuDolbyApo();
            VirtualFree(this, 0, MEM_RELEASE);
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
            HRESULT properties_status = E_NOINTERFACE;
            constexpr GUID Raw = {0x9e90ea20, 0xb493, 0x4fd1, {0xa1, 0xa8, 0x7e, 0x13, 0x61, 0xa9, 0x56, 0xcf}};
            if (bytes == sizeof(APOInitSystemEffects3)) {
                auto *fx3 = reinterpret_cast<APOInitSystemEffects3 *>(data);
                discovering = fx3->InitializeForDiscoveryOnly != FALSE;
                raw = fx3->AudioProcessingMode == Raw;
                mode_flags_=(raw?1U:0U)|(discovering?2U:0U);
                if (fx3->pServiceProvider) {
                    logging_status_=fx3->pServiceProvider->QueryService(LoggingServiceId,
                        __uuidof(IAudioProcessingObjectLoggingService),reinterpret_cast<void **>(&logging_));
                    if (FAILED(logging_status_) && logging_) { logging_->Release(); logging_=nullptr; }
                }
                nabu_diag(6,fx3->InitializeForDiscoveryOnly);nabu_diag(7,raw);
                // Windows 11's settings framework uses a context-specific store
                // activated on the last collection device, not the init2 pointer.
                if (!raw && fx3->pDeviceCollection) {
                    UINT count = 0;
                    IMMDevice *endpoint = nullptr;
                    IAudioSystemEffectsPropertyStore *store = nullptr;
                    HRESULT hr = fx3->pDeviceCollection->GetCount(&count);
                    if (SUCCEEDED(hr)) hr = count ? fx3->pDeviceCollection->Item(count - 1, &endpoint) : HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
                    if (SUCCEEDED(hr) && !endpoint) hr = E_NOINTERFACE;
                    if (SUCCEEDED(hr) && endpoint) {
                        PROPVARIANT context = {};
                        context.vt = VT_CLSID;
                        context.puuid = const_cast<GUID *>(&NabuDolbyApoId);
                        hr = endpoint->Activate(__uuidof(IAudioSystemEffectsPropertyStore), CLSCTX_ALL, &context, reinterpret_cast<void **>(&store));
                        if (SUCCEEDED(hr)) hr = store ? store->OpenDefaultPropertyStore(STGM_READ, &properties) : E_NOINTERFACE;
                        owns_properties = properties != nullptr;
                        if (SUCCEEDED(hr) && !properties) hr = E_NOINTERFACE;
                    }
                    properties_status = hr;
                    if (store) store->Release();
                    if (endpoint) endpoint->Release();
                }
            } else {
                properties = reinterpret_cast<APOInitSystemEffects *>(data)->pAPOSystemEffectsProperties;
                properties_status = properties ? S_OK : E_NOINTERFACE;
            }
            if (bytes == sizeof(APOInitSystemEffects2)) {
                auto *fx2 = reinterpret_cast<APOInitSystemEffects2 *>(data);
                discovering = fx2->InitializeForDiscoveryOnly != FALSE;
                raw = fx2->AudioProcessingMode == Raw;
                mode_flags_=(raw?1U:0U)|(discovering?2U:0U);
                nabu_diag(6,fx2->InitializeForDiscoveryOnly);nabu_diag(7,fx2->AudioProcessingMode==Raw);
            }
            if (!raw) {
                PROPVARIANT value = {};
                HRESULT hr = SUCCEEDED(properties_status) && properties ? properties->GetValue(PKEY_NabuDolbyBank, &value) : properties_status;
                nabu_diag(4,hr);nabu_diag(5,value.vt);
                // An explicit property belongs to this endpoint/context. Do not
                // substitute another bank when it is invalid, missing or busy.
                // Only a successful VT_EMPTY read means absence. An inaccessible
                // context or failed read must not select another context's bank.
                const bool bank_absent = SUCCEEDED(hr) && value.vt == VT_EMPTY;
                if (SUCCEEDED(hr) && value.vt == VT_LPWSTR && value.pwszVal && wcsnlen(value.pwszVal, 128) < 128) {
                    if (discovering) discovery_effect_ = nabu_bank_ready(value.pwszVal);
                    else transport_ = nabu_bank_acquire(value.pwszVal);
                }
                PropVariantClear(&value);
                if (discovering && bank_absent) {
                    if (!discovery_effect_) discovery_effect_ = nabu_bank_ready(L"Global\\NabuDolbyBank-9df8834a-209e-4cc1-bc73-bb4353adfa9f");
                } else if (!discovering && bank_absent && !transport_) {
                    transport_=nabu_bank_acquire(L"Global\\NabuDolbyBank-9df8834a-209e-4cc1-bc73-bb4353adfa9f");
                }
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
            shared_ = transport_->shared;
            InterlockedExchange(&shared_->effect_enabled, InterlockedCompareExchange(&effect_enabled_,0,0));
            InterlockedExchange(&shared_->apo_pid, LONG(GetCurrentProcessId()));
            InterlockedExchange(&shared_->init_kind, init_kind_);
        }
        initialized_ = true;
        log_event(L"Initialize",S_OK);
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

    HRESULT STDMETHODCALLTYPE GetEffectsList(GUID **effects, UINT *count, HANDLE event) override
    {
        if (!effects || !count) return E_POINTER;
        *effects = nullptr; *count = 0;
        if (has_effect()) {
            *effects = static_cast<GUID *>(CoTaskMemAlloc(sizeof(GUID)));
            if (!*effects) return E_OUTOFMEMORY;
            **effects = NabuDolbyEffectId; *count = 1;
        }
        HRESULT status = subscribe_event(event,false);
        if (FAILED(status)) { CoTaskMemFree(*effects);*effects=nullptr;*count=0; }
        else if (!InterlockedCompareExchange(&effect_enabled_,0,0)) { CoTaskMemFree(*effects);*effects=nullptr;*count=0; }
        return status;
    }

    HRESULT STDMETHODCALLTYPE GetControllableSystemEffectsList(AUDIO_SYSTEMEFFECT **effects, UINT *count, HANDLE event) override
    {
        if (!effects || !count) return E_POINTER;
        *effects = nullptr; *count = 0;
        if (has_effect()) {
            auto *entry = static_cast<AUDIO_SYSTEMEFFECT *>(CoTaskMemAlloc(sizeof(AUDIO_SYSTEMEFFECT)));
            if (!entry) return E_OUTOFMEMORY;
            *entry = {NabuDolbyEffectId, TRUE, AUDIO_SYSTEMEFFECT_STATE_OFF};
            *effects = entry; *count = 1;
        }
        HRESULT status = subscribe_event(event,true);
        if (FAILED(status)) { CoTaskMemFree(*effects);*effects=nullptr;*count=0; }
        else if (*effects) (*effects)->state = InterlockedCompareExchange(&effect_enabled_,0,0) ? AUDIO_SYSTEMEFFECT_STATE_ON : AUDIO_SYSTEMEFFECT_STATE_OFF;
        return status;
    }

    HRESULT STDMETHODCALLTYPE SetAudioSystemEffectState(GUID id, AUDIO_SYSTEMEFFECT_STATE state) override
    {
        if (id != NabuDolbyEffectId || (state != AUDIO_SYSTEMEFFECT_STATE_ON && state != AUDIO_SYSTEMEFFECT_STATE_OFF)) return E_INVALIDARG;
        if (!initialized_) return APOERR_NOT_INITIALIZED;
        if (!has_effect() && state == AUDIO_SYSTEMEFFECT_STATE_ON) return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
        const LONG enabled = state == AUDIO_SYSTEMEFFECT_STATE_ON ? 1 : 0;
        LONG previous = InterlockedExchange(&effect_enabled_,enabled);
        if (shared_) InterlockedExchange(&shared_->effect_enabled,enabled);
        if (previous != enabled) {
            AcquireSRWLockShared(&notification_lock_);
            if (legacy_event_) SetEvent(legacy_event_);
            if (control_event_) SetEvent(control_event_);
            ReleaseSRWLockShared(&notification_lock_);
        }
        return S_OK;
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
            return lock_result(APOERR_NOT_INITIALIZED);
        if (max_frames_)
            return lock_result(APOERR_APO_LOCKED);
        if (input_count != 1 || output_count != 1)
            return lock_result(APOERR_NUM_CONNECTIONS_INVALID);
        if (!inputs || !outputs || !inputs[0] || !outputs[0])
            return lock_result(E_POINTER);
        const auto &input = *inputs[0];
        const auto &output = *outputs[0];
        if (input.u32Signature != APO_CONNECTION_DESCRIPTOR_SIGNATURE ||
            output.u32Signature != APO_CONNECTION_DESCRIPTOR_SIGNATURE ||
            !stereo_float(input.pFormat) || !stereo_float(output.pFormat))
            return lock_result(APOERR_INVALID_CONNECTION_FORMAT);
        if (!input.u32MaxFrameCount || output.u32MaxFrameCount < input.u32MaxFrameCount)
            return lock_result(APOERR_INVALID_OUTPUT_MAXFRAMECOUNT);

        if (delay_) {
            if (transport_ && !nabu_pin_transport(transport_))
                return failed_lock(lock_error(GetLastError()));
            if (!VirtualLock(this, sizeof(*this)))
                return failed_lock(lock_error(GetLastError()));
            self_pinned_ = true;
            delay_frames_ = static_cast<StereoFrame *>(VirtualAlloc(nullptr, delay_bytes(),
                MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
            if (!delay_frames_)
                return failed_lock(E_OUTOFMEMORY);
            if (!VirtualLock(delay_frames_, delay_bytes())) {
                HRESULT error = HRESULT_FROM_WIN32(GetLastError());
                return failed_lock(error);
            }
            reset_stream();
        }
        max_frames_ = input.u32MaxFrameCount;
        if (shared_) InterlockedExchange(&shared_->lock_error, 0);
        return lock_result(S_OK);
    }

    HRESULT STDMETHODCALLTYPE UnlockForProcess() override
    {
        if (!max_frames_)
            return APOERR_ALREADY_UNLOCKED;
        max_frames_ = 0;
        free_delay();
        nabu_unpin_transport(transport_);
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
        dolby_stream_process_effect(&stream_, data, result, count, InterlockedCompareExchange(&effect_enabled_,0,0) != 0);
        if (shared_) {
            // The view wrapper lives on the pageable heap. Only access the
            // cached mapping here; LockForProcess pins it and this object.
            auto *s = shared_;
            InterlockedIncrement(&s->callbacks);
            if (UINT64(InterlockedCompareExchange64(&s->frames_dry, 0, 0)) != stream_.fallback.dry_count)
                nabu_publish_dry_history(s, stream_.fallback);
            InterlockedExchange64(&s->frames_processed, stream_.fallback.next_frame);
            InterlockedExchange64(&s->frames_wet, stream_.fallback.wet_count);
            InterlockedExchange64(&s->frames_dry, stream_.fallback.dry_count);
            InterlockedExchange64(&s->frames_bypassed, stream_.fallback.bypass_count);
            InterlockedExchange64(&s->first_dry_frame, LONG64(stream_.fallback.first_dry_frame));
        }
        output.u32BufferFlags = BUFFER_VALID;
    }

private:
    bool has_effect() const { return transport_ || discovery_effect_ || (requests_ && replies_ && delay_); }

    HRESULT subscribe_event(HANDLE event, bool controllable)
    {
        HANDLE owned = nullptr;
        if (event && !DuplicateHandle(GetCurrentProcess(),event,GetCurrentProcess(),&owned,0,FALSE,DUPLICATE_SAME_ACCESS))
            return HRESULT_FROM_WIN32(GetLastError());
        AcquireSRWLockExclusive(&notification_lock_);
        HANDLE &slot = controllable ? control_event_ : legacy_event_;
        HANDLE previous = slot;slot=owned;
        ReleaseSRWLockExclusive(&notification_lock_);
        if (previous) CloseHandle(previous);
        return S_OK;
    }

    void close_notifications()
    {
        AcquireSRWLockExclusive(&notification_lock_);
        if (legacy_event_) CloseHandle(legacy_event_);
        if (control_event_) CloseHandle(control_event_);
        legacy_event_=control_event_=nullptr;
        ReleaseSRWLockExclusive(&notification_lock_);
    }

    void log_event(const WCHAR *phase,HRESULT status)
    {
        DWORD error=GetLastError();
        WCHAR message[512]={};
        if (SUCCEEDED(StringCchPrintfW(message,ARRAYSIZE(message),
                L"NabuApo build=%ls source=%ls phase=%ls hr=0x%08lx pid=%lu init=%ld mode=%u transport=%u delay=%u logger=0x%08lx queue=%ls epoch=%u",
                NabuApoBuildTag,NabuApoSourceFingerprint,phase,status,GetCurrentProcessId(),
                init_kind_,mode_flags_,transport_?1U:0U,delay_,logging_status_,
                transport_?transport_->name:L"-",generation_))) {
            nabu_diag_message(message);
            if (logging_) logging_->ApoLog(FAILED(status)?APO_LOG_LEVEL_ERROR:APO_LOG_LEVEL_INFO,L"%ls",message);
        }
        SetLastError(error);
    }

    HRESULT lock_result(HRESULT status)
    {
        log_event(L"LockForProcess",status);
        return status;
    }

    HRESULT failed_lock(HRESULT status)
    {
        free_delay();
        nabu_unpin_transport(transport_);
        return lock_result(status);
    }

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
        generation_ = shared_ ? UINT32(InterlockedIncrement(&shared_->generation)) : generation_ + 1;
        dolby_stream_init(&stream_, delay_frames_, delay_, requests_, replies_, generation_);
        stream_.fallback.wet_mix = InterlockedCompareExchange(&effect_enabled_,0,0) ? DOLBY_EFFECT_RAMP_FRAMES : 0;
        if (shared_) {
            stream_.requests_dropped = &shared_->requests_dropped;
            // Frame telemetry belongs to the new epoch, not the previous owner.
            InterlockedExchange(&shared_->callbacks,0);
            InterlockedExchange64(&shared_->frames_processed,0);
            InterlockedExchange64(&shared_->frames_wet,0);
            InterlockedExchange64(&shared_->frames_dry,0);
            InterlockedExchange64(&shared_->frames_bypassed,0);
            InterlockedExchange64(&shared_->first_dry_frame,-1);
            nabu_publish_dry_history(shared_, stream_.fallback);
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

    InnerUnknown inner_;
    // The outer owns the inner IUnknown; taking a strong outer reference here
    // would create a cycle. Public interfaces delegate identity and references.
    IUnknown *controlling_;
    volatile LONG references_ = 1;
    volatile LONG effect_enabled_ = 1;
    SRWLOCK notification_lock_ = SRWLOCK_INIT;
    HANDLE legacy_event_ = nullptr, control_event_ = nullptr;
    bool initialized_ = false;
    UINT32 max_frames_ = 0;
    UINT32 generation_ = 0;
    DolbyBlockQueue *requests_;
    DolbyBlockQueue *replies_;
    UINT32 delay_;
    NabuTransportView *transport_ = nullptr;
    NabuSharedTransport *shared_ = nullptr;
    bool self_pinned_ = false;
    bool system_init_ = false;
    bool discovery_effect_ = false;
    LONG init_kind_ = 0;
    UINT32 mode_flags_=0;
    HRESULT logging_status_=E_NOINTERFACE;
    IAudioProcessingObjectLoggingService *logging_=nullptr;
    StereoFrame *delay_frames_ = nullptr;
    DolbyStream stream_ = {};
};

HRESULT create_apo(REFIID iid, void **result, DolbyBlockQueue *requests = nullptr,
    DolbyBlockQueue *replies = nullptr, UINT32 delay = 0, IUnknown *outer = nullptr)
{
    if (!result)
        return E_POINTER;
    *result = nullptr;
    // VirtualLock/VirtualUnlock operate on entire pages, without regard to
    // heap-object boundaries. Give every real-time object its own allocation
    // so unlocking it cannot also unlock another instance's state.
    void *storage = VirtualAlloc(nullptr, sizeof(NabuDolbyApo),
        MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    if (!storage)
        return E_OUTOFMEMORY;
    auto *apo = new (storage) NabuDolbyApo(requests, replies, delay, outer);
    HRESULT status = apo->NonDelegatingQueryInterface(iid, result);
    apo->NonDelegatingRelease();
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
        nabu_diag(21, 0, true);
        nabu_diag(22, LONG(GetCurrentProcessId()));
        nabu_diag(24, outer ? 1 : 0);
        factory_event(L"CreateInstanceEnter", NabuDolbyApoId, iid, S_OK);
        HRESULT status;
        if (!result)
            status = E_POINTER;
        else {
            *result = nullptr;
            status = outer && iid != __uuidof(IUnknown) ? CLASS_E_NOAGGREGATION :
                create_apo(iid, result, nullptr, nullptr, 0, outer);
        }
        nabu_diag(23, LONG(status));
        factory_event(L"CreateInstanceReturn", NabuDolbyApoId, iid, status);
        return status;
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
    nabu_diag(18, 0, true);
    nabu_diag(19, LONG(GetCurrentProcessId()));
    factory_event(L"DllGetClassObjectEnter", clsid, iid, S_OK);
    HRESULT status;
    if (!result)
        status = E_POINTER;
    else {
        *result = nullptr;
        if (clsid != NabuDolbyApoId)
            status = CLASS_E_CLASSNOTAVAILABLE;
        else {
            void *storage = HeapAlloc(GetProcessHeap(), 0, sizeof(Factory));
            if (!storage)
                status = E_OUTOFMEMORY;
            else {
                auto *factory = new (storage) Factory();
                status = factory->QueryInterface(iid, result);
                factory->Release();
            }
        }
    }
    nabu_diag(20, LONG(status));
    factory_event(L"DllGetClassObjectReturn", clsid, iid, status);
    return status;
}

STDAPI DllCanUnloadNow()
{
    return InterlockedCompareExchange(&Objects, 0, 0) || InterlockedCompareExchange(&ServerLocks, 0, 0)
        ? S_FALSE : S_OK;
}

extern "C" __declspec(dllexport) const WCHAR *WINAPI NabuApoBuildFingerprint()
{
    return NabuApoSourceFingerprint;
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
