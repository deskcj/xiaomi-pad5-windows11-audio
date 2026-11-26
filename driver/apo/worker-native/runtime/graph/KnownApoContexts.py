"""Preserve known pre-existing Nabu settings; remove only captured-new contexts."""
import winreg as R
from ApoContextRollback import read_tree,plan_cleanup,validate_path
def normalized(tree):
    if tree is None:return None
    return dict(Values={k.lower():v for k,v in tree['Values'].items()},Children={k.lower():normalized(v) for k,v in tree['Children'].items()})
def capture(paths):
    result={}
    for path in paths:
        validate_path(path);tree=read_tree(path)
        plan_cleanup(tree) # Reject unexpected content without modifying it.
        result[path]=tree
    return result
def restore(snapshot,old_endpoint_key,live_endpoint_key):
    plans=[]
    for original,before in snapshot.items():
        path=live_endpoint_key+original[len(old_endpoint_key):] if original.startswith(old_endpoint_key+'\\') else original
        validate_path(path);current=read_tree(path)
        if before is not None:
            if normalized(current)!=normalized(before):raise RuntimeError('Existing Nabu context changed; preserve it for inspection')
            continue
        plans.append((path,plan_cleanup(current)))
    for path,operations in plans:
        for operation in operations:
            target=path+('\\'+operation[1] if operation[1] else '')
            if operation[0]=='value':
                with R.OpenKey(R.HKEY_LOCAL_MACHINE,target,0,R.KEY_QUERY_VALUE|R.KEY_SET_VALUE) as key:
                    value,kind=R.QueryValueEx(key,operation[2])
                    if [value,kind]!=operation[3]:raise RuntimeError('Context value changed during rollback')
                    R.DeleteValue(key,operation[2])
            else:
                with R.OpenKey(R.HKEY_LOCAL_MACHINE,target) as key:
                    children,nvalues,_=R.QueryInfoKey(key)
                    if children or nvalues:raise RuntimeError('Context no longer empty')
                R.DeleteKey(R.HKEY_LOCAL_MACHINE,target)
    for original,before in snapshot.items():
        path=live_endpoint_key+original[len(old_endpoint_key):] if original.startswith(old_endpoint_key+'\\') else original
        if normalized(read_tree(path))!=normalized(before):raise RuntimeError('Context rollback did not match snapshot')
    return dict(Verified=True,ExistingContextsPreserved=sum(v is not None for v in snapshot.values()),NewContextsRemoved=sum(v is None for v in snapshot.values()))
