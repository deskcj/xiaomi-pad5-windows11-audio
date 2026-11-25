"""Audited single-function acceleration in an isolated lab process only."""
from pathlib import Path
import ctypes as C,hashlib,io,json,os,struct,sys,time
root=Path(__file__).resolve().parent
sys.path.insert(0,str(root))
import ProbeDolbyCore as probe
from unicorn import UC_HOOK_CODE
from unicorn.arm64_const import UC_ARM64_REG_X0,UC_ARM64_REG_X1,UC_ARM64_REG_X2,UC_ARM64_REG_X30,UC_ARM64_REG_PC,UC_ARM64_REG_FPCR

kernel=C.WinDLL('kernel32',use_last_error=True)
kernel.VirtualAlloc.argtypes=(C.c_void_p,C.c_size_t,C.c_uint32,C.c_uint32);kernel.VirtualAlloc.restype=C.c_void_p
kernel.VirtualProtect.argtypes=(C.c_void_p,C.c_size_t,C.c_uint32,C.POINTER(C.c_uint32));kernel.VirtualProtect.restype=C.c_int
kernel.GetCurrentProcess.restype=C.c_void_p
kernel.FlushInstructionCache.argtypes=(C.c_void_p,C.c_void_p,C.c_size_t);kernel.FlushInstructionCache.restype=C.c_int
kernel.SetErrorMode(0x8003)
OriginalUc=probe.Uc
payload=probe.LIBRARY.read_bytes();assert hashlib.sha256(payload).hexdigest()==probe.SHA256
elf=probe.ELFFile(io.BytesIO(payload));segments=[s for s in elf.iter_segments() if s['p_type']=='PT_LOAD']
image_size=probe.align(max(s['p_vaddr']+s['p_memsz'] for s in segments))
def allocate(address,size):
    result=kernel.VirtualAlloc(address,size,0x3000,4)
    if result!=address:raise RuntimeError('Fixed lab mapping unavailable: '+hex(address)+' error='+str(C.get_last_error()))
    return result
class SharedUc:
    def __init__(self,*args,**kwargs):
        self._cpu=OriginalUc(*args,**kwargs);allocate(probe.IMAGE,image_size)
    def __getattr__(self,name):return getattr(self._cpu,name)
    def mem_map(self,address,size,perms=7):
        if not probe.IMAGE<=address<probe.IMAGE+image_size:allocate(address,size)
        self._cpu.mem_map_ptr(address,size,perms,address)
    def mem_protect(self,address,size,perms):
        old=C.c_uint32();mapping={1:2,2:4,3:4,4:0x10,5:0x20,6:0x40,7:0x40}
        if not kernel.VirtualProtect(address,size,mapping[perms],C.byref(old)):raise C.WinError(C.get_last_error())
        if perms&4 and not kernel.FlushInstructionCache(kernel.GetCurrentProcess(),address,size):raise C.WinError(C.get_last_error())
        return self._cpu.mem_protect(address,size,perms)
probe.Uc=SharedUc
class SharedDap(probe.SoftwareDap):
    def __init__(self,**kwargs):
        kwargs['fast_heap']=False
        super().__init__(**kwargs)
        if self.fast_imports:self.fast_imports.set_heap(probe.HEAP,probe.HEAP_BYTES,probe.HEAP)
        build_dir=root/'native/classifier'
        build=json.loads((build_dir/'Build.json').read_text());dll=build_dir/'NativeClassifier.dll'
        assert hashlib.sha256(dll.read_bytes()).hexdigest()==build['DllSha256']
        self.native=C.WinDLL(str(dll));self.invoke=self.native.NativeClassifierInvoke
        self.invoke.argtypes=(C.c_size_t,C.POINTER(C.c_uint64),C.c_uint64);self.invoke.restype=C.c_uint32
        self.patch=[];symbols=elf.get_section_by_name('.dynsym')
        bad=C.cast(self.native.NativeClassifierUnexpectedImport,C.c_void_p).value
        lrint=C.cast(self.native.NativeClassifierLrint,C.c_void_p).value
        for r in elf.get_section_by_name('.rela.plt').iter_relocations():
            symbol=symbols.get_symbol(r['r_info_sym'])
            if symbol['st_shndx']!='SHN_UNDEF':continue
            address=probe.IMAGE+r['r_offset'];old=self.read_int(address)
            self.patch.append((address,old,lrint if symbol.name=='lrintf' else bad))
        self.calls=0;self.native_ms=[];self.enabled=False;self.hook=None;self.c_context=None
        audit=json.loads((root/'resources/CallGraphAudit02.json').read_text())
        row=next(r for r in audit['Candidates'] if r['Name']=='DAP_CPDP_PVT_classifier_process')
        assert row['Eligible'] and row['Imports']==['lrintf'] and len(row['Functions'])==2
        for f in row['Functions']:
            a=int(f['Address'],16);length=f['Bytes']
            expected=None
            for s in segments:
                if s['p_vaddr']<=a and a+length<=s['p_vaddr']+s['p_filesz']:
                    off=s['p_offset']+a-s['p_vaddr'];expected=payload[off:off+length];break
            assert expected is not None and bytes(self.cpu.mem_read(probe.IMAGE+a,length))==expected
    def invoke_classifier(self):
        # Only one thread runs this context. Patch import dependencies only while
        # the emulator is stopped; restore all GOT values before it resumes.
        arguments=(C.c_uint64*3)(*(self.cpu.reg_read(r) for r in (UC_ARM64_REG_X0,UC_ARM64_REG_X1,UC_ARM64_REG_X2)))
        began=time.perf_counter()
        try:
            for address,old,value in self.patch:C.c_uint64.from_address(address).value=value
            result=self.invoke(probe.IMAGE+0x13b888,arguments,self.cpu.reg_read(UC_ARM64_REG_FPCR))
            if result:raise RuntimeError('Native classifier status '+hex(result))
        finally:
            for address,old,value in self.patch:C.c_uint64.from_address(address).value=old
        self.calls+=1;self.native_ms.append(1000*(time.perf_counter()-began))
    def enable_classifier(self):
        if self.hook is not None:raise RuntimeError('Classifier already attached')
        def execute(cpu,address,size,user):
            if not self.enabled:return
            lr=cpu.reg_read(UC_ARM64_REG_X30)
            self.invoke_classifier();cpu.reg_write(UC_ARM64_REG_PC,lr)
        self.enabled=True
        self.hook=self.cpu.hook_add(UC_HOOK_CODE,execute,begin=probe.IMAGE+0x13b888,end=probe.IMAGE+0x13b888)
        self.cpu.ctl_remove_cache(probe.IMAGE+0x13b888,probe.IMAGE+0x13c880)
    def disable_classifier(self):
        self.enabled=False
        if self.hook is not None:self.cpu.hook_del(self.hook);self.hook=None
        if self.c_context is not None:
            status=self.native.NativeClassifierDetach(self.c_context)
            if status:raise RuntimeError('Native C hook detach status '+str(status))
            self.c_context=None
        self.cpu.ctl_remove_cache(probe.IMAGE+0x13b888,probe.IMAGE+0x13c880)
    def enable_c_classifier(self):
        if self.hook is not None or self.c_context is not None:raise RuntimeError('Classifier already attached')
        from unicorn.unicorn_py3.unicorn import uclib
        class Patch(C.Structure):_fields_=[('address',C.c_size_t),('oldValue',C.c_uint64),('nativeValue',C.c_uint64)]
        size=self.native.NativeClassifierHookSize;size.argtypes=();size.restype=C.c_size_t
        self.c_context=C.create_string_buffer(size())
        patches=(Patch*len(self.patch))(*(Patch(*p) for p in self.patch))
        attach=self.native.NativeClassifierAttach
        attach.argtypes=(C.c_void_p,C.c_void_p,C.c_void_p,C.POINTER(Patch),C.c_uint32);attach.restype=C.c_int
        status=attach(uclib._handle,self.cpu._uch,self.c_context,patches,len(patches))
        if status:self.c_context=None;raise RuntimeError('Native C hook attach status '+str(status))
        self.native.NativeClassifierDetach.argtypes=(C.c_void_p,);self.native.NativeClassifierDetach.restype=C.c_int
        self.native.NativeClassifierMetric.argtypes=(C.c_void_p,C.c_uint32);self.native.NativeClassifierMetric.restype=C.c_uint64
        self.native.NativeClassifierClearMetrics.argtypes=(C.c_void_p,);self.native.NativeClassifierClearMetrics.restype=None
        self.cpu.ctl_remove_cache(probe.IMAGE+0x13b888,probe.IMAGE+0x13c880)
    def clear_metrics(self):
        self.calls=0;self.native_ms.clear()
        if self.c_context is not None:self.native.NativeClassifierClearMetrics(self.c_context)
    def native_statistics(self):
        if self.c_context is None:
            return self.calls,(sum(self.native_ms)/self.calls if self.calls else None)
        values=[self.native.NativeClassifierMetric(self.c_context,i) for i in range(5)]
        if values[3] or values[4]:raise RuntimeError('Native C hook failed: '+str(values))
        return values[0],(1000*values[1]/values[2]/values[0] if values[0] else None)
