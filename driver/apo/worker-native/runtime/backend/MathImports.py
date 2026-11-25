"""Previously qualified native import helpers for the original DAP code."""
from pathlib import Path
def enable():
    import FastDolbyImports
    FastDolbyImports.NAMES=('memcpy','memmove','memset','frexpf','sincosf','logf','lrintf','pthread_mutex_trylock','pthread_mutex_unlock')
    return Path(__file__).resolve().parent/'native/DolbyImportMath.dll'
