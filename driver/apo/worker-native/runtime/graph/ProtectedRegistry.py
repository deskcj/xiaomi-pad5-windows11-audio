"""Narrow restoration of captured speaker FX; preserves ownership and ACLs."""
import ctypes as C
from contextlib import contextmanager
import struct
import winreg as R
from GraphDevices import values

class Luid(C.Structure):
    _fields_ = [('Low', C.c_uint32), ('High', C.c_int32)]
class Privilege(C.Structure):
    _fields_ = [('Luid', Luid), ('Attributes', C.c_uint32)]
class Privileges(C.Structure):
    _fields_ = [('Count', C.c_uint32), ('Item', Privilege)]
advapi = C.WinDLL('advapi32', use_last_error=True)
kernel = C.WinDLL('kernel32', use_last_error=True)
kernel.GetCurrentProcess.restype = C.c_void_p
kernel.CloseHandle.argtypes = (C.c_void_p,)
advapi.OpenProcessToken.argtypes = (C.c_void_p, C.c_uint32, C.POINTER(C.c_void_p))
advapi.LookupPrivilegeValueW.argtypes = (C.c_wchar_p, C.c_wchar_p, C.POINTER(Luid))
advapi.AdjustTokenPrivileges.argtypes = (C.c_void_p, C.c_int, C.POINTER(Privileges), C.c_uint32, C.POINTER(Privileges), C.POINTER(C.c_uint32))
advapi.RegCreateKeyExW.argtypes = (C.c_void_p, C.c_wchar_p, C.c_uint32, C.c_wchar_p, C.c_uint32, C.c_uint32, C.c_void_p, C.POINTER(C.c_void_p), C.POINTER(C.c_uint32))
advapi.RegSetValueExW.argtypes = (C.c_void_p, C.c_wchar_p, C.c_uint32, C.c_uint32, C.c_void_p, C.c_uint32)
advapi.RegDeleteValueW.argtypes = (C.c_void_p, C.c_wchar_p)
advapi.RegCloseKey.argtypes = (C.c_void_p,)

@contextmanager
def restore_privilege():
    token = C.c_void_p()
    old = Privileges()
    returned = C.c_uint32()
    if not advapi.OpenProcessToken(kernel.GetCurrentProcess(), 0x28, C.byref(token)):
        raise C.WinError(C.get_last_error())
    try:
        enabled = Privileges()
        enabled.Count = 1
        enabled.Item.Attributes = 2
        if not advapi.LookupPrivilegeValueW(None, 'SeRestorePrivilege', C.byref(enabled.Item.Luid)):
            raise C.WinError(C.get_last_error())
        C.set_last_error(0)
        if (not advapi.AdjustTokenPrivileges(token, False, C.byref(enabled), C.sizeof(old), C.byref(old), C.byref(returned))
                or C.get_last_error()):
            raise C.WinError(C.get_last_error())
        try:
            yield
        finally:
            if not advapi.AdjustTokenPrivileges(token, False, C.byref(old), 0, None, None):
                raise C.WinError(C.get_last_error())
    finally:
        kernel.CloseHandle(token)

def restore_values(rows, allowed_paths):
    # Paths were captured from the current physical speaker, never from package data.
    allowed_names = {'{d04e05a6-594b-4fb6-a80d-01af5eed7d1d},' + str(i) for i in (0, 5, 6, 7)}
    allowed_names |= {'{d3993a3f-99c2-4402-b5ec-a92a0367664b},5'}
    for row in rows:
        if row['Key'] not in allowed_paths or row['Name'].lower() not in allowed_names:
            raise ValueError('Unexpected captured FX restoration target')
        if values(row['Key']) is None:
            raise RuntimeError('Captured FX key disappeared; do not recreate it')
    with restore_privilege():
        for row in rows:
            handle = C.c_void_p()
            disposition = C.c_uint32()
            error = advapi.RegCreateKeyExW(C.c_void_p(0xffffffff80000002), row['Key'], 0, None, 4, 0, None, C.byref(handle), C.byref(disposition))
            if error:
                raise C.WinError(error)
            try:
                if disposition.value != 2:
                    raise RuntimeError('FX key disappeared during restoration')
                before = row['Before']
                if before is None:
                    error = advapi.RegDeleteValueW(handle, row['Name'])
                    if error == 2:
                        error = 0
                else:
                    value, kind = before
                    if kind in (R.REG_SZ, R.REG_EXPAND_SZ):
                        data = (value + '\0').encode('utf-16le')
                    elif kind == R.REG_MULTI_SZ:
                        data = ('\0'.join(value) + '\0\0').encode('utf-16le')
                    elif kind == R.REG_DWORD:
                        data = struct.pack('<I', value)
                    elif kind == R.REG_BINARY:
                        data = bytes.fromhex(value)
                    else:
                        raise ValueError('Unsupported captured FX type')
                    buffer = C.create_string_buffer(data)
                    error = advapi.RegSetValueExW(handle, row['Name'], 0, kind, buffer, len(data))
                if error:
                    raise C.WinError(error)
            finally:
                advapi.RegCloseKey(handle)
