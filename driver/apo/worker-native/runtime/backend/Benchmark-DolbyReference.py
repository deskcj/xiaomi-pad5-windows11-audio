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
    atexit.register(revert, handle)
