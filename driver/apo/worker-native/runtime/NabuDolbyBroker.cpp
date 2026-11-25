// User-mode bank/worker supervisor. Opens no audio device and installs nothing.
#include <windows.h>
#include <sddl.h>
#include <strsafe.h>
#include <string>
#include <vector>
#include <sstream>
#include <stdexcept>
#include <stdio.h>
namespace
{
HANDLE cancelEvent = nullptr;
BOOL WINAPI cancel(DWORD kind)
{
    if (kind == CTRL_C_EVENT || kind == CTRL_BREAK_EVENT)
    {
        SetEvent(cancelEvent);
        return TRUE;
    }
    return FALSE;
}
struct Handle
{
    HANDLE h = nullptr;
    ~Handle()
    {
        if (h && h != INVALID_HANDLE_VALUE)
            CloseHandle(h);
    }
    Handle() = default;
    Handle(const Handle &) = delete;
    Handle &operator=(const Handle &) = delete;
};
struct Module
{
    HMODULE h = nullptr;
    ~Module()
    {
        if (h)
            FreeLibrary(h);
    }
};
std::string json(const std::wstring &s)
{
    std::string out = "\"";
    char text[8];
    for (WCHAR c : s)
    {
        if (c == '"' || c == '\\')
        {
            out += '\\';
            out += char(c);
        }
        else if (c < 32 || c > 126)
        {
            sprintf_s(text, "\\u%04x", unsigned(c));
            out += text;
        }
        else
            out += char(c);
    }
    return out + '"';
}
std::wstring quote(const std::wstring &s)
{
    std::wstring out = L"\"";
    size_t slashes = 0;
    for (WCHAR c : s)
    {
        if (c == '\\')
        {
            ++slashes;
            continue;
        }
        if (c == '"')
            out.append(slashes * 2 + 1, L'\\');
        else
            out.append(slashes, L'\\');
        slashes = 0;
        out += c;
    }
    out.append(slashes * 2, L'\\');
    return out + L'"';
}
std::wstring full(const WCHAR *s)
{
    WCHAR path[32768];
    DWORD n = GetFullPathNameW(s, ARRAYSIZE(path), path, nullptr);
    if (!n || n >= ARRAYSIZE(path))
        throw DWORD(ERROR_INVALID_NAME);
    return path;
}
void file(const std::wstring &p)
{
    DWORD a = GetFileAttributesW(p.c_str());
    if (a == INVALID_FILE_ATTRIBUTES || (a & FILE_ATTRIBUTE_DIRECTORY))
        throw DWORD(ERROR_FILE_NOT_FOUND);
}
unsigned number(const WCHAR *s, unsigned limit)
{
    if (!s || !*s)
        throw DWORD(ERROR_INVALID_PARAMETER);
    unsigned n = 0;
    for (; *s; ++s)
    {
        if (*s < L'0' || *s > L'9' ||
            (n > limit / 10 || (n == limit / 10 && unsigned(*s - L'0') > limit % 10)))
            throw DWORD(ERROR_INVALID_PARAMETER);
        n = n * 10 + unsigned(*s - L'0');
    }
    if (n > limit)
        throw DWORD(ERROR_INVALID_PARAMETER);
    return n;
}
std::wstring ownerSddl()
{
    Handle token;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token.h))
        throw GetLastError();
    DWORD size = 0;
    GetTokenInformation(token.h, TokenUser, nullptr, 0, &size);
    if (!size || size > 65536)
        throw DWORD(ERROR_INVALID_DATA);
    std::vector<BYTE> data(size);
    if (!GetTokenInformation(token.h, TokenUser, data.data(), size, &size))
        throw GetLastError();
    LPWSTR sid = nullptr;
    if (!ConvertSidToStringSidW(reinterpret_cast<TOKEN_USER *>(data.data())->User.Sid, &sid))
        throw GetLastError();
    std::wstring result = L"D:P(A;;GA;;;SY)(A;;GA;;;LS)(A;;GA;;;";
    result += sid;
    result += L")S:(ML;;NW;;;LW)";
    LocalFree(sid);
    return result;
}
struct Api
{
    Module dll;
    using Create = void *(WINAPI *)(const WCHAR *, UINT32, const WCHAR *);
    using Bank = void *(WINAPI *)(const WCHAR *, const WCHAR *, UINT32, const WCHAR *);
    using Close = void(WINAPI *)(void *);
    using Metric = LONG(WINAPI *)(void *, UINT32);
    using Frames = LONG64(WINAPI *)(void *, UINT32);
    struct Range { UINT64 first, last; };
    using Ranges = BOOL(WINAPI *)(void *, Range *, UINT32, UINT64 *);
    using Reclaim = BOOL(WINAPI *)(void *, HANDLE);
    Create create = nullptr;
    Bank bank = nullptr;
    Close close = nullptr, bankClose = nullptr, stop = nullptr;
    Metric metric = nullptr;
    Frames frames = nullptr;
    Ranges ranges = nullptr;
    Reclaim reclaim = nullptr;
    template <class T> T get(const char *name)
    {
        auto p = GetProcAddress(dll.h, name);
        if (!p)
            throw DWORD(ERROR_PROC_NOT_FOUND);
        return reinterpret_cast<T>(p);
    }
    void load(const std::wstring &path, const std::wstring &fingerprint)
    {
        dll.h = LoadLibraryExW(path.c_str(), nullptr,
                               LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_SYSTEM32);
        if (!dll.h)
            throw GetLastError();
        auto id = get<const WCHAR *(WINAPI *)()>("NabuApoBuildFingerprint");
        if (!id() || fingerprint != id())
            throw DWORD(ERROR_REVISION_MISMATCH);
        create = get<Create>("NabuTransportCreateSecured");
        bank = get<Bank>("NabuBankCreate");
        close = get<Close>("NabuTransportClose");
        bankClose = get<Close>("NabuBankClose");
        stop = get<Close>("NabuTransportStop");
        metric = get<Metric>("NabuTransportMetric");
        frames = get<Frames>("NabuTransportFrames");
        ranges = get<Ranges>("NabuTransportDryRanges");
        reclaim = get<Reclaim>("NabuSupervisorReleaseExitedWorker");
    }
};
struct Worker
{
    Handle process, log;
    void *view = nullptr;
    DWORD pid = 0, exit = STILL_ACTIVE;
    DWORD previousExit = 0;
    unsigned restarts = 0;
    bool recovering = false;
    ULONGLONG launched = 0;
    std::wstring name, output;
};
class Broker
{
    Api api_;
    Handle job_, input_;
    std::vector<Worker> workers_;
    void *bank_ = nullptr;
    std::wstring output_, bankName_, fingerprint_;
    std::wstring dllPath_, exePath_, scriptPath_;
    bool native_ = false;
    unsigned seconds_ = 0;
    bool published_ = false, forced_ = false;
    DWORD error_ = 0, reportError_ = 0;
    UINT64 skippedReports_ = 0;
    void write(const char *phase)
    {
        std::ostringstream data;
        data << "{\"state\":" << json(std::wstring(phase, phase + strlen(phase))) << ",\"error\":" << error_
             << ",\"broker_pid\":" << GetCurrentProcessId()
             << ",\"audio_opened\":false,\"drivers_changed\":false,\"bank_published\":"
             << (published_ ? "true" : "false")
             << ",\"forced_owned_job_termination\":" << (forced_ ? "true" : "false")
             << ",\"duration_seconds\":" << seconds_ << ",\"bank\":" << json(bankName_)
             << ",\"recovery_mode\":\"preserve_active_stream\",\"restart_limit_per_slot\":2"
             << ",\"source_fingerprint\":" << json(fingerprint_) << ",\"workers\":[";
        for (size_t i = 0; i < workers_.size(); ++i)
        {
            auto &w = workers_[i];
            if (i)
                data << ',';
            data << "{\"pid\":" << w.pid << ",\"exit_code\":" << w.exit << ",\"transport\":" << json(w.name)
                 << ",\"restart_count\":" << w.restarts << ",\"previous_exit_code\":" << w.previousExit
                 << ",\"recovering\":" << (w.recovering ? "true" : "false")
                 << ",\"output\":" << json(w.output)
                 << ",\"ready\":" << (w.view && api_.metric(w.view, 7) ? "true" : "false");
            if (w.view)
                data << ",\"apo_claim\":" << api_.metric(w.view, 0)
                     << ",\"worker_claim\":" << api_.metric(w.view, 1)
                     << ",\"stop\":" << api_.metric(w.view, 3) << ",\"init_kind\":" << api_.metric(w.view, 9)
                     << ",\"lock_error\":" << api_.metric(w.view, 10)
                     << ",\"apo_pid\":" << api_.metric(w.view, 8)
                     << ",\"generation\":" << api_.metric(w.view, 2)
                     << ",\"callbacks\":" << api_.metric(w.view, 11)
                     << ",\"requests_dropped\":" << api_.metric(w.view, 4)
                     << ",\"worker_gaps\":" << api_.metric(w.view, 5)
                     << ",\"blocks_replied\":" << api_.metric(w.view, 6)
                     << ",\"frames_processed\":" << api_.frames(w.view, 0)
                     << ",\"frames_wet\":" << api_.frames(w.view, 1)
                     << ",\"frames_dry\":" << api_.frames(w.view, 2)
                     << ",\"frames_bypassed\":" << api_.frames(w.view, 6)
                     << ",\"effect_enabled\":" << api_.metric(w.view, 12)
                     << ",\"first_dry_frame\":" << api_.frames(w.view, 3)
                     << ",\"last_dry_frame\":" << api_.frames(w.view, 4);
            if (w.view) {
                Api::Range ranges[16];UINT64 runs = 0;
                BOOL available = api_.ranges(w.view, ranges, 16, &runs);
                data << ",\"dry_ranges_available\":" << (available ? "true" : "false")
                     << ",\"dry_runs\":" << runs << ",\"dry_ranges\":[";
                for (unsigned j = 0; available && j < (runs < 16 ? runs : 16); ++j) {
                    if (j) data << ',';
                    data << "{\"first\":" << ranges[j].first << ",\"last\":" << ranges[j].last << '}';
                }
                data << ']';
            }
            data << '}';
        }
        data << "],\"telemetry_write_failures\":" << skippedReports_
             << ",\"last_telemetry_error\":" << reportError_ << "}\n";
        const std::string content = data.str();
        Handle f;
        const auto temp = output_ + L"\\Broker.json.tmp", dest = output_ + L"\\Broker.json";
        f.h = CreateFileW(temp.c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS,
                          FILE_ATTRIBUTE_NORMAL, nullptr);
        if (f.h == INVALID_HANDLE_VALUE)
            throw GetLastError();
        DWORD wrote = 0;
        if (!WriteFile(f.h, content.data(), DWORD(content.size()), &wrote, nullptr) ||
            wrote != content.size() || !FlushFileBuffers(f.h))
            throw GetLastError();
        CloseHandle(f.h);
        f.h = nullptr;
        for (unsigned attempt = 0;; ++attempt)
        {
            if (MoveFileExW(temp.c_str(), dest.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
                break;
            DWORD e = GetLastError();
            if (attempt >= 3 || (e != ERROR_SHARING_VIOLATION && e != ERROR_ACCESS_DENIED))
                throw e;
            Sleep(10);
        }
    }
    void snapshot(const char *phase) noexcept
    {
        try
        {
            write(phase);
        }
        catch (DWORD e)
        {
            reportError_ = e;
            ++skippedReports_;
        }
        catch (...)
        {
            reportError_ = ERROR_UNHANDLED_EXCEPTION;
            ++skippedReports_;
        }
    }
    void exited()
    {
        for (auto &w : workers_)
            if (w.process.h && WaitForSingleObject(w.process.h, 0) == WAIT_OBJECT_0)
            {
                GetExitCodeProcess(w.process.h, &w.exit);
                throw DWORD(ERROR_BROKEN_PIPE);
            }
    }
    void startWorker(Worker &w, unsigned index)
    {
        if (w.process.h)
        {
            if (WaitForSingleObject(w.process.h, 0) != WAIT_OBJECT_0)
                throw DWORD(ERROR_BUSY);
            CloseHandle(w.process.h);
            w.process.h = nullptr;
        }
        if (w.log.h)
        {
            CloseHandle(w.log.h);
            w.log.h = nullptr;
        }
        std::wstring stem = L"worker" + std::to_wstring(index);
        if (w.restarts)
            stem += L"-attempt" + std::to_wstring(w.restarts);
        w.output = output_ + L"\\" + stem;
        SECURITY_ATTRIBUTES inheritable = {sizeof(inheritable), nullptr, TRUE};
        w.log.h = CreateFileW((output_ + L"\\" + stem + L".console").c_str(), GENERIC_WRITE, FILE_SHARE_READ,
                              &inheritable, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (w.log.h == INVALID_HANDLE_VALUE)
            throw GetLastError();
        std::wstring command = quote(exePath_) + L" ";
        if (!native_)
            command += quote(scriptPath_) + L" ";
        command += quote(w.name) + L" " + quote(w.output) + L" " + quote(dllPath_);
        std::vector<WCHAR> mutableCommand(command.begin(), command.end());
        mutableCommand.push_back(0);
        STARTUPINFOW startup = {};
        startup.cb = sizeof(startup);
        startup.dwFlags = STARTF_USESTDHANDLES;
        startup.hStdInput = input_.h;
        startup.hStdOutput = startup.hStdError = w.log.h;
        PROCESS_INFORMATION process = {};
        if (!CreateProcessW(exePath_.c_str(), mutableCommand.data(), nullptr, nullptr, TRUE,
                            CREATE_SUSPENDED | CREATE_NO_WINDOW, nullptr, output_.c_str(), &startup,
                            &process))
            throw GetLastError();
        w.process.h = process.hProcess;
        w.pid = process.dwProcessId;
        w.exit = STILL_ACTIVE;
        Handle thread;
        thread.h = process.hThread;
        if (!AssignProcessToJobObject(job_.h, w.process.h))
        {
            DWORD error = GetLastError();
            TerminateProcess(w.process.h, error);
            WaitForSingleObject(w.process.h, 2000);
            throw error;
        }
        if (ResumeThread(thread.h) == DWORD(-1))
            throw GetLastError();
        w.launched = GetTickCount64();
    }
    bool recoverExitedWorkers()
    {
        bool pending = false;
        for (unsigned index = 0; index < workers_.size(); ++index)
        {
            auto &w = workers_[index];
            const DWORD waited = WaitForSingleObject(w.process.h, 0);
            if (waited == WAIT_FAILED)
                throw GetLastError();
            if (waited == WAIT_OBJECT_0)
            {
                GetExitCodeProcess(w.process.h, &w.exit);
                w.previousExit = w.exit;
                w.recovering = true;
                if (w.restarts >= 2)
                    throw DWORD(ERROR_BROKEN_PIPE);
                if (!api_.reclaim(w.view, w.process.h))
                {
                    const DWORD error = GetLastError();
                    if (error != ERROR_BUSY)
                        throw error;
                }
                else
                {
                    ++w.restarts;
                    startWorker(w, index);
                }
                pending = true;
            }
            else if (w.recovering)
            {
                if (api_.metric(w.view, 7))
                    w.recovering = false;
                else if (GetTickCount64() - w.launched >= 30000)
                    throw DWORD(ERROR_TIMEOUT);
                pending |= w.recovering;
            }
        }
        return pending;
    }
    void cleanup()
    {
        if (bank_)
        {
            api_.bankClose(bank_);
            bank_ = nullptr;
        }
        for (auto &w : workers_)
            if (w.view)
                api_.stop(w.view);
        ULONGLONG until = GetTickCount64() + 5000;
        for (auto &w : workers_)
            if (w.process.h)
            {
                DWORD left = GetTickCount64() < until ? DWORD(until - GetTickCount64()) : 0;
                if (WaitForSingleObject(w.process.h, left) == WAIT_TIMEOUT)
                {
                    forced_ = true;
                    if (job_.h)
                        TerminateJobObject(job_.h, ERROR_CANCELLED);
                    WaitForSingleObject(w.process.h, 2000);
                }
                GetExitCodeProcess(w.process.h, &w.exit);
            }
    }

  public:
    ~Broker()
    {
        cleanup();
        for (auto &w : workers_)
            if (w.view)
                api_.close(w.view);
    }
    int run(WCHAR **argv)
    {
        const auto dll = full(argv[1]), exe = full(argv[2]);
        const bool native = wcscmp(argv[3], L"-") == 0;
        const auto script = native ? L"" : full(argv[3]);
        dllPath_ = dll;
        exePath_ = exe;
        scriptPath_ = script;
        native_ = native;
        output_ = full(argv[4]);
        bankName_ = argv[5];
        seconds_ = number(argv[6], 3600);
        const unsigned slots = number(argv[7], 4);
        fingerprint_ = argv[8];
        if (!slots || fingerprint_.size() != 64 || bankName_.size() >= 128 ||
            (bankName_.find(L"Local\\NabuDolbyBank-") != 0 && bankName_.find(L"Global\\NabuDolbyBank-") != 0))
            throw DWORD(ERROR_INVALID_PARAMETER);
        for (WCHAR c : fingerprint_)
            if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f')))
                throw DWORD(ERROR_INVALID_PARAMETER);
        file(dll);
        file(exe);
        if (!native)
            file(script);
        if (!CreateDirectoryW(output_.c_str(), nullptr))
            throw GetLastError();
        try
        {
            api_.load(dll, fingerprint_);
            workers_ = std::vector<Worker>(slots);
            job_.h = CreateJobObjectW(nullptr, nullptr);
            if (!job_.h)
                throw GetLastError();
            JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits = {};
            limits.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
            if (!SetInformationJobObject(job_.h, JobObjectExtendedLimitInformation, &limits, sizeof(limits)))
                throw GetLastError();
            SECURITY_ATTRIBUTES inheritable = {sizeof(inheritable), nullptr, TRUE};
            input_.h = CreateFileW(L"NUL", GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, &inheritable,
                                   OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
            if (input_.h == INVALID_HANDLE_VALUE)
                throw GetLastError();
            GUID token = {};
            if (FAILED(CoCreateGuid(&token)))
                throw DWORD(ERROR_GEN_FAILURE);
            WCHAR guid[40];
            StringFromGUID2(token, guid, ARRAYSIZE(guid));
            const auto sddl = ownerSddl();
            const bool global = bankName_.find(L"Global\\") == 0;
            std::vector<WCHAR> names(slots * 128, 0);
            for (unsigned i = 0; i < slots; ++i)
            {
                auto &w = workers_[i];
                w.name = (global ? L"Global\\NabuDolbyLab-" : L"Local\\NabuDolbyLab-") + std::wstring(guid) +
                         L"-" + std::to_wstring(i);
                w.view = api_.create(w.name.c_str(), 6144, sddl.c_str());
                if (!w.view)
                    throw GetLastError();
                wcscpy_s(names.data() + i * 128, 128, w.name.c_str());
                startWorker(w, i);
            }
            write("Starting");
            ULONGLONG began = GetTickCount64();
            bool ready = false;
            while (GetTickCount64() - began < 30000 && !ready)
            {
                exited();
                if (WaitForSingleObject(cancelEvent, 20) == WAIT_OBJECT_0)
                    throw DWORD(ERROR_CANCELLED);
                ready = true;
                for (auto &w : workers_)
                    ready &= api_.metric(w.view, 7) != 0;
            }
            if (!ready)
                throw DWORD(ERROR_TIMEOUT);
            bank_ = api_.bank(bankName_.c_str(), names.data(), slots, sddl.c_str());
            if (!bank_)
                throw GetLastError();
            published_ = true;
            snapshot("Ready");
            began = GetTickCount64();
            ULONGLONG report = began;
            while (!seconds_ || GetTickCount64() - began < UINT64(seconds_) * 1000)
            {
                if (GetFileAttributesW((output_ + L"\\Stop.request").c_str()) != INVALID_FILE_ATTRIBUTES ||
                    WaitForSingleObject(cancelEvent, 20) == WAIT_OBJECT_0)
                    break;
                const bool recovering = recoverExitedWorkers();
                if (GetTickCount64() - report >= 1000)
                {
                    snapshot(recovering ? "Recovering" : "Ready");
                    report = GetTickCount64();
                }
            }
        }
        catch (DWORD e)
        {
            error_ = e;
        }
        catch (...)
        {
            error_ = ERROR_UNHANDLED_EXCEPTION;
        }
        cleanup();
        snapshot(error_ ? "Failed" : "Finished");
        return error_ ? 1 : 0;
    }
};
} // namespace
int wmain(int argc, WCHAR **argv)
{
    if (argc != 9)
    {
        fwprintf(stderr, L"NabuDolbyBroker dll worker-exe script-or-- new-output bank duration-seconds slots "
                         L"source-fingerprint\n");
        return 2;
    }
    Handle event;
    event.h = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (!event.h)
        return 1;
    cancelEvent = event.h;
    SetConsoleCtrlHandler(cancel, TRUE);
    int result = 1;
    try
    {
        Broker broker;
        result = broker.run(argv);
    }
    catch (DWORD e)
    {
        fwprintf(stderr, L"Broker preflight failed: %lu\n", e);
    }
    catch (...)
    {
        fwprintf(stderr, L"Broker preflight failed\n");
    }
    SetConsoleCtrlHandler(cancel, FALSE);
    cancelEvent = nullptr;
    return result;
}
