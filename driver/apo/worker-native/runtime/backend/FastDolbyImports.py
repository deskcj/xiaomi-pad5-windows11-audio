"""Bind native implementations of frequent imports used by the Dolby guest."""

import ctypes

from unicorn.unicorn_py3.unicorn import uclib

NAMES = ("memcpy", "memmove", "memset", "frexpf", "sincosf")


class FastDolbyImports:
    def __init__(self, library, cpu, address):
        self.library = ctypes.WinDLL(str(library.resolve(strict=True)))
        context_size = self.library.nabu_import_context_size
        context_size.argtypes = ()
        context_size.restype = ctypes.c_size_t
        self.context = (ctypes.c_uint64 * ((context_size() + 7) // 8))()

        attach = self.library.nabu_add_imports
        attach.argtypes = (ctypes.c_void_p, ctypes.c_void_p, ctypes.c_void_p, ctypes.c_uint64)
        attach.restype = ctypes.c_int
        result = attach(uclib._handle, cpu._uch, self.context, address)
        if result:
            raise RuntimeError(f"Native Dolby import binding failed with Unicorn status {result}")

        self.read_error = self.library.nabu_import_error
        self.read_error.argtypes = (ctypes.c_void_p,)
        self.read_error.restype = ctypes.c_int

    def set_heap(self, guest_start, byte_count, host_start):
        configure = self.library.nabu_set_guest_heap
        configure.argtypes = (ctypes.c_void_p, ctypes.c_uint64, ctypes.c_size_t, ctypes.c_void_p)
        configure.restype = None
        configure(self.context, guest_start, byte_count, host_start)

    def check(self):
        result = self.read_error(self.context)
        if result:
            raise RuntimeError(f"Native Dolby import failed with Unicorn status {result}")
