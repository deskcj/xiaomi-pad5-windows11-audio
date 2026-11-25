"""Lab-only typed native original DRC scale; every other Dolby function is unchanged."""
from pathlib import Path
import ctypes as C,hashlib,json,sys
root=Path(__file__).resolve().parent
previous=root
sys.path.insert(0,str(previous))
from SharedClassifier import SharedDap,probe,payload,segments
ENTRY=probe.IMAGE+0x156370
class LevelDap(SharedDap):
    def __init__(self,**kwargs):
        super().__init__(**kwargs)
        audit=json.loads((root/'resources/CallGraphAudit02.json').read_text())
        row=next(r for r in audit['Candidates'] if r['Address']=='0x156370')
        assert audit['OriginalSha256']==probe.SHA256 and row['Eligible'] and not row['Imports']
        assert row['Functions']==[{'Address':'0x156370','Name':'DAP_CPDP_PVT_lvl_calc_drc_scale','Bytes':792}]
        a=0x156370;length=792
        for segment in segments:
            if segment['p_vaddr']<=a and a+length<=segment['p_vaddr']+segment['p_filesz']:
                offset=segment['p_offset']+a-segment['p_vaddr']
                assert bytes(self.cpu.mem_read(ENTRY,length))==payload[offset:offset+length]
                break
        else:raise RuntimeError('Audited level-scale code is not file backed')
        dll=root/'native/level/NativeLevelScale.dll'
        build=json.loads((dll.parent/'Build.json').read_text(encoding='utf-8-sig'))
        assert hashlib.sha256(dll.read_bytes()).hexdigest()==build['DllSha256']
        self.level=C.WinDLL(str(dll));self.level_context=None
        self.level.NativeLevelScaleInvoke.argtypes=(C.c_size_t,C.POINTER(C.c_uint64),C.POINTER(C.c_float),C.c_uint64)
        self.level.NativeLevelScaleInvoke.restype=C.c_uint32
        self.level.NativeLevelScaleHookSize.argtypes=();self.level.NativeLevelScaleHookSize.restype=C.c_size_t
        self.level.NativeLevelScaleAttach.argtypes=(C.c_void_p,C.c_void_p,C.c_void_p);self.level.NativeLevelScaleAttach.restype=C.c_int
        self.level.NativeLevelScaleDetach.argtypes=(C.c_void_p,);self.level.NativeLevelScaleDetach.restype=C.c_int
        self.level.NativeLevelScaleMetric.argtypes=(C.c_void_p,C.c_uint32);self.level.NativeLevelScaleMetric.restype=C.c_uint64
        self.level.NativeLevelScaleClearMetrics.argtypes=(C.c_void_p,);self.level.NativeLevelScaleClearMetrics.restype=None
    def enable_level(self):
        if self.level_context is not None:raise RuntimeError('Level scale already attached')
        from unicorn.unicorn_py3.unicorn import uclib
        context=C.create_string_buffer(self.level.NativeLevelScaleHookSize())
        status=self.level.NativeLevelScaleAttach(uclib._handle,self.cpu._uch,context)
        if status:raise RuntimeError('Level hook failed '+str(status))
        self.level_context=context;self.cpu.ctl_remove_cache(ENTRY,ENTRY+792)
    def disable_level(self):
        if self.level_context is None:return
        status=self.level.NativeLevelScaleDetach(self.level_context)
        if status:raise RuntimeError('Level detach failed '+str(status))
        self.level_context=None;self.cpu.ctl_remove_cache(ENTRY,ENTRY+792)
    def level_statistics(self):
        if self.level_context is None:return dict(Calls=0)
        values=[self.level.NativeLevelScaleMetric(self.level_context,i) for i in range(5)]
        if values[3] or values[4]:raise RuntimeError('Level native call failed '+str(values))
        return dict(Calls=values[0],MeanCallMs=1000*values[1]/values[2]/values[0] if values[0] else None)
