"""Read a saved 64-bit Windows ETL file; never start tracing or audio."""
import ctypes as c
import hashlib
import uuid
from ApoActivationEvidence import PROVIDER

CALLBACK=c.WINFUNCTYPE(None,c.c_void_p)
class LogFile(c.Structure):
    _fields_=[('file',c.c_wchar_p),('logger',c.c_wchar_p),('time',c.c_int64),
        ('buffers',c.c_uint32),('mode',c.c_uint32),('event',c.c_ubyte*88),('header',c.c_ubyte*280),
        ('buffer_callback',c.c_void_p),('buffer_size',c.c_uint32),('filled',c.c_uint32),('lost',c.c_uint32),
        ('event_callback',CALLBACK),('kernel_trace',c.c_uint32),('context',c.c_void_p)]

def read_trace(path):
    if c.sizeof(c.c_void_p)!=8 or c.sizeof(LogFile)!=448:raise RuntimeError('64-bit Windows ETW ABI required')
    api=c.WinDLL('advapi32',use_last_error=True)
    api.OpenTraceW.argtypes=[c.POINTER(LogFile)];api.OpenTraceW.restype=c.c_uint64
    api.ProcessTrace.argtypes=[c.POINTER(c.c_uint64),c.c_uint32,c.c_void_p,c.c_void_p];api.ProcessTrace.restype=c.c_uint32
    api.CloseTrace.argtypes=[c.c_uint64];api.CloseTrace.restype=c.c_uint32
    provider=uuid.UUID(PROVIDER).bytes_le;events=[];malformed=[]
    @CALLBACK
    def receive(address):
        header=c.string_at(address,88)
        if header[24:40]!=provider:return
        length=int.from_bytes(header[86:88],'little');pointer=c.c_void_p.from_address(address+96).value
        if not pointer or not length or length%2:malformed.append('Invalid UTF16 length');return
        try:message=c.string_at(pointer,length).decode('utf-16-le').rstrip('\0')
        except UnicodeError:malformed.append('Invalid UTF16 data');return
        events.append(dict(Provider=PROVIDER,ProcessId=int.from_bytes(header[12:16],'little'),
            ThreadId=int.from_bytes(header[8:12],'little'),FileTime=int.from_bytes(header[16:24],'little'),Message=message))
    config=LogFile();config.file=str(path);config.mode=0x10000000;config.event_callback=receive
    handle=api.OpenTraceW(c.byref(config))
    if handle==(1<<64)-1:raise c.WinError(c.get_last_error())
    try:
        trace=c.c_uint64(handle);status=api.ProcessTrace(c.byref(trace),1,None,None)
        if status:raise c.WinError(status)
    finally:api.CloseTrace(handle)
    return dict(TraceSha256=hashlib.sha256(path.read_bytes()).hexdigest(),EventsLost=config.lost,
        Events=events,Malformed=malformed,LiveTracingStarted=False)
