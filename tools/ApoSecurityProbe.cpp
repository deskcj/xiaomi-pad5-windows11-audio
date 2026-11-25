// Read-only registry/section audit with the actual audio process token.
// Does not impersonate, enable privileges, write ACLs or open audio.
// AccessCheck requires duplication permission: an unavailable check is not denial.
// A successful DACL check does not prove namespace or mandatory-label access.
#include <windows.h>
#include <aclapi.h>
#include <sddl.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
static void text(const WCHAR *value){
    putchar('"');if(value)for(const WCHAR *p=value;*p;++p){
        if(*p=='"'||*p=='\\')putchar('\\');
        if(*p<32||*p>126)printf("\\u%04x",unsigned(*p));else putchar(int(*p));
    }putchar('"');
}
static LPWSTR tokenSid(HANDLE token,TOKEN_INFORMATION_CLASS kind){
    DWORD bytes=0;GetTokenInformation(token,kind,nullptr,0,&bytes);
    if(!bytes||bytes>65536)return nullptr;std::vector<BYTE> data(bytes);
    if(!GetTokenInformation(token,kind,data.data(),bytes,&bytes))return nullptr;
    PSID sid=kind==TokenUser?reinterpret_cast<TOKEN_USER *>(data.data())->User.Sid:
        reinterpret_cast<TOKEN_APPCONTAINER_INFORMATION *>(data.data())->TokenAppContainer;
    LPWSTR result=nullptr;if(sid)ConvertSidToStringSidW(sid,&result);return result;
}
int wmain(int argc,WCHAR **argv){
    if(argc<3)return 2;
    DWORD pid=wcstoul(argv[1],nullptr,10);if(!pid)return 2;
    HANDLE process=OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION,FALSE,pid),token=nullptr,duplicate=nullptr;
    DWORD processError=process?0:GetLastError(),tokenError=0,duplicateError=0;
    if(process&&!OpenProcessToken(process,TOKEN_QUERY|TOKEN_DUPLICATE,&token)){
        tokenError=GetLastError();if(OpenProcessToken(process,TOKEN_QUERY,&token))tokenError=0;
    }
    if(token&&!DuplicateToken(token,SecurityImpersonation,&duplicate))duplicateError=GetLastError();
    DWORD appContainer=0,bytes=0;BOOL appKnown=token&&GetTokenInformation(token,TokenIsAppContainer,&appContainer,sizeof(appContainer),&bytes);
    WCHAR image[32768]={};DWORD imageSize=ARRAYSIZE(image);if(process)QueryFullProcessImageNameW(process,0,image,&imageSize);
    LPWSTR user=token?tokenSid(token,TokenUser):nullptr,app=token&&appKnown&&appContainer?tokenSid(token,TokenAppContainerSid):nullptr;
    printf("{\"audio_started\":false,\"privileges_enabled\":false,\"acl_modified\":false,\"target_pid\":%lu,\"process_error\":%lu,\"token_error\":%lu,\"duplicate_error\":%lu,\"appcontainer_known\":%s,\"appcontainer\":%s,\"restricted\":%s,\"target_image\":",
        pid,processError,tokenError,duplicateError,appKnown?"true":"false",appKnown&&appContainer?"true":"false",token&&IsTokenRestricted(token)?"true":"false");
    text(image);printf(",\"user_sid\":");text(user);printf(",\"appcontainer_sid\":");text(app);printf(",\"paths\":[");
    if(user)LocalFree(user);if(app)LocalFree(app);
    for(int i=2;i<argc;++i){
        if(i>2)putchar(',');HKEY key=nullptr;HANDLE section=nullptr;PSECURITY_DESCRIPTOR sd=nullptr;
        const bool isSection=wcsncmp(argv[i],L"section:",8)==0;
        DWORD openError=0,securityError=0,checkError=0,requested=isSection?FILE_MAP_WRITE:KEY_READ;
        if(isSection){section=OpenFileMappingW(READ_CONTROL,FALSE,argv[i]+8);if(!section)openError=GetLastError();}
        else openError=RegOpenKeyExW(HKEY_LOCAL_MACHINE,argv[i],0,KEY_READ,&key);
        BOOL granted=FALSE,performed=FALSE;DWORD grantedMask=0;
        if(!openError)securityError=GetSecurityInfo(isSection?section:key,isSection?SE_KERNEL_OBJECT:SE_REGISTRY_KEY,OWNER_SECURITY_INFORMATION|GROUP_SECURITY_INFORMATION|DACL_SECURITY_INFORMATION,nullptr,nullptr,nullptr,nullptr,&sd);
        if(sd&&duplicate&&!securityError){
            GENERIC_MAPPING mapping={KEY_READ,KEY_WRITE,KEY_EXECUTE,KEY_ALL_ACCESS};
            if(isSection)mapping={STANDARD_RIGHTS_READ|SECTION_QUERY|SECTION_MAP_READ,STANDARD_RIGHTS_WRITE|SECTION_MAP_WRITE|SECTION_EXTEND_SIZE,STANDARD_RIGHTS_EXECUTE|SECTION_MAP_EXECUTE,SECTION_ALL_ACCESS};
            std::vector<BYTE> privileges(1024);DWORD size=DWORD(privileges.size());
            performed=AccessCheck(sd,duplicate,requested,&mapping,reinterpret_cast<PRIVILEGE_SET *>(privileges.data()),&size,&grantedMask,&granted);
            if(!performed&&GetLastError()==ERROR_INSUFFICIENT_BUFFER&&size<65536){
                privileges.resize(size);performed=AccessCheck(sd,duplicate,requested,&mapping,reinterpret_cast<PRIVILEGE_SET *>(privileges.data()),&size,&grantedMask,&granted);
            }
            if(!performed)checkError=GetLastError();
        }
        LPWSTR sddl=nullptr;
        if(sd)ConvertSecurityDescriptorToStringSecurityDescriptorW(sd,SDDL_REVISION_1,OWNER_SECURITY_INFORMATION|GROUP_SECURITY_INFORMATION|DACL_SECURITY_INFORMATION,&sddl,nullptr);
        printf("{\"path\":");text(argv[i]);printf(",\"is_section\":%s,\"requested_mask\":%lu,\"open_error\":%lu,\"security_error\":%lu,\"accesscheck_error\":%lu,\"accesscheck_performed\":%s,\"access_granted\":%s,\"granted_mask\":%lu,\"sddl\":",
            isSection?"true":"false",requested,openError,securityError,checkError,performed?"true":"false",performed&&granted?"true":"false",grantedMask);text(sddl);putchar('}');
        if(sddl)LocalFree(sddl);if(sd)LocalFree(sd);if(key)RegCloseKey(key);if(section)CloseHandle(section);
    }
    printf("]}\n");if(duplicate)CloseHandle(duplicate);if(token)CloseHandle(token);if(process)CloseHandle(process);
    return 0;
}
