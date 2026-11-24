"""Correlate APO identity events with audiodg counters supplied by a live trial.

This module performs no tracing, installation or audio I/O. Input correlation is
not an acoustic measurement or authentication of arbitrary user-supplied JSON.
"""
import re

PROVIDER='87ad024a-44cc-4b70-8a14-f967501645b2'
MESSAGE=re.compile(
    r'^NabuApo build=(\S+) source=([0-9a-f]{64}) phase=(\w+) hr=0x([0-9a-fA-F]{8}) '
    r'pid=(\d+) init=(\d+) mode=(\d+) transport=(\d+) delay=(\d+) logger=0x[0-9a-fA-F]{8} queue=(\S+) epoch=(\d+)$')

def correlate(events,metrics,audiodg_pids,fingerprint,build='2.0.0.7-lab'):
    """Require matching writer PID, source, initialization, lock and wet counters."""
    pids={int(pid) for pid in audiodg_pids}
    initialized=set();locked=set();accepted=[]
    for event in events:
        if event.get('Provider','').lower()!=PROVIDER:continue
        match=MESSAGE.fullmatch(event.get('Message',''))
        if not match:continue
        tag,source,phase,status,pid,kind,mode,transport,delay,queue,epoch=match.groups()
        pid=int(pid)
        if tag!=build or source!=fingerprint or pid not in pids or int(event.get('ProcessId',-1))!=pid:continue
        if int(status,16)!=0 or int(mode)!=0 or int(kind) not in (2,3,4) or int(transport)!=1:continue
        accepted.append(dict(Pid=pid,Phase=phase,DelayFrames=int(delay),TransportName=queue,Generation=int(epoch)))
        if phase=='Initialize':initialized.add((pid,queue))
        elif phase=='LockForProcess' and int(epoch)>0:locked.add((pid,queue,int(epoch)))
    active={(int(row.get('ApoPid',-1)),row.get('TransportName',''),int(row.get('Generation',-1))) for row in metrics
        if int(row.get('Callbacks',0))>0 and int(row.get('FramesWet',0))>0 and int(row.get('FramesProcessed',0))>0}
    matched={key for key in locked&active if key[:2] in initialized}
    return dict(InitializationCorrelated=bool(initialized),StreamLockCorrelated=bool(locked),
        ZeroInputProcessingCorrelated=bool(matched),MatchedAudiodgPids=sorted({key[0] for key in matched}),
        MatchedStreams=[dict(Pid=pid,TransportName=queue,Generation=epoch) for pid,queue,epoch in sorted(matched)],
        AcceptedEvents=accepted,AcousticLrProven=False,HyperOsQualityProven=False,
        Interpretation='Correlation of supplied trace/counter records only; real trial provenance must be established by the collector.')
