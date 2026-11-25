// Owns only the packaged background broker. Does not open an audio endpoint.
#include <windows.h>
#include <shlobj.h>
#include <strsafe.h>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <stdio.h>

namespace {
constexpr WCHAR ServiceName[]=L"NabuDolbyRuntime";
HANDLE stopped=nullptr;
SERVICE_STATUS_HANDLE serviceHandle=nullptr;
SERVICE_STATUS serviceStatus={};
SRWLOCK statusLock=SRWLOCK_INIT;
struct Handle {
    HANDLE h=nullptr;
    ~Handle(){if(h&&h!=INVALID_HANDLE_VALUE)CloseHandle(h);}
    Handle()=default;Handle(const Handle&)=delete;Handle&operator=(const Handle&)=delete;
};
std::wstring quote(const std::wstring&s){
    std::wstring out=L"\"";size_t slashes=0;
    for(WCHAR c:s){if(c==L'\\'){++slashes;continue;}out.append(c==L'"'?slashes*2+1:slashes,L'\\');slashes=0;out+=c;}
    out.append(slashes*2,L'\\');return out+L"\"";
}
std::wstring folder(){
    WCHAR path[32768];DWORD n=GetModuleFileNameW(nullptr,path,ARRAYSIZE(path));
    if(!n||n>=ARRAYSIZE(path))throw DWORD(ERROR_INVALID_NAME);
    std::wstring result(path,n);auto at=result.find_last_of(L"\\/");
    if(at==std::wstring::npos)throw DWORD(ERROR_INVALID_NAME);return result.substr(0,at);
}
void file(const std::wstring&path){DWORD a=GetFileAttributesW(path.c_str());if(a==INVALID_FILE_ATTRIBUTES||(a&FILE_ATTRIBUTE_DIRECTORY))throw DWORD(ERROR_FILE_NOT_FOUND);}
std::wstring setting(const std::wstring&ini,const WCHAR*key){
    WCHAR value[512];DWORD n=GetPrivateProfileStringW(L"Runtime",key,L"",value,ARRAYSIZE(value),ini.c_str());
    if(!n||n>=ARRAYSIZE(value)-1)throw DWORD(ERROR_INVALID_DATA);return std::wstring(value,n);
}
bool hex64(const std::wstring&s){if(s.size()!=64)return false;for(WCHAR c:s)if(!((c>=L'0'&&c<=L'9')||(c>=L'a'&&c<=L'f')))return false;return true;}
std::wstring logRoot(const std::wstring&ini){
    WCHAR configured[32768];DWORD n=GetPrivateProfileStringW(L"Runtime",L"LogRoot",L"",configured,ARRAYSIZE(configured),ini.c_str());
    if(n){if(n>=ARRAYSIZE(configured)-1||n<3||configured[1]!=L':'||(configured[2]!=L'\\'&&configured[2]!=L'/'))throw DWORD(ERROR_INVALID_NAME);return configured;}
    PWSTR p=nullptr;HRESULT hr=SHGetKnownFolderPath(FOLDERID_LocalAppData,KF_FLAG_CREATE,nullptr,&p);
    if(FAILED(hr))throw DWORD(hr);std::wstring base(p);CoTaskMemFree(p);
    base+=L"\\NabuAudio";if(!CreateDirectoryW(base.c_str(),nullptr)&&GetLastError()!=ERROR_ALREADY_EXISTS)throw GetLastError();
    base+=L"\\Dolby";if(!CreateDirectoryW(base.c_str(),nullptr)&&GetLastError()!=ERROR_ALREADY_EXISTS)throw GetLastError();
    return base;
}
void status(DWORD state,DWORD error=NO_ERROR,DWORD wait=0){
    if(!serviceHandle)return;
    AcquireSRWLockExclusive(&statusLock);
    if(state==SERVICE_RUNNING&&WaitForSingleObject(stopped,0)==WAIT_OBJECT_0){state=SERVICE_STOP_PENDING;wait=10000;}
    serviceStatus.dwServiceType=SERVICE_WIN32_OWN_PROCESS;
    serviceStatus.dwCurrentState=state;
    serviceStatus.dwControlsAccepted=state==SERVICE_RUNNING?SERVICE_ACCEPT_STOP|SERVICE_ACCEPT_SHUTDOWN:0;
    serviceStatus.dwWin32ExitCode=error;
    serviceStatus.dwWaitHint=wait;
    serviceStatus.dwCheckPoint=state==SERVICE_START_PENDING||state==SERVICE_STOP_PENDING?serviceStatus.dwCheckPoint+1:0;
    SetServiceStatus(serviceHandle,&serviceStatus);
    ReleaseSRWLockExclusive(&statusLock);
}
DWORD WINAPI control(DWORD action,DWORD,LPVOID,LPVOID){
    if(action==SERVICE_CONTROL_STOP||action==SERVICE_CONTROL_SHUTDOWN){SetEvent(stopped);status(SERVICE_STOP_PENDING,NO_ERROR,10000);return NO_ERROR;}
    if(action==SERVICE_CONTROL_INTERROGATE){AcquireSRWLockShared(&statusLock);SetServiceStatus(serviceHandle,&serviceStatus);ReleaseSRWLockShared(&statusLock);return NO_ERROR;}
    return ERROR_CALL_NOT_IMPLEMENTED;
}
BOOL WINAPI consoleControl(DWORD action){if(action==CTRL_C_EVENT||action==CTRL_BREAK_EVENT){SetEvent(stopped);return TRUE;}return FALSE;}
std::string read(const std::wstring&path){
    Handle f;f.h=CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,0,nullptr);
    if(f.h==INVALID_HANDLE_VALUE)return {};
    LARGE_INTEGER size;if(!GetFileSizeEx(f.h,&size)||size.QuadPart<0||size.QuadPart>131072)return {};
    std::string data(size_t(size.QuadPart),'\0');DWORD got=0;
    if(!ReadFile(f.h,data.data(),DWORD(data.size()),&got,nullptr)||got!=data.size())return {};return data;
}
void write(const std::wstring&path,const std::string&data){
    Handle f;f.h=CreateFileW(path.c_str(),GENERIC_WRITE,FILE_SHARE_READ,nullptr,CREATE_ALWAYS,0,nullptr);
    if(f.h==INVALID_HANDLE_VALUE)throw GetLastError();DWORD written=0;
    if(!WriteFile(f.h,data.data(),DWORD(data.size()),&written,nullptr)||written!=data.size())throw GetLastError();
}
std::string ascii(const std::wstring&s){std::string out;for(WCHAR c:s){if(c>127)throw DWORD(ERROR_INVALID_DATA);out+=char(c);}return out;}
std::string json(const std::wstring&s){
    std::string out="\"";char encoded[8];
    for(WCHAR c:s){if(c==L'"'||c==L'\\'){out+='\\';out+=char(c);}else if(c<32||c>126){sprintf_s(encoded,"\\u%04x",unsigned(c));out+=encoded;}else out+=char(c);}
    return out+'"';
}
struct Spawn {
    Handle process,thread,job,output,error,input;
    std::vector<BYTE> attributes;
    LPPROC_THREAD_ATTRIBUTE_LIST list=nullptr;
    ~Spawn(){if(list)DeleteProcThreadAttributeList(list);}
    void start(const std::wstring&command,const std::wstring&root,const std::wstring&logs){
        SECURITY_ATTRIBUTES security={sizeof(security),nullptr,TRUE};
        output.h=CreateFileW((logs+L"\\broker.stdout").c_str(),GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,&security,CREATE_NEW,0,nullptr);
        error.h=CreateFileW((logs+L"\\broker.stderr").c_str(),GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,&security,CREATE_NEW,0,nullptr);
        input.h=CreateFileW(L"NUL",GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,&security,OPEN_EXISTING,0,nullptr);
        if(output.h==INVALID_HANDLE_VALUE||error.h==INVALID_HANDLE_VALUE||input.h==INVALID_HANDLE_VALUE)throw GetLastError();
        job.h=CreateJobObjectW(nullptr,nullptr);if(!job.h)throw GetLastError();
        JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits={};limits.BasicLimitInformation.LimitFlags=JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
        if(!SetInformationJobObject(job.h,JobObjectExtendedLimitInformation,&limits,sizeof(limits)))throw GetLastError();
        SIZE_T bytes=0;InitializeProcThreadAttributeList(nullptr,1,0,&bytes);attributes.resize(bytes);
        list=reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(attributes.data());
        if(!InitializeProcThreadAttributeList(list,1,0,&bytes))throw GetLastError();
        HANDLE inherited[]={output.h,error.h,input.h};
        if(!UpdateProcThreadAttribute(list,0,PROC_THREAD_ATTRIBUTE_HANDLE_LIST,inherited,sizeof(inherited),nullptr,nullptr))throw GetLastError();
        STARTUPINFOEXW info={};info.StartupInfo.cb=sizeof(info);info.lpAttributeList=list;
        info.StartupInfo.dwFlags=STARTF_USESTDHANDLES;info.StartupInfo.hStdOutput=output.h;info.StartupInfo.hStdError=error.h;info.StartupInfo.hStdInput=input.h;
        std::vector<WCHAR> cmd(command.begin(),command.end());cmd.push_back(0);PROCESS_INFORMATION child={};
        if(!CreateProcessW(nullptr,cmd.data(),nullptr,nullptr,TRUE,CREATE_SUSPENDED|CREATE_NO_WINDOW|EXTENDED_STARTUPINFO_PRESENT,nullptr,root.c_str(),&info.StartupInfo,&child))throw GetLastError();
        process.h=child.hProcess;thread.h=child.hThread;
        if(!AssignProcessToJobObject(job.h,process.h)){DWORD e=GetLastError();TerminateProcess(process.h,e);throw e;}
        if(ResumeThread(thread.h)==DWORD(-1))throw GetLastError();
    }
};
DWORD run(){
    try {
        const std::wstring root=folder(),ini=root+L"\\Runtime.ini";file(ini);
        const std::wstring fingerprint=setting(ini,L"SourceFingerprint"),digest=setting(ini,L"BridgeSha256"),bank=setting(ini,L"BankName");
        if(!hex64(fingerprint)||!hex64(digest))throw DWORD(ERROR_INVALID_DATA);
        if(bank!=L"Global\\NabuDolbyBank-9df8834a-209e-4cc1-bc73-bb4353adfa9f"&&bank.find(L"Global\\NabuDolbyBank-ServiceSmoke-")!=0)throw DWORD(ERROR_INVALID_NAME);
        for(WCHAR c:bank)if(!(c==L'\\'||c==L'-'||c==L'{'||c==L'}'||(c>=L'a'&&c<=L'z')||(c>=L'A'&&c<=L'Z')||(c>=L'0'&&c<=L'9')))throw DWORD(ERROR_INVALID_NAME);
        const std::wstring broker=root+L"\\NabuDolbyBroker.exe",bridge=root+L"\\NabuDolbyBridge.dll",python=root+L"\\python\\python.exe",worker=root+L"\\PortableOriginalWorker.py";
        for(const auto&p:{broker,bridge,python,worker})file(p);
        if(!SetEnvironmentVariableW(L"LIBUNICORN_PATH",(root+L"\\backend\\native\\unicorn").c_str())||!SetEnvironmentVariableW(L"NABU_DOLBY_BRIDGE_SHA256",digest.c_str()))throw GetLastError();
        const std::wstring base=logRoot(ini);DWORD last=NO_ERROR;
        for(unsigned attempt=0;attempt<3&&WaitForSingleObject(stopped,0)!=WAIT_OBJECT_0;++attempt){
            WCHAR unique[80];swprintf_s(unique,L"run-%lu-%llu-%u",GetCurrentProcessId(),GetTickCount64(),attempt);
            const std::wstring logs=base+L"\\"+unique;
            if(!CreateDirectoryW(logs.c_str(),nullptr))throw GetLastError();
            const std::wstring output=logs+L"\\runtime";
            const std::wstring command=quote(broker)+L" "+quote(bridge)+L" "+quote(python)+L" "+quote(worker)+L" "+quote(output)+L" "+quote(bank)+L" 0 2 "+quote(fingerprint);
            Spawn child;child.start(command,root,logs);
            const ULONGLONG start=GetTickCount64();bool ready=false;
            for(;;){
                DWORD waited=WaitForSingleObject(child.process.h,100);
                if(waited==WAIT_OBJECT_0){GetExitCodeProcess(child.process.h,&last);break;}
                if(waited==WAIT_FAILED)throw GetLastError();
                if(WaitForSingleObject(stopped,0)==WAIT_OBJECT_0){
                    status(SERVICE_STOP_PENDING,NO_ERROR,10000);
                    if(GetFileAttributesW(output.c_str())!=INVALID_FILE_ATTRIBUTES)write(output+L"\\Stop.request","SCM requested cooperative shutdown");
                    bool forced=WaitForSingleObject(child.process.h,10000)!=WAIT_OBJECT_0;
                    if(forced)TerminateJobObject(child.job.h,ERROR_OPERATION_ABORTED);
                    write(logs+L"\\ServiceResult.json",std::string("{\"stopped\":true,\"forced_owned_job_termination\":")+(forced?"true":"false")+",\"audio_opened\":false}\n");
                    return forced?ERROR_TIMEOUT:NO_ERROR;
                }
                if(!ready){
                    const std::string data=read(output+L"\\Broker.json");
                    const bool stateReady=data.find("\"state\":\"Ready\"")!=std::string::npos||data.find("\"state\": \"Ready\"")!=std::string::npos;
                    const std::string ownedPid="\"broker_pid\":"+std::to_string(GetProcessId(child.process.h))+",";
                    if(stateReady&&data.find(ownedPid)!=std::string::npos&&data.find(ascii(fingerprint))!=std::string::npos){
                        ready=true;status(SERVICE_RUNNING);
                        write(logs+L"\\ServiceReady.json","{\"ready\":true,\"backend_root\":"+json(root)+",\"broker_pid_owned\":true,\"audio_opened\":false}\n");
                    }else if(GetTickCount64()-start>25000){TerminateJobObject(child.job.h,ERROR_TIMEOUT);last=ERROR_TIMEOUT;break;}
                    else status(SERVICE_START_PENDING,NO_ERROR,30000);
                }
            }
            write(logs+L"\\ServiceResult.json","{\"broker_exit\":"+std::to_string(last)+",\"retry_attempt\":"+std::to_string(attempt)+",\"audio_opened\":false}\n");
            if(last==NO_ERROR)return NO_ERROR;
            if(WaitForSingleObject(stopped,1000)==WAIT_OBJECT_0)return NO_ERROR;
        }
        return last;
    }catch(DWORD error){return error;}catch(...){return ERROR_UNHANDLED_EXCEPTION;}
}
void WINAPI serviceMain(DWORD,LPWSTR*){
    serviceHandle=RegisterServiceCtrlHandlerExW(ServiceName,control,nullptr);
    if(!serviceHandle)return;
    status(SERVICE_START_PENDING,NO_ERROR,30000);DWORD result=run();status(SERVICE_STOPPED,result);
}
}
int wmain(int argc,WCHAR**argv){
    stopped=CreateEventW(nullptr,TRUE,FALSE,nullptr);if(!stopped)return int(GetLastError());
    if(argc==2&&wcscmp(argv[1],L"--console")==0){SetConsoleCtrlHandler(consoleControl,TRUE);DWORD result=run();CloseHandle(stopped);return int(result);}
    if(argc>2||(argc==2&&wcscmp(argv[1],L"--service")!=0)){CloseHandle(stopped);return ERROR_INVALID_PARAMETER;}
    SERVICE_TABLE_ENTRYW entries[]={{const_cast<WCHAR*>(ServiceName),serviceMain},{nullptr,nullptr}};
    BOOL result=StartServiceCtrlDispatcherW(entries);DWORD error=result?NO_ERROR:GetLastError();CloseHandle(stopped);return int(error);
}
