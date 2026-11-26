"""Original Android DSP worker with bounded telemetry, no audio files or device IO.

Previously qualified adapters and resources are resolved inside this installed
runtime. This remains an original Android stereo DAP renderer in an ARM64 guest.
"""
import sys
sys.dont_write_bytecode=True
from pathlib import Path
import collections,ctypes as C,hashlib,heapq,importlib,json,math,os,queue,statistics,struct,sys,threading,time

def directory(key):
    p=Path(os.environ[key]);assert p.is_absolute() and p.is_dir(),key;return p

def validate_handoff(assignment,broker_pid,allowed_targets,parent_pid):
    if type(assignment) is not dict or set(assignment)!={'protocol','broker_pid','transport','generation'}:
        raise ValueError('Invalid handoff fields')
    if type(assignment['protocol']) is not int or assignment['protocol']!=1 or type(assignment['broker_pid']) is not int or assignment['broker_pid']!=broker_pid or broker_pid!=parent_pid:
        raise ValueError('Foreign handoff owner/protocol')
    target=assignment['transport'];epoch=assignment['generation']
    if type(target) is not str or target not in allowed_targets or type(epoch) is not int or not 0<=epoch<2**31:
        raise ValueError('Unowned handoff target/epoch')
    return target,epoch

def standby_priority(enabled):
    kernel=C.WinDLL('kernel32',use_last_error=True)
    kernel.GetCurrentThread.restype=C.c_void_p
    kernel.SetThreadPriority.argtypes=(C.c_void_p,C.c_int);kernel.SetThreadPriority.restype=C.c_int
    if not kernel.SetThreadPriority(kernel.GetCurrentThread(),-1 if enabled else 0):
        raise C.WinError(C.get_last_error())

def main():
    warm_mode=len(sys.argv)==6 and sys.argv[1]=='--warm-spare'
    if not warm_mode and len(sys.argv)!=4:raise ValueError('worker transport new-output bridge-dll, or --warm-spare output dll broker-pid allowed-targets')
    allowed_targets=sys.argv[5].split('|') if warm_mode else []
    broker_pid=int(sys.argv[4]) if warm_mode else os.getppid()
    if warm_mode and (broker_pid!=os.getppid() or not allowed_targets or len(allowed_targets)>4 or any(not n.startswith(('Local\\NabuDolbyLab-','Global\\NabuDolbyLab-')) or len(n)>=128 for n in allowed_targets)):
        raise ValueError('Invalid private warm-spare parent/target inventory')
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
    class FrameScan(C.Structure):
        _fields_=[('frames',C.c_uint32),('flags',C.c_uint32),('peak',C.c_float),('reserved',C.c_uint32)]
    lib.NabuScanStereoFrames.argtypes=(C.c_void_p,C.c_uint32,C.POINTER(FrameScan));lib.NabuScanStereoFrames.restype=C.c_int32
    input_scan,output_scan=FrameScan(),FrameScan();slowest=[]
    definitions={
        'NabuWorkerOpen':(C.c_void_p,(C.c_wchar_p,)),
        'NabuTransportClose':(None,(C.c_void_p,)),
        'NabuTransportStopped':(C.c_int32,(C.c_void_p,)),
        'NabuTransportGeneration':(C.c_int32,(C.c_void_p,)),
        'NabuWorkerReadCurrent':(C.c_int32,(C.c_void_p,C.POINTER(Block),C.POINTER(C.c_uint32))),
        'NabuWorkerDiscardPending':(C.c_int32,(C.c_void_p,C.POINTER(C.c_uint32))),
        'NabuWorkerWrite':(C.c_int32,(C.c_void_p,C.POINTER(Block))),
        'NabuWorkerReady':(None,(C.c_void_p,C.c_int32)),
        'NabuWorkerGap':(None,(C.c_void_p,)),
        'NabuTransportMetric':(C.c_int32,(C.c_void_p,C.c_uint32)),
        'NabuTransportFrames':(C.c_int64,(C.c_void_p,C.c_uint32))}
    for name,(result,args) in definitions.items():
        f=getattr(lib,name);f.restype=result;f.argtypes=args
    view=None if warm_mode else lib.NabuWorkerOpen(sys.argv[1])
    if not warm_mode and not view:raise C.WinError(C.get_last_error())
    assigned_target=None;assigned_epoch=None;handoff_ms=None
    if warm_mode:standby_priority(True)
    scheduling=dict(Task='Standby',RequestedPriority='THREAD_PRIORITY_BELOW_NORMAL') if warm_mode else bench.schedule_audio_worker()
    settings,bass,pregain=render.original_configuration();imports=enable()
    render.SoftwareDap=LevelDap
    player=None;pristine=None;errors=[];resets=collections.deque(maxlen=64);epochs=collections.deque(maxlen=16);durations=collections.deque(maxlen=2048)
    frames=blocks=discarded=reset_count=expired_requests=0;generation=expected_frame=first=0
    pending=bytearray();input_digest=hashlib.sha256();output_digest=hashlib.sha256();input_peak=output_peak=render_ms=0.0
    began=time.perf_counter();last_report=began;cancelled=False;telemetry_errors=0;last_telemetry_error=None
    reply_gaps=collections.deque(maxlen=64);last_reply=None;max_reply_gap=max_report_ms=0.0;startup_ms=None;startup_discarded=0
    first_request_ms=first_render_ms=first_reply_ms=signal_warmup_ms=None
    telemetry_queue=queue.Queue(maxsize=1);telemetry_stop=threading.Event()
    telemetry_coalesced=0;max_telemetry_write_ms=0.0
    def write_telemetry():
        nonlocal telemetry_errors,last_telemetry_error,max_telemetry_write_ms
        try:standby_priority(True)
        except OSError as e:
            telemetry_errors+=1;last_telemetry_error=dict(Stage='Priority',Error=str(e))
        while not telemetry_stop.is_set() or not telemetry_queue.empty():
            try:data=telemetry_queue.get(timeout=.1)
            except queue.Empty:continue
            started=time.perf_counter();temp=output/'worker.json.tmp'
            for attempt in range(4):
                try:
                    temp.write_text(json.dumps(data,indent=2)+'\n');temp.replace(output/'worker.json');break
                except OSError as e:
                    if attempt<3 and getattr(e,'winerror',None) in (5,32):time.sleep(.01);continue
                    telemetry_errors+=1;last_telemetry_error=dict(Errno=e.errno,Winerror=getattr(e,'winerror',None));break
            max_telemetry_write_ms=max(max_telemetry_write_ms,(time.perf_counter()-started)*1000)
    telemetry_thread=threading.Thread(target=write_telemetry,name='NabuTelemetry',daemon=True)
    telemetry_thread.start()
    def report(phase):
        nonlocal telemetry_coalesced,max_report_ms
        report_began=time.perf_counter()
        data=dict(State=phase,AudioDeviceOpened=False,AudioDumpWritten=False,PacketLogWritten=False,
            AndroidLibrarySha256=render.SHA256,BridgeDllSha256=expected,Profile='Captured Dynamic / speaker_landscape',
            CapturedPregain=pregain,VirtualBass=bass,Blocks=blocks,Frames=frames,BatchFrames=1024,
            InputSha256=input_digest.hexdigest(),OutputSha256=output_digest.hexdigest(),InputPeak=input_peak,OutputPeak=output_peak,
            ContextResetCount=reset_count,RecentContextResets=list(resets),RecentEpochs=[dict(epoch) for epoch in epochs],
            DiscardedStaleRequests=discarded,ExpiredRequestsSkipped=expired_requests,MeanRenderMs=render_ms/blocks if blocks else None,
            RecentP95RenderMs=statistics.quantiles(durations,n=20)[18] if len(durations)>1 else None,
            RecentMaxRenderMs=max(durations) if durations else None,DurationWindowCapacity=2048,
            WholeLibraryNative=False,SnapshotBytes=pristine.bytes if pristine else None,
            WallSeconds=time.perf_counter()-began,Errors=list(errors),Transport=metrics(lib,view) if view else None,
            TelemetryWriteFailures=telemetry_errors,LastTelemetryError=last_telemetry_error,
            StartupMs=startup_ms,StartupRequestsDiscarded=startup_discarded,MaxReplyGapMs=max_reply_gap,RecentReplyGaps=list(reply_gaps),MaxReportMs=max_report_ms,
            SignalWarmupMs=signal_warmup_ms,SignalWarmupFrames=1024 if signal_warmup_ms is not None else 0,FirstRequestMs=first_request_ms,FirstRenderMs=first_render_ms,FirstReplyMs=first_reply_ms)
        data['PackedFloat32Path']=True
        data['Scheduling']=scheduling
        data['SlowestBatches']=[dict(WorkMs=x[0],Generation=x[1],FirstFrame=x[2],RenderMs=x[3]) for x in sorted(slowest,reverse=True)]
        data['SlowBatchCapacity']=64
        data['WarmSpare']=warm_mode;data['BrokerPid']=broker_pid
        data['AssignedTransport']=assigned_target;data['AssignedEpoch']=assigned_epoch;data['HandoffMs']=handoff_ms
        if player:
            data['NativeLevelScale']=player.dap.level_statistics();data['NativeClassifierCalls']=player.dap.native_statistics()[0]
        data['TelemetryQueueCapacity']=1;data['TelemetryCoalescedSnapshots']=telemetry_coalesced
        data['MaxTelemetryWriteMs']=max_telemetry_write_ms;data['TelemetryWriterPriority']='THREAD_PRIORITY_BELOW_NORMAL'
        # Only the processing owner captures native metrics and mutable state.
        # The bounded writer owns serialization and file I/O; it never touches
        # the transport or original DSP. New snapshots replace stale telemetry.
        try:telemetry_queue.put_nowait(data)
        except queue.Full:
            try:telemetry_queue.get_nowait();telemetry_coalesced+=1
            except queue.Empty:pass
            telemetry_queue.put_nowait(data)
        max_report_ms=max(max_report_ms,(time.perf_counter()-report_began)*1000)
    try:
        player=render.StereoRenderer(settings|{'preg':(pregain,)},bass,call_timeout_us=None,fast_imports=imports,fast_heap=True)
        player.prepare_buffers(4);player.dap.enable_level();player.dap.enable_c_classifier();pristine=OriginalState(player)
        for _ in range(4):player.render((0.0,)*2048)
        # Exercise signal-dependent original code before publishing READY. This
        # is guest-only warmup, never an audio output. Restore every captured
        # writable region, CPU/heap cursor and original parameter afterwards.
        warmup=tuple(value for i in range(1024) for value in
            (0.02*math.sin(2*math.pi*800*i/48000),0.02*math.sin(2*math.pi*1100*i/48000)))
        warmup_began=time.perf_counter();warm_result=player.render(warmup)
        signal_warmup_ms=(time.perf_counter()-warmup_began)*1000
        if any(not math.isfinite(v) or abs(v)>1 for v in warm_result):raise ValueError('Original signal warmup output invalid')
        pristine.restore(player);dropped=C.c_uint32()
        if warm_mode:
            # No mapping is opened or claimed before assignment. The inherited
            # anonymous stdin pipe has exactly one writer: the owning broker.
            startup_ms=(time.perf_counter()-began)*1000;report('WarmReady')
            marker=dict(protocol=1,pid=os.getpid(),broker_pid=broker_pid,warm=True)
            temp=output/'warm-ready.tmp';temp.write_text(json.dumps(marker,separators=(',',':')));temp.replace(output/'warm-ready.json')
            print('WARM_READY',flush=True)
            line=sys.stdin.buffer.readline(1025)
            if not line or line==b'STOP\n':return
            if len(line)>1024 or not line.endswith(b'\n'):raise ValueError('Unbounded or incomplete handoff')
            assignment=json.loads(line)
            assigned_target,assigned_epoch=validate_handoff(assignment,broker_pid,allowed_targets,os.getppid())
            handoff_start=time.perf_counter();view=lib.NabuWorkerOpen(assigned_target)
            if not view:raise C.WinError(C.get_last_error())
            if lib.NabuTransportGeneration(view)!=assigned_epoch:raise ValueError('Handoff epoch changed before claim')
            standby_priority(False);scheduling=bench.schedule_audio_worker()
            # Preserve pending active-stream records. ReadCurrent alone discards
            # expired data; never reset the APO epoch or another slot's cursors.
            handoff_ms=(time.perf_counter()-handoff_start)*1000
        else:
            if not lib.NabuWorkerDiscardPending(view,C.byref(dropped)):raise C.WinError(C.get_last_error())
            startup_discarded=dropped.value;startup_ms=(time.perf_counter()-began)*1000
        lib.NabuWorkerReady(view,1);report('Ready');print('READY',flush=True)
        request,reply=Block(),Block();skipped=C.c_uint32()
        while not lib.NabuTransportStopped(view):
            available=lib.NabuWorkerReadCurrent(view,C.byref(request),C.byref(skipped))
            expired_requests+=skipped.value
            if not available:
                if time.perf_counter()-last_report>=1:report('Ready');last_report=time.perf_counter()
                time.sleep(.001);continue
            if first_request_ms is None:first_request_ms=(time.perf_counter()-began)*1000
            current=lib.NabuTransportGeneration(view)
            if request.generation!=current:discarded+=1;continue
            if request.generation!=generation or request.first_frame!=expected_frame:
                if generation:
                    lib.NabuWorkerGap(view);lib.NabuWorkerReady(view,0);reset_start=time.perf_counter();pristine.restore(player)
                    reset_count+=1;resets.append(dict(Generation=request.generation,FirstFrame=request.first_frame,RestoreMs=(time.perf_counter()-reset_start)*1000));lib.NabuWorkerReady(view,1)
                generation=request.generation;first=expected_frame=request.first_frame;pending.clear()
                last_reply=None
                epochs.append(dict(Generation=generation,FirstFrame=first,Blocks=0,InputPeak=0.0,OutputPeak=0.0))
            pending.extend(C.string_at(C.addressof(request.frames),request.count*8));expected_frame+=request.count
            while len(pending)>=1024*8:
                batch_began=time.perf_counter();data=bytes(pending[:8192]);del pending[:8192]
                if not lib.NabuScanStereoFrames(C.c_char_p(data),len(data),C.byref(input_scan)):raise ValueError('Invalid packed input span')
                if input_scan.flags&1:raise ValueError('Non-finite input')
                started=time.perf_counter();encoded=player.render_packed(data);duration=(time.perf_counter()-started)*1000
                if first_render_ms is None:first_render_ms=duration
                if not lib.NabuScanStereoFrames(C.c_char_p(encoded),len(encoded),C.byref(output_scan)):raise ValueError('Invalid packed output span')
                if output_scan.flags&1:raise ValueError('Non-finite Dolby output')
                ip=input_scan.peak;op=output_scan.peak
                if output_scan.flags&2:raise ValueError('Dolby result exceeded normalized Float32 range')
                reply.first_frame,reply.generation,reply.count=first,generation,1024
                C.memmove(C.addressof(reply.frames),encoded,len(encoded));deadline=time.perf_counter()+.2
                while not lib.NabuWorkerWrite(view,C.byref(reply)):
                    if lib.NabuTransportStopped(view):cancelled=True;break
                    if time.perf_counter()>deadline:raise TimeoutError('Reply queue did not drain')
                    time.sleep(.001)
                if cancelled:break
                replied=time.perf_counter()
                timing=((replied-batch_began)*1000,generation,first,duration)
                if len(slowest)<64:heapq.heappush(slowest,timing)
                elif timing>slowest[0]:heapq.heapreplace(slowest,timing)
                if first_reply_ms is None:first_reply_ms=(replied-began)*1000
                if last_reply is not None:
                    gap=(replied-last_reply)*1000;max_reply_gap=max(max_reply_gap,gap)
                    if gap>=25:reply_gaps.append(dict(Generation=generation,FirstFrame=first,GapMs=gap,RenderMs=duration))
                last_reply=replied
                durations.append(duration);render_ms+=duration;blocks+=1;frames+=1024;first+=1024
                input_digest.update(data);output_digest.update(encoded);input_peak=max(input_peak,ip);output_peak=max(output_peak,op)
                epoch=epochs[-1];epoch['Blocks']+=1;epoch['InputPeak']=max(epoch['InputPeak'],ip);epoch['OutputPeak']=max(epoch['OutputPeak'],op)
                if time.perf_counter()-last_report>=1:report('Ready');last_report=time.perf_counter()
            if cancelled:break
    except BaseException as e:
        errors.append(f'{type(e).__name__}: {e}');raise
    finally:
        if view:lib.NabuWorkerReady(view,0)
        try:report('Failed' if errors else 'Stopped')
        finally:
            telemetry_stop.set();telemetry_thread.join(timeout=3)
            if view:lib.NabuTransportClose(view)

if __name__=='__main__':main()
