"""Restore only captured-new Nabu contexts using ordinary backup/restore rights.
Existing context contents, registry ownership and ACLs are never changed.
"""
import ctypes as C
import winreg as R
from ApoContextRollback import read_tree,plan_cleanup,validate_path
from KnownApoContexts import normalized
from ProtectedRegistry import restore_privilege,advapi
native=C.WinDLL('ntdll')
native.NtDeleteKey.argtypes=(C.c_void_p,);native.NtDeleteKey.restype=C.c_int32

def open_restore(path):
    handle=C.c_void_p();disposition=C.c_uint32()
    result=advapi.RegCreateKeyExW(C.c_void_p(0xffffffff80000002),path,0,None,4,0,None,C.byref(handle),C.byref(disposition))
    if result:raise C.WinError(result)
    if disposition.value!=2:
        # A concurrently removed key must not be recreated by the open helper.
        native.NtDeleteKey(handle);advapi.RegCloseKey(handle)
        raise RuntimeError('Captured key disappeared during restoration')
    return handle

def restore(snapshot,old_endpoint_key,live_endpoint_key):
    plans=[]
    for original,before in snapshot.items():
        path=live_endpoint_key+original[len(old_endpoint_key):] if original.startswith(old_endpoint_key+'\\') else original
        validate_path(path);current=read_tree(path)
        if before is not None:
            if normalized(current)!=normalized(before):raise RuntimeError('Existing context changed; preserve it')
            continue
        plans.append((path,plan_cleanup(current)))
    operations_done=0
    with restore_privilege():
        for path,operations in plans:
            for operation in operations:
                target=path+('\\'+operation[1] if operation[1] else '')
                if operation[0]=='value':
                    with R.OpenKey(R.HKEY_LOCAL_MACHINE,target) as key:
                        value,kind=R.QueryValueEx(key,operation[2])
                    if [value,kind]!=operation[3]:raise RuntimeError('Context value changed during restoration')
                    handle=open_restore(target)
                    try:
                        status=advapi.RegDeleteValueW(handle,operation[2])
                        if status:raise C.WinError(status)
                    finally:advapi.RegCloseKey(handle)
                else:
                    with R.OpenKey(R.HKEY_LOCAL_MACHINE,target) as key:
                        children,nvalues,_=R.QueryInfoKey(key)
                    if children or nvalues:raise RuntimeError('Context is not empty')
                    handle=open_restore(target)
                    try:
                        status=native.NtDeleteKey(handle)
                        if status<0:raise RuntimeError('Delete empty captured key failed: '+hex(status&0xffffffff))
                    finally:advapi.RegCloseKey(handle)
                operations_done+=1
    for original,before in snapshot.items():
        path=live_endpoint_key+original[len(old_endpoint_key):] if original.startswith(old_endpoint_key+'\\') else original
        if normalized(read_tree(path))!=normalized(before):raise RuntimeError('Context restoration did not verify')
    return dict(Verified=True,Operations=operations_done,ExistingContextsPreserved=sum(v is not None for v in snapshot.values()),
        RegistryAclChanged=False,RegistryOwnerChanged=False)
