"""Read-only device and speaker discovery; no research-tree dependency."""
import ctypes as C
from pathlib import Path
import hashlib
import os
import winreg as R

CLSID = '{9DF8834A-209E-4CC1-BC73-BB4353ADFA9F}'
ADAPTER = r'AUDD\VEN_QCOM&DEV_052C&SUBSYS_MTPA8150'
CLASS_ROOT = r'SYSTEM\CurrentControlSet\Control\Class'
ENUM_ROOT = r'SYSTEM\CurrentControlSet\Enum'
RENDER_ROOT = r'SOFTWARE\Microsoft\Windows\CurrentVersion\MMDevices\Audio\Render'
OWNER = '{b3f8fa53-0004-438e-9003-51a46e139bfc},2'
LABEL = '{a45c254e-df1c-4efd-8020-67d146a850e0},2'
cfg = C.WinDLL('cfgmgr32')
cfg.CM_Locate_DevNodeW.argtypes = (C.POINTER(C.c_uint32), C.c_wchar_p, C.c_uint32)
cfg.CM_Get_DevNode_Status.argtypes = (C.POINTER(C.c_uint32), C.POINTER(C.c_uint32), C.c_uint32, C.c_uint32)

def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()

def values(path):
    try:
        key = R.OpenKey(R.HKEY_LOCAL_MACHINE, path)
    except FileNotFoundError:
        return None
    with key:
        result = {}
        for i in range(R.QueryInfoKey(key)[1]):
            name, value, kind = R.EnumValue(key, i)
            result[name] = [value.hex() if isinstance(value, bytes) else value, kind]
        return result

def subkeys(path):
    try:
        key = R.OpenKey(R.HKEY_LOCAL_MACHINE, path)
    except FileNotFoundError:
        return []
    with key:
        return [R.EnumKey(key, i) for i in range(R.QueryInfoKey(key)[0])]

def value(path, name):
    return (values(path) or {}).get(name)

def device(instance):
    dev = C.c_uint32()
    status = C.c_uint32()
    problem = C.c_uint32()
    result = cfg.CM_Locate_DevNodeW(C.byref(dev), instance, 0)
    if not result:
        result = cfg.CM_Get_DevNode_Status(C.byref(status), C.byref(problem), dev, 0)
    props = values(ENUM_ROOT + '\\' + instance) or {}
    driver = props.get('Driver', [None])[0]
    registration = CLASS_ROOT + '\\' + driver if driver else None
    driver_props = values(registration) if registration else {}
    return dict(InstanceId=instance, ConfigResult=result, Started=bool(status.value & 8),
                Problem=problem.value, NeedsRestart=bool(status.value & 0x100),
                Version=(driver_props or {}).get('DriverVersion', [None])[0],
                Inf=(driver_props or {}).get('InfPath', [None])[0], ClassKey=registration,
                HardwareIds=props.get('HardwareID'), CompatibleIds=props.get('CompatibleIDs'))

def healthy(row, version=None):
    return (row['ConfigResult'] == 0 and row['Started'] and not row['Problem']
            and not row['NeedsRestart'] and (version is None or row['Version'] == version))

def amplifiers():
    rows = []
    for hardware in (r'ACPI\10133541', r'ACPI\CSCL3541'):
        rows += [device(hardware + '\\' + s) for s in subkeys(ENUM_ROOT + '\\' + hardware)]
    return [r for r in rows if r['ConfigResult'] == 0]

def component():
    root = r'SWD\DRIVERENUM'
    rows = [device(root + '\\' + s) for s in subkeys(ENUM_ROOT + '\\' + root)
            if s.upper().startswith('{FD13C2DF-5315-4F4F-9556-F78FB784A12A}#NABUDOLBYGLOBAL')]
    rows = [r for r in rows if r['ConfigResult'] == 0]
    if len(rows) != 1:
        raise RuntimeError('Expected one present Nabu APO component')
    row = rows[0]
    server = value(row['ClassKey'] + '\\Classes\\CLSID\\' + CLSID + '\\InProcServer32', '')
    if not server:
        raise RuntimeError('Component COM server missing')
    path = Path(os.path.expandvars(server[0])).resolve()
    store = (Path(os.environ['SystemRoot']) / 'System32/DriverStore/FileRepository').resolve()
    if not path.is_relative_to(store):
        raise RuntimeError('Component DLL is outside DriverStore')
    return dict(Device=row, Dll=str(path), DllHash=digest(path))

def adapter():
    rows = [device(ADAPTER + '\\' + s) for s in subkeys(ENUM_ROOT + '\\' + ADAPTER)]
    rows = [r for r in rows if r['ConfigResult'] == 0]
    if len(rows) != 1 or not healthy(rows[0], '1.0.2100.0'):
        raise RuntimeError('Expected one healthy supported Qualcomm adapter')
    return rows[0]

def endpoint(owner):
    matches = []
    for name in subkeys(RENDER_ROOT):
        path = RENDER_ROOT + '\\' + name
        props = path + '\\Properties'
        actual_owner = value(props, OWNER)
        if (actual_owner and actual_owner[0].lower() == owner.lower()
                and value(props, LABEL) == ['Speakers', R.REG_SZ]
                and value(path, 'DeviceState') == [1, R.REG_DWORD]):
            matches.append(path + '\\FxProperties')
    if len(matches) != 1:
        raise RuntimeError('Expected one active speaker endpoint for captured physical owner')
    return matches[0]

def fx_paths(row):
    owner = '{1}.' + row['InstanceId']
    result = [endpoint(owner)]
    symbolic = '##?#' + row['InstanceId'].replace('\\', '#') + '#'
    for category in ('{6994AD04-93EF-11D0-A3CC-00A0C9223196}', '{DDA54A40-1E4C-11D1-A050-405705C10000}'):
        parent = r'SYSTEM\CurrentControlSet\Control\DeviceClasses' + '\\' + category
        matches = [parent + '\\' + n + r'\#TopologySpeaker\Device Parameters\FX\0'
                   for n in subkeys(parent) if n.lower().startswith(symbolic.lower())]
        matches = [p for p in matches if values(p) is not None]
        if len(matches) != 1:
            raise RuntimeError('Ambiguous or missing physical TopologySpeaker interface')
        result += matches
    return owner, result

def kernel_state():
    service = r'SYSTEM\CurrentControlSet\Services\cs35l41'
    path = value(service, 'ImagePath')[0]
    path = path.replace('\\SystemRoot', os.environ['SystemRoot'], 1)
    return dict(Sha256=digest(path), Parameters=values(service + '\\Parameters'))
