"""Original Android DSP worker with bounded telemetry, no audio files or device IO.

Previously qualified adapters and resources are resolved inside this installed
runtime. This remains an original Android stereo DAP renderer in an ARM64 guest.
"""
import sys
sys.dont_write_bytecode=True
from pathlib import Path
import collections,ctypes as C,hashlib,importlib,json,math,os,statistics,struct,sys,time

def directory(key):
    p=Path(os.environ[key]);assert p.is_absolute() and p.is_dir(),key;return p

def main():
    if len(sys.argv)!=4:raise ValueError('worker transport new-output bridge-dll')
    support=render_root=native=Path(__file__).resolve().parent/'backend'
    sys.path[:0]=[str(support),str(support/'deps')]
    from Bridge import Block,metrics
    from OriginalState import OriginalState
    from MathImports import enable
    from SharedLevelScale import LevelDap
    render=importlib.import_module('Render-DolbyReference');bench=importlib.import_module('Benchmark-DolbyReference')
    output=Path(sys.argv[2]);output.mkdir();dll=Path(sys.argv[3])
    expected=os.environ['NABU_DOLBY_BRIDGE_SHA256']
    if hashlib.sha256(dll.read_bytes()).hexdigest()!=expected:raise ValueError('Bridge DLL hash mismatch')
    lib=C.WinDLL(str(dll),use_last_error=True)
    definitions={
        'NabuWorkerOpen':(C.c_void_p,(C.c_wchar_p,)),
        'NabuTransportClose':(None,(C.c_void_p,)),
        'NabuTransportStopped':(C.c_int32,(C.c_void_p,)),
        'NabuTransportGeneration':(C.c_int32,(C.c_void_p,)),
        'NabuWorkerReadCurrent':(C.c_int32,(C.c_void_p,C.POINTER(Block),C.POINTER(C.c_uint32))),
        'NabuWorkerWrite':(C.c_int32,(C.c_void_p,C.POINTER(Block))),
        'NabuWorkerReady':(None,(C.c_void_p,C.c_int32)),
        'NabuWorkerGap':(None,(C.c_void_p,)),
        'NabuTransportMetric':(C.c_int32,(C.c_void_p,C.c_uint32)),
        'NabuTransportFrames':(C.c_int64,(C.c_void_p,C.c_uint32))}
    for name,(result,args) in definitions.items():
        f=getattr(lib,name);f.restype=result;f.argtypes=args
    view=lib.NabuWorkerOpen(sys.argv[1])
    if not view:raise C.WinError(C.get_last_error())
    bench.schedule_audio_worker();settings,bass,pregain=render.original_configuration();imports=enable()
    render.SoftwareDap=LevelDap
    player=None;pristine=None;errors=[];resets=collections.deque(maxlen=64);epochs=collections.deque(maxlen=16);durations=collections.deque(maxlen=2048)
    frames=blocks=discarded=reset_count=expired_requests=0;generation=expected_frame=first=0
    pending=bytearray();input_digest=hashlib.sha256();output_digest=hashlib.sha256();input_peak=output_peak=render_ms=0.0
    began=time.perf_counter();last_report=began;cancelled=False;telemetry_errors=0;last_telemetry_error=None
    def report(phase):
        nonlocal telemetry_errors,last_telemetry_error
        data=dict(State=phase,AudioDeviceOpened=False,AudioDumpWritten=False,PacketLogWritten=False,
            AndroidLibrarySha256=render.SHA256,BridgeDllSha256=expected,Profile='Captured Dynamic / speaker_landscape',
            CapturedPregain=pregain,VirtualBass=bass,Blocks=blocks,Frames=frames,BatchFrames=1024,
            InputSha256=input_digest.hexdigest(),OutputSha256=output_digest.hexdigest(),InputPeak=input_peak,OutputPeak=output_peak,
            ContextResetCount=reset_count,RecentContextResets=list(resets),RecentEpochs=list(epochs),
            DiscardedStaleRequests=discarded,ExpiredRequestsSkipped=expired_requests,MeanRenderMs=render_ms/blocks if blocks else None,
            RecentP95RenderMs=statistics.quantiles(durations,n=20)[18] if len(durations)>1 else None,
            RecentMaxRenderMs=max(durations) if durations else None,DurationWindowCapacity=2048,
            WholeLibraryNative=False,SnapshotBytes=pristine.bytes if pristine else None,
            WallSeconds=time.perf_counter()-began,Errors=errors,Transport=metrics(lib,view),
            TelemetryWriteFailures=telemetry_errors,LastTelemetryError=last_telemetry_error)
        if player:
            data['NativeLevelScale']=player.dap.level_statistics();data['NativeClassifierCalls']=player.dap.native_statistics()[0]
        temp=output/'worker.json.tmp'
        for attempt in range(4):
            try:
                temp.write_text(json.dumps(data,indent=2)+'\n');temp.replace(output/'worker.json');break
            except OSError as e:
                if attempt<3 and getattr(e,'winerror',None) in (5,32):time.sleep(.01);continue
                telemetry_errors+=1;last_telemetry_error=dict(Errno=e.errno,Winerror=getattr(e,'winerror',None));break
    try:
        player=render.StereoRenderer(settings|{'preg':(pregain,)},bass,call_timeout_us=None,fast_imports=imports,fast_heap=True)
        player.prepare_buffers(4);player.dap.enable_level();player.dap.enable_c_classifier();pristine=OriginalState(player)
        for _ in range(4):player.render((0.0,)*2048)
        pristine.restore(player);lib.NabuWorkerReady(view,1);report('Ready');print('READY',flush=True)
        request,reply=Block(),Block();skipped=C.c_uint32()
        while not lib.NabuTransportStopped(view):
            available=lib.NabuWorkerReadCurrent(view,C.byref(request),C.byref(skipped))
            expired_requests+=skipped.value
            if not available:
                if time.perf_counter()-last_report>=1:report('Ready');last_report=time.perf_counter()
                time.sleep(.001);continue
            current=lib.NabuTransportGeneration(view)
            if request.generation!=current:discarded+=1;continue
            if request.generation!=generation or request.first_frame!=expected_frame:
                if generation:
                    lib.NabuWorkerGap(view);lib.NabuWorkerReady(view,0);reset_start=time.perf_counter();pristine.restore(player)
                    reset_count+=1;resets.append(dict(Generation=request.generation,FirstFrame=request.first_frame,RestoreMs=(time.perf_counter()-reset_start)*1000));lib.NabuWorkerReady(view,1)
                generation=request.generation;first=expected_frame=request.first_frame;pending.clear()
                epochs.append(dict(Generation=generation,FirstFrame=first,Blocks=0,InputPeak=0.0,OutputPeak=0.0))
            pending.extend(C.string_at(C.addressof(request.frames),request.count*8));expected_frame+=request.count
            while len(pending)>=1024*8:
                data=bytes(pending[:8192]);del pending[:8192];samples=struct.unpack('<2048f',data)
                if any(not math.isfinite(v) for v in samples):raise ValueError('Non-finite input')
                started=time.perf_counter();result=player.render(samples);duration=(time.perf_counter()-started)*1000
                if any(not math.isfinite(v) for v in result):raise ValueError('Non-finite Dolby output')
                ip=max(map(abs,samples));op=max(map(abs,result))
                if op>1:raise ValueError('Dolby result exceeded normalized Float32 range')
                encoded=struct.pack('<2048f',*result);reply.first_frame,reply.generation,reply.count=first,generation,1024
                C.memmove(C.addressof(reply.frames),encoded,len(encoded));deadline=time.perf_counter()+.2
                while not lib.NabuWorkerWrite(view,C.byref(reply)):
                    if lib.NabuTransportStopped(view):cancelled=True;break
                    if time.perf_counter()>deadline:raise TimeoutError('Reply queue did not drain')
                    time.sleep(.001)
                if cancelled:break
                durations.append(duration);render_ms+=duration;blocks+=1;frames+=1024;first+=1024
                input_digest.update(data);output_digest.update(encoded);input_peak=max(input_peak,ip);output_peak=max(output_peak,op)
                epoch=epochs[-1];epoch['Blocks']+=1;epoch['InputPeak']=max(epoch['InputPeak'],ip);epoch['OutputPeak']=max(epoch['OutputPeak'],op)
                if time.perf_counter()-last_report>=1:report('Ready');last_report=time.perf_counter()
            if cancelled:break
    except BaseException as e:
        errors.append(f'{type(e).__name__}: {e}');raise
    finally:
        lib.NabuWorkerReady(view,0)
        try:report('Failed' if errors else 'Stopped')
        finally:lib.NabuTransportClose(view)

if __name__=='__main__':main()
