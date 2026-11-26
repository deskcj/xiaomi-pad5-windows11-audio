"""MMCSS scheduling for the background processor; no device access."""
import atexit,ctypes
from ctypes import wintypes
def schedule_audio_worker():
    avrt = ctypes.WinDLL("avrt", use_last_error=True)
    enter = avrt.AvSetMmThreadCharacteristicsW
    enter.argtypes = (wintypes.LPCWSTR, ctypes.POINTER(wintypes.DWORD))
    enter.restype = wintypes.HANDLE
    revert = avrt.AvRevertMmThreadCharacteristics
    revert.argtypes = (wintypes.HANDLE,)
    revert.restype = wintypes.BOOL
    task_index = wintypes.DWORD()
    handle = enter("Audio", ctypes.byref(task_index))
    if not handle:
        raise ctypes.WinError(ctypes.get_last_error())
    priority = avrt.AvSetMmThreadPriority
    priority.argtypes = (wintypes.HANDLE, ctypes.c_int)
    priority.restype = wintypes.BOOL
    # Raise only this Audio MMCSS thread using the documented HIGH setting.
    # No process-wide realtime priority or registry/task change.
    if not priority(handle, 1):  # AVRT_PRIORITY_HIGH
        error = ctypes.get_last_error()
        revert(handle)
        raise ctypes.WinError(error)
    atexit.register(revert, handle)
    kernel = ctypes.WinDLL('kernel32', use_last_error=True)
    kernel.GetCurrentThread.restype = wintypes.HANDLE
    kernel.GetThreadPriority.argtypes = (wintypes.HANDLE,)
    kernel.GetThreadPriority.restype = ctypes.c_int
    return dict(Task='Audio', RequestedPriority='AVRT_PRIORITY_HIGH',
                ObservedThreadPriority=kernel.GetThreadPriority(kernel.GetCurrentThread()))
