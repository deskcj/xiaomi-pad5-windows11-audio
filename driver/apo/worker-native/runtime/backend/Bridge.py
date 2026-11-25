"""Typed stereo transport records and bounded counters."""
import ctypes as C
from pathlib import Path
ROOT=Path(__file__).resolve().parent
class Frame(C.Structure):
    _fields_ = [('left', C.c_float), ('right', C.c_float)]

class Block(C.Structure):
    _fields_ = [('first_frame', C.c_uint64), ('generation', C.c_uint32),
                ('count', C.c_uint32), ('frames', Frame * 1024)]

def metrics(lib, view):
    names = ('ApoClaim', 'WorkerClaim', 'Generation', 'Stop', 'RequestsDropped',
             'WorkerGaps', 'BlocksReplied', 'WorkerReady', 'ApoPid', 'InitKind', 'LockError', 'Callbacks')
    return {name: lib.NabuTransportMetric(view, index) for index, name in enumerate(names)} | {
        name: lib.NabuTransportFrames(view, i) for i, name in enumerate(('FramesProcessed','FramesWet','FramesDry'))}
assert C.sizeof(Block)==8208
