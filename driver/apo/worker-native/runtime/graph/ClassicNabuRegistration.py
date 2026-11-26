"""Owned classic COM registration diagnostic for the installed signed Nabu DLL.
Does not change protected-audio settings or an existing COM registration.
"""
from pathlib import Path
import hashlib,os,winreg as R
CLSID='{9DF8834A-209E-4CC1-BC73-BB4353ADFA9F}'
CLASS=r'SOFTWARE\Classes\CLSID'+'\\'+CLSID
AUDIO=r'SOFTWARE\Classes\AudioEngine\AudioProcessingObjects'+'\\'+CLSID
def read(path):
    try:key=R.OpenKey(R.HKEY_LOCAL_MACHINE,path)
    except FileNotFoundError:return None
    with key:
        children,values,_=R.QueryInfoKey(key)
        entries={}
        for i in range(values):name,value,kind=R.EnumValue(key,i);entries[name]=[value,kind]
        return dict(Values=entries,Children={R.EnumKey(key,i):read(path+'\\'+R.EnumKey(key,i)) for i in range(children)})
def prepare(component_key,dll_hash):
    assert component_key.startswith(r'SYSTEM\CurrentControlSet\Control\Class'+'\\')
    assert read(CLASS) is None and read(AUDIO) is None
    com=read(component_key+r'\Classes\CLSID'+'\\'+CLSID)
    engine=read(component_key+r'\AudioEngine\AudioProcessingObjects'+'\\'+CLSID)
    assert com and engine,'Installed component registration is missing'
    assert {k.lower() for k in com['Children']}=={'inprocserver32'} and not engine['Children'],'Unexpected component registration shape'
    server=next(iter(com['Children'].values()))['Values'][''][0]
    path=Path(os.path.expandvars(server));assert str(path).lower().startswith('c:\\windows\\system32\\driverstore\\filerepository\\'),'COM server is not in DriverStore'
    assert hashlib.sha256(path.read_bytes()).hexdigest()==dll_hash
    return {CLASS:com,AUDIO:engine}
def subset(actual,expected):
    if actual is None:return True
    values={k.lower():v for k,v in expected['Values'].items()}
    children={k.lower():v for k,v in expected['Children'].items()}
    return all(values.get(k.lower())==v for k,v in actual['Values'].items()) and all(k.lower() in children and subset(v,children[k.lower()]) for k,v in actual['Children'].items())
def apply(plan):
    assert set(plan)=={CLASS,AUDIO} and read(CLASS) is None and read(AUDIO) is None
    def write(path,tree):
        with R.CreateKeyEx(R.HKEY_LOCAL_MACHINE,path,0,R.KEY_WRITE) as key:
            for name,(value,kind) in tree['Values'].items():R.SetValueEx(key,name,0,kind,value)
        for name,child in tree['Children'].items():write(path+'\\'+name,child)
    for path,tree in plan.items():write(path,tree)
    assert all(read(p)==tree for p,tree in plan.items())
def restore(plan):
    assert set(plan)=={CLASS,AUDIO}
    for path,expected in plan.items():
        if not subset(read(path),expected):raise RuntimeError('Classic registration changed; retain unexpected content')
    def erase(path,expected):
        actual=read(path)
        if actual is None:return
        if not subset(actual,expected):raise RuntimeError('Classic registration changed during rollback')
        for name in actual['Children']:
            child=next(v for k,v in expected['Children'].items() if k.lower()==name.lower());erase(path+'\\'+name,child)
        with R.OpenKey(R.HKEY_LOCAL_MACHINE,path,0,R.KEY_SET_VALUE|R.KEY_QUERY_VALUE) as key:
            for name,value in actual['Values'].items():
                if list(R.QueryValueEx(key,name))!=value:raise RuntimeError('Registration value changed')
                R.DeleteValue(key,name)
        R.DeleteKey(R.HKEY_LOCAL_MACHINE,path)
    for path,expected in plan.items():erase(path,expected)
    assert all(read(p) is None for p in plan)
    return dict(Verified=True,PreExistingRegistrationTouched=False,ProtectedAudioSettingsChanged=False)
