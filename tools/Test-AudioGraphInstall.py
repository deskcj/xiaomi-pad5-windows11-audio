"""Offline safety contracts for graph deployment; no driver/registry writes."""
import copy
import importlib.util
import json
from pathlib import Path
import sys
import tempfile

repo = Path(__file__).resolve().parent.parent
source = repo / 'driver/apo/worker-native/runtime/graph/Install-AudioGraph.py'
sys.path.insert(0, str(source.parent))
spec = importlib.util.spec_from_file_location('graph_installer', source)
graph = importlib.util.module_from_spec(spec)
spec.loader.exec_module(graph)

checks = []
def check(name, function):
    function()
    checks.append(dict(Name=name, Passed=True))

def rejects(operation, phrase):
    try:
        operation()
    except (RuntimeError, ValueError) as error:
        if phrase not in str(error):
            raise AssertionError(str(error))
    else:
        raise AssertionError('Unsafe input was accepted')

with tempfile.TemporaryDirectory(prefix='nabu-graph-contract-') as folder:
    root = Path(folder)
    check('escaping member rejected before reading', lambda: rejects(
        lambda: graph.verify_members(root, {'../foreign.bin': '0'*64}), 'hash mismatch'))

    # All native mutation entry points are replaced with an audit-only recorder.
    calls = []
    graph.services = lambda out, action, label: calls.append(('services', action))
    graph.execute = lambda *args, **kwargs: calls.append(('execute', args))
    graph.powershell = lambda *args, **kwargs: calls.append(('powershell', args))
    graph.runtime_receipt = lambda out: {'Installed': True, 'Restored': False}
    graph.write = lambda *args: None
    owner = '{1}.AUDD\\VEN_QCOM&DEV_052C&SUBSYS_MTPA8150\\test'
    paths = ['speaker-endpoint', 'speaker-audio-interface', 'speaker-topology-interface']
    adapter = dict(InstanceId=owner[4:])
    baseline = dict(Adapter=adapter, Owner=owner, EndpointKey=paths[0], FxPaths=paths,
                    Apo=dict(Device={'Version':'2.0.0.4'}, DllHash='baseline'),
                    FxValues=[dict(Key=paths[0],Name=graph.SFX,Before=None)], Kernel={})
    manifest = {'Files': {'APO/NabuDolbyBridge.dll':'candidate'}}
    graph.adapter = lambda: copy.deepcopy(adapter)
    graph.fx_paths = lambda row: (owner, paths[:])
    graph.component = lambda: {'DllHash':'candidate'}
    graph.value = lambda path, name: [graph.CLSID, graph.R.REG_SZ]

    def refused_rollback(name, change, phrase):
        original = {key:getattr(graph,key) for key in change}
        calls.clear()
        for key, val in change.items():
            setattr(graph, key, val)
        state = dict(Before=copy.deepcopy(baseline), Rollback={}, **{'Attempted-APO':True})
        try:
            rejects(lambda: graph.restore(root, state, manifest), phrase)
            # Recovering Windows audio is permitted; candidate backend and all
            # packages must be preserved when the graph lease is not satisfied.
            assert not any(c[0] in ('execute','powershell') for c in calls), calls
            assert not any(c == ('services','Stop') for c in calls), calls
            assert state['Phase'] == 'RestoreAttention'
            checks.append(dict(Name=name,Passed=True))
        finally:
            for key, val in original.items():
                setattr(graph, key, val)

    refused_rollback('changed physical adapter retained', {'adapter':lambda:{'InstanceId':'foreign'}}, 'adapter changed')
    refused_rollback('foreign captured interface paths retained', {'fx_paths':lambda row:(owner,paths[:2]+['foreign-interface'])}, 'paths do not match')
    refused_rollback('newer unrelated APO retained', {'component':lambda:{'DllHash':'foreign'}}, 'APO changed')
    refused_rollback('other SFX retained', {'value':lambda path,name:['foreign-effect',graph.R.REG_SZ]}, 'SFX changed')

    graph.endpoint = lambda owner: paths[0]
    graph.oem_preserved = lambda *args: None
    graph.component = lambda: {'DllHash':'candidate','Device':{}}
    graph.healthy = lambda row, version=None: True
    graph.kernel_state = lambda: {}
    manifest.update(ApoVersion='2.0.0.21')
    for count in (0,3,5):
        graph.amplifiers = lambda count=count: [{}]*count
        check('amplifier count '+str(count)+' rejected', lambda: rejects(
            lambda: graph.check_running(root,{'Before':baseline},manifest), 'Amplifier driver'))
print(json.dumps(dict(Passed=True,AudioOpened=False,SystemChanged=False,Checks=checks),indent=2))
