"""Narrow rollback for a newly created Nabu APO settings context.
Capture and planning are read-only. Restore requires an already elevated token.
Pre-existing contexts or unexpected new content are preserved, never overwritten.
"""
import ctypes as C
import winreg as R

CONTEXT = '{9df8834a-209e-4cc1-bc73-bb4353adfa9f}'
ASSOCIATION = '{d04e05a6-594b-4fb6-a80d-01af5eed7d1d},0'
BANK = '{1a5bcc9e-61ee-41b7-9dc2-1b580dcafa1b},1'
ENABLED = '{1a5bcc9e-61ee-41b7-9dc2-1b580dcafa1b},2'
SPEAKER = '{DFF21CE1-F70F-11D0-B917-00A0C9223196}'
DEFAULT_BANK = 'Global\\NabuDolbyBank-9df8834a-209e-4cc1-bc73-bb4353adfa9f'
_captured_absent = set()

def validate_path(path):
    lower = path.lower()
    device = lower.startswith('system\\currentcontrolset\\control\\deviceclasses\\') and '\\#topologyspeaker\\device parameters\\fx\\0\\' in lower
    endpoint = lower.startswith('software\\microsoft\\windows\\currentversion\\mmdevices\\audio\\render\\') and '\\fxproperties\\' in lower
    if not (device or endpoint) or lower.rsplit('\\', 1)[-1] != CONTEXT:
        raise ValueError('Unexpected context path')

def read_tree(path):
    try: key = R.OpenKey(R.HKEY_LOCAL_MACHINE, path, 0, R.KEY_READ)
    except FileNotFoundError: return None
    with key:
        children, values, _ = R.QueryInfoKey(key)
        data = {}
        for i in range(values):
            name, value, kind = R.EnumValue(key, i)
            data[name] = [value, kind]
        sub = {}
        for i in range(children):
            name = R.EnumKey(key, i)
            sub[name] = read_tree(path + '\\' + name)
    return dict(Values=data, Children=sub)

def capture_absent(paths):
    for path in paths:
        validate_path(path)
        if read_tree(path) is not None:
            raise RuntimeError('Nabu context already exists; retain its contents and abort this trial')
    _captured_absent.update(path.lower() for path in paths)
    return list(paths)

def plan_cleanup(tree, relative=''):
    if tree is None: return []
    normalized = relative.lower()
    if normalized not in ('', 'default', 'user', 'volatile'):
        raise RuntimeError('Unexpected context child; preserve contents')
    expected = {ASSOCIATION: [SPEAKER, R.REG_SZ]} if not relative else {BANK: [DEFAULT_BANK, R.REG_SZ]} if normalized == 'default' else {}
    if normalized == 'user':
        for name,value in tree['Values'].items():
            if name.lower()==ENABLED and value in ([0,R.REG_DWORD],[1,R.REG_DWORD]):expected[ENABLED]=value
    operations = []
    for name, value in tree['Values'].items():
        if name.lower() not in expected or value != expected[name.lower()]:
            raise RuntimeError('Unexpected context value; preserve contents')
        operations.append(('value', relative, name, value))
    for name, child in tree['Children'].items():
        if relative or name.lower() not in ('default', 'user', 'volatile'):
            raise RuntimeError('Unexpected context hierarchy; preserve contents')
        operations += plan_cleanup(child, name)
    operations.append(('key', relative))
    return operations

def require_captured(path):
    validate_path(path)
    if path.lower() not in _captured_absent:
        raise RuntimeError('Context was not captured absent in this process; preserve contents')

def restore_new_contexts(paths):
    if not C.windll.shell32.IsUserAnAdmin():
        raise RuntimeError('Already elevated administrator token required')
    plans = []
    for path in paths:
        require_captured(path)
        plans.append((path, plan_cleanup(read_tree(path))))
    for path, operations in plans:
        for operation in operations:
            target = path + ('\\' + operation[1] if operation[1] else '')
            if operation[0] == 'value':
                with R.OpenKey(R.HKEY_LOCAL_MACHINE, target, 0, R.KEY_QUERY_VALUE | R.KEY_SET_VALUE) as key:
                    value, kind = R.QueryValueEx(key, operation[2])
                    if [value, kind] != operation[3]:
                        raise RuntimeError('Context changed during rollback; preserve new value')
                    R.DeleteValue(key, operation[2])
            else:
                with R.OpenKey(R.HKEY_LOCAL_MACHINE, target, 0, R.KEY_READ) as key:
                    children, values, _ = R.QueryInfoKey(key)
                    if children or values: raise RuntimeError('Context is no longer empty; preserve it')
                R.DeleteKey(R.HKEY_LOCAL_MACHINE, target)
    if any(read_tree(path) is not None for path in paths):
        raise RuntimeError('New Nabu context remains after rollback')
    _captured_absent.difference_update(path.lower() for path in paths)
    return dict(NewContextRemoved=True,PreExistingContextTouched=False)
