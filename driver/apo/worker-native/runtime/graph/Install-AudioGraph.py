"""Transactional Dolby add-on for the qualified .20/.4 Qualcomm configuration.

Run with the package's native ARM64 Python. --preflight is read-only apart from
its new SSD report directory. --install saves rollback before mutation; --restore
requires that exact state directory. Never installs a kernel driver or reboots.
"""
import argparse
import ctypes as C
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import time
import winreg as R

sys.path.insert(0, str(Path(__file__).resolve().parent))
from GraphDevices import (adapter, amplifiers, component, digest, endpoint, fx_paths,
                          healthy, kernel_state, value, CLSID)
from ApoContextRollback import CONTEXT
from KnownApoContexts import capture as capture_contexts
from ProtectedApoContexts import restore as restore_contexts
from ProtectedRegistry import restore_values
import ClassicNabuRegistration as classic

PACKAGE = Path(__file__).resolve().parent.parent
os.environ['PSModulePath'] = str(Path(os.environ['SystemRoot']) / 'System32/WindowsPowerShell/v1.0/Modules')
os.environ['PYTHONDONTWRITEBYTECODE'] = '1'
SFX = '{d04e05a6-594b-4fb6-a80d-01af5eed7d1d},5'
OEM_NAMES = ['{d04e05a6-594b-4fb6-a80d-01af5eed7d1d},' + str(i) for i in (6, 7)]
FX_NAMES = ['{d04e05a6-594b-4fb6-a80d-01af5eed7d1d},0', SFX,
            '{d3993a3f-99c2-4402-b5ec-a92a0367664b},5', *OEM_NAMES]

def load(path):
    return json.loads(Path(path).read_text(encoding='utf-8-sig'))

def write(path, data):
    path = Path(path)
    temp = path.with_suffix(path.suffix + '.pending')
    temp.write_text(json.dumps(data, indent=2) + '\n', encoding='utf-8')
    os.replace(temp, path)

def execute(args, out, label, timeout=60, codes=(0,)):
    result = subprocess.run([str(a) for a in args], capture_output=True, text=True,
                            timeout=timeout, creationflags=subprocess.CREATE_NO_WINDOW)
    (out / (label + '.stdout')).write_text(result.stdout, encoding='utf-8')
    (out / (label + '.stderr')).write_text(result.stderr, encoding='utf-8')
    write(out / (label + '.exit.json'), dict(ExitCode=result.returncode))
    if result.returncode not in codes:
        raise RuntimeError(label + ' failed: ' + str(result.returncode) + ': ' + result.stdout + result.stderr)
    return result.stdout

def powershell(script, out, label, *args, **kwargs):
    return execute(['powershell.exe', '-NoProfile', '-NonInteractive', '-File', script, *args], out, label, **kwargs)

def services(out, action, label):
    result = json.loads(powershell(PACKAGE / 'graph/AudioGraphServices.ps1', out, label, '-Action', action))
    if action != 'Plan':
        expected = 'Stopped' if action == 'Stop' else 'Running'
        if any(r['Status'] != expected for r in result['After']):
            raise RuntimeError('Audio service transition did not verify')
    return result

def verify_members(root, files):
    root = root.resolve()
    for name, expected in files.items():
        path = (root / name).resolve()
        if not path.is_relative_to(root) or digest(path) != expected:
            raise RuntimeError('Package/member hash mismatch: ' + name)

def qualify(out):
    manifest = load(PACKAGE / 'GraphPackage.json')
    verify_members(PACKAGE, manifest['Files'])
    if os.environ.get('PROCESSOR_ARCHITECTURE') != 'ARM64':
        raise RuntimeError('Native ARM64 process required')
    result = json.loads(powershell(PACKAGE / 'graph/Verify-GraphPackage.ps1', out, 'Signatures', '-Root', PACKAGE))
    if not result['Passed']:
        raise RuntimeError('Signature verification failed')
    runtime = load(PACKAGE / 'runtime/Manifest.json')
    if (not runtime['AudioGraphBank'] or runtime['ApoVersion'] != manifest['ApoVersion']
            or runtime['BridgeSha256'] != manifest['Files']['APO/NabuDolbyBridge.dll']):
        raise RuntimeError('APO/backend graph identity mismatch')
    return manifest

def find_published(expected):
    matches = [p for p in (Path(os.environ['SystemRoot']) / 'INF').glob('oem*.inf') if digest(p) == expected]
    if len(matches) > 1:
        raise RuntimeError('Ambiguous published package hash')
    return matches[0].name if matches else None

def preflight(out, manifest):
    amps = amplifiers()
    if len(amps) != 4 or not all(healthy(r, '4.17.27.20') for r in amps):
        raise RuntimeError('Four healthy .20 amplifiers required; kernel installation is separate')
    row = adapter()
    apo = component()
    if not healthy(apo['Device'], '2.0.0.4') or apo['DllHash'] != manifest['BaselineApoDllSha256']:
        raise RuntimeError('Unsupported APO baseline; preserve current installation')
    if not find_published(manifest['BaselineExtensionInfSha256']):
        raise RuntimeError('Qualified baseline extension not found')
    if any(find_published(manifest['Files'][n]) for n in ('APO/NabuDolbyApo.inf', 'extension/NabuDolbyBound.inf')):
        raise RuntimeError('Candidate already staged; preserve it and inspect prior state')
    if classic.read(classic.CLASS) is not None or classic.read(classic.AUDIO) is not None:
        raise RuntimeError('Pre-existing global Nabu registration; preserve it')
    owner, paths = fx_paths(row)
    fx = [dict(Key=p, Name=n, Before=value(p, n)) for p in paths for n in FX_NAMES]
    if any(r['Before'] is not None for r in fx if r['Name'] == SFX):
        raise RuntimeError('Existing SFX would be displaced; preserve it')
    contexts = capture_contexts([p + '\\' + CONTEXT for p in paths])
    state = kernel_state()
    if state['Sha256'] != manifest['BaselineKernelSha256']:
        raise RuntimeError('Kernel differs from qualified baseline')
    plan = services(out, 'Plan', 'AudioBefore')
    if any(r['Status'] != 'Running' for r in plan['Before']):
        raise RuntimeError('Both Windows audio services must initially be running')
    return dict(Amplifiers=amps, Adapter=row, Apo=apo, Owner=owner, EndpointKey=paths[0],
                FxPaths=paths, FxValues=fx, Contexts=contexts, Kernel=state, AudioServices=plan)

def capture_rollback(out, before, manifest):
    result = {}
    packages = {'APO': before['Apo']['Device']['Inf'],
                'extension': find_published(manifest['BaselineExtensionInfSha256'])}
    for kind, inf in packages.items():
        if not re.fullmatch(r'oem\d+\.inf', inf or ''):
            raise RuntimeError('Invalid baseline INF identity')
        target = out / ('rollback-' + kind)
        target.mkdir()
        execute(['pnputil.exe', '/export-driver', inf, target], out, 'Export-' + kind)
        files = {p.relative_to(out).as_posix(): digest(p) for p in target.rglob('*') if p.is_file()}
        published_hash = digest(Path(os.environ['SystemRoot']) / 'INF' / inf)
        infs = [p for p in target.rglob('*.inf') if digest(p) == published_hash]
        if len(infs) != 1 or not files:
            raise RuntimeError('Exported baseline package did not match published INF')
        result[kind] = dict(Inf=infs[0].relative_to(out).as_posix(), Files=files, Published=inf)
    return result

def oem_preserved(before, live):
    for row in before['FxValues']:
        if row['Name'] in OEM_NAMES:
            path = live if row['Key'] == before['EndpointKey'] else row['Key']
            if value(path, row['Name']) != row['Before']:
                raise RuntimeError('OEM MFX/EFX changed; restore before audio testing')

def check_running(out, state, manifest):
    before = state['Before']
    live = endpoint(before['Owner'])
    if value(live, SFX) != [CLSID, R.REG_SZ]:
        raise RuntimeError('Speaker endpoint does not expose the candidate SFX')
    oem_preserved(before, live)
    apo = component()
    if (not healthy(apo['Device'], manifest['ApoVersion'])
            or apo['DllHash'] != manifest['Files']['APO/NabuDolbyBridge.dll']):
        raise RuntimeError('Installed APO component identity/state mismatch')
    amps = amplifiers()
    if kernel_state() != before['Kernel'] or len(amps) != 4 or not all(healthy(r, '4.17.27.20') for r in amps):
        raise RuntimeError('Amplifier driver or calibration changed')
    return dict(EndpointKey=live, Apo=apo, OemEffectsPreserved=True, CalibrationUnchanged=True,
                AudioProcessingMeasured=False, AcousticStereoProven=False)

def runtime_receipt(out):
    path = out / 'runtime-state/RuntimeInstall.json'
    return load(path) if path.exists() else None

def restore(out, state, manifest):
    errors = []
    before = state['Before']
    def attempt(name, operation):
        try:
            operation()
            state.setdefault('Restoration', {})[name] = True
        except BaseException as error:
            errors.append(name + ': ' + str(error))
        write(out / 'GraphInstall.json', state)

    def drivers():
        # Re-discover the physical device before trusting paths in an SSD receipt.
        current_adapter = adapter()
        owner, live_paths = fx_paths(current_adapter)
        if current_adapter['InstanceId'] != before['Adapter']['InstanceId'] or owner != before['Owner']:
            raise RuntimeError('Physical adapter changed since installation; preserve current state')
        expected_paths = [live_paths[0] if p == before['EndpointKey'] else p for p in before['FxPaths']]
        if set(expected_paths) != set(live_paths):
            raise RuntimeError('Captured FX paths do not match the live physical speaker')
        current_apo = component()
        accepted_hashes = {before['Apo']['DllHash'], manifest['Files']['APO/NabuDolbyBridge.dll']}
        if current_apo['DllHash'] not in accepted_hashes:
            raise RuntimeError('APO changed since installation; preserve newer code')
        current_sfx = value(live_paths[0], SFX)
        original_sfx = next(r['Before'] for r in before['FxValues'] if r['Key'] == before['EndpointKey'] and r['Name'] == SFX)
        if current_sfx != original_sfx and current_sfx != [CLSID, R.REG_SZ]:
            raise RuntimeError('SFX changed since installation; preserve other effects')
        services(out, 'Stop', 'RollbackStop')
        for kind, name in (('extension', 'extension/NabuDolbyBound.inf'), ('APO', 'APO/NabuDolbyApo.inf')):
            if not state.get('Attempted-' + kind):
                continue
            published = find_published(manifest['Files'][name])
            if published:
                execute(['pnputil.exe', '/delete-driver', published, '/uninstall'], out,
                        'Remove-' + kind, codes=(0, 3010, 259))
            saved = state['Rollback'][kind]
            verify_members(out, saved['Files'])
            execute(['pnputil.exe', '/add-driver', out / saved['Inf'], '/install'], out,
                    'Restore-' + kind, codes=(0, 3010, 259))
        if state.get('ClassicAttempted'):
            classic.restore(state['ClassicPlan'])
        # Let Windows publish its new endpoint before restoring captured metadata.
        services(out, 'Start', 'RollbackEnumerate')
        live = endpoint(before['Owner'])
        services(out, 'Stop', 'RollbackMetadataStop')
        rows = [dict(r, Key=live if r['Key'] == before['EndpointKey'] else r['Key']) for r in before['FxValues']]
        paths = [live if p == before['EndpointKey'] else p for p in before['FxPaths']]
        # OEM values were never owned by this installer. Verify, do not overwrite.
        oem_preserved(before, live)
        owned = [r for r in rows if r['Name'] not in OEM_NAMES]
        restore_values(owned, paths)
        restore_contexts(before['Contexts'], before['EndpointKey'], live)
        if any(value(r['Key'], r['Name']) != r['Before'] for r in rows):
            raise RuntimeError('Restored FX does not match captured state')

    if state.get('Attempted-APO') or state.get('Attempted-extension') or state.get('ClassicAttempted'):
        attempt('DriversAndMetadata', drivers)
        if errors:
            # Keep the candidate backend available if its graph could not be
            # safely removed. Recover the Windows services for manual inspection.
            attempt('AudioServices', lambda: services(out, 'Start', 'RollbackAudioStart'))
            state['CleanupErrors'] = errors
            state['Phase'] = 'RestoreAttention'
            write(out / 'GraphInstall.json', state)
            raise RuntimeError('; '.join(errors))

    def backend():
        receipt = runtime_receipt(out)
        if receipt and receipt['Installed'] and not receipt['Restored']:
            powershell(PACKAGE / 'Install-Runtime.ps1', out, 'RestoreRuntime',
                       '-StateDirectory', out / 'runtime-state', '-Restore')
        receipt = runtime_receipt(out)
        if receipt and receipt.get('CleanupError'):
            raise RuntimeError(receipt['CleanupError'])
    attempt('Backend', backend)
    attempt('AudioServices', lambda: services(out, 'Start', 'RollbackAudioStart'))

    def verify():
        deadline = time.monotonic() + 10
        while True:
            apo = component()
            if (healthy(apo['Device'], before['Apo']['Device']['Version'])
                    and apo['DllHash'] == before['Apo']['DllHash']):
                break
            if time.monotonic() >= deadline:
                raise RuntimeError('Baseline APO did not return healthy')
            time.sleep(.2)
        if kernel_state() != before['Kernel'] or len(amplifiers()) != 4 or not all(healthy(r, '4.17.27.20') for r in amplifiers()):
            raise RuntimeError('Baseline amplifiers/calibration did not verify')
        live = endpoint(before['Owner'])
        if any(value(live if r['Key'] == before['EndpointKey'] else r['Key'], r['Name']) != r['Before'] for r in before['FxValues']):
            raise RuntimeError('Baseline FX readback failed')
    attempt('Readback', verify)
    state['CleanupErrors'] = errors
    state['Phase'] = 'Restored' if not errors else 'RestoreAttention'
    write(out / 'GraphInstall.json', state)
    if errors:
        raise RuntimeError('; '.join(errors))

def install(out, manifest, before, fault_after=None):
    state = dict(Phase='Saved', Before=before, Rollback=capture_rollback(out, before, manifest),
                 PackageManifestSha256=digest(PACKAGE / 'GraphPackage.json'), Error=None,
                 CleanupErrors=[], RebootPerformed=False, KernelInstalled=False,
                 AudioProcessingMeasured=False, AcousticStereoProven=False)
    write(out / 'GraphInstall.json', state)
    try:
        powershell(PACKAGE / 'Install-Runtime.ps1', out, 'InstallRuntime',
                   '-StateDirectory', out / 'runtime-state', '-StartMode', 'Automatic')
        receipt = runtime_receipt(out)
        if not receipt['ServiceRunning'] or receipt['Error'] or receipt['CleanupError']:
            raise RuntimeError('Runtime receipt is not healthy')
        state['RuntimeAutomatic'] = True
        write(out / 'GraphInstall.json', state)
        if fault_after == 'runtime':
            raise RuntimeError('Explicit qualification fault after runtime installation')
        services(out, 'Stop', 'InstallStop')
        state['Attempted-APO'] = True
        write(out / 'GraphInstall.json', state)
        execute(['pnputil.exe', '/add-driver', PACKAGE / 'APO/NabuDolbyApo.inf', '/install'], out, 'InstallAPO')
        apo = component()
        if not healthy(apo['Device'], manifest['ApoVersion']) or apo['DllHash'] != manifest['Files']['APO/NabuDolbyBridge.dll']:
            raise RuntimeError('Candidate APO did not bind healthy')
        state['ClassicPlan'] = classic.prepare(apo['Device']['ClassKey'], apo['DllHash'])
        state['ClassicAttempted'] = True
        write(out / 'GraphInstall.json', state)
        classic.apply(state['ClassicPlan'])
        if fault_after == 'apo':
            raise RuntimeError('Explicit qualification fault after APO registration')
        state['Attempted-extension'] = True
        write(out / 'GraphInstall.json', state)
        execute(['pnputil.exe', '/add-driver', PACKAGE / 'extension/NabuDolbyBound.inf', '/install'], out, 'InstallExtension')
        if fault_after == 'extension':
            raise RuntimeError('Explicit qualification fault after extension installation')
        services(out, 'Start', 'InstallStart')
        deadline = time.monotonic() + 10
        while value(endpoint(before['Owner']), SFX) != [CLSID, R.REG_SZ]:
            if time.monotonic() >= deadline:
                raise RuntimeError('Endpoint did not publish bound SFX')
            time.sleep(.2)
        state['LiveReadback'] = check_running(out, state, manifest)
        state['Phase'] = 'InstalledAwaitingPlaybackQualification'
        write(out / 'GraphInstall.json', state)
        return state
    except BaseException as error:
        state['Error'] = type(error).__name__ + ': ' + str(error)
        write(out / 'GraphInstall.json', state)
        restore(out, state, manifest)
        raise

def main():
    args = argparse.ArgumentParser(description=__doc__)
    action = args.add_mutually_exclusive_group(required=True)
    action.add_argument('--preflight', action='store_true')
    action.add_argument('--install', action='store_true')
    action.add_argument('--restore', action='store_true')
    args.add_argument('--state-directory', type=Path, required=True)
    args.add_argument('--fault-after', choices=('runtime', 'apo', 'extension'),
                      help='Qualification only: deliberately fail and verify rollback')
    opt = args.parse_args()
    out = opt.state_directory.resolve()
    if out.drive.lower() == os.environ['SystemDrive'].lower() or not out.parent.is_dir():
        raise RuntimeError('Use a dedicated state directory on an available SSD')
    if not opt.preflight and not C.windll.shell32.IsUserAnAdmin():
        raise RuntimeError('Elevated native ARM64 Python required')
    if opt.restore:
        state = load(out / 'GraphInstall.json')
        if state['Phase'] == 'Restored':
            raise RuntimeError('Already restored; preserve its receipt')
        manifest = qualify(out)
        if digest(PACKAGE / 'GraphPackage.json') != state['PackageManifestSha256']:
            raise RuntimeError('Restore requires the unchanged original package')
        restore(out, state, manifest)
    else:
        out.mkdir()  # Preserve all prior attempts.
        manifest = qualify(out)
        before = preflight(out, manifest)
        write(out / 'Preflight.json', dict(Passed=True, Details=before, AudioOpened=False, DriverChanged=False))
        if opt.install:
            state = install(out, manifest, before, opt.fault_after)
        else:
            state = dict(Phase='PreflightPassed', AudioOpened=False, DriverChanged=False)
    print(json.dumps({k: state[k] for k in ('Phase', 'Error', 'CleanupErrors', 'KernelInstalled', 'AudioProcessingMeasured') if k in state}))

if __name__ == '__main__':
    main()
