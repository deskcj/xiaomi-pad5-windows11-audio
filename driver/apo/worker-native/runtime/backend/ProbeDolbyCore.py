"""Exercise the captured Nabu Dolby software core in an isolated ARM64 guest."""

import ctypes
import hashlib
import io
import math
from pathlib import Path
import struct
import sys

HERE = Path(__file__).resolve().parent
ROOT = HERE
sys.path.insert(0, str(HERE / "deps"))
from elftools.elf.elffile import ELFFile
from unicorn import Uc, UcError, UC_ARCH_ARM64, UC_HOOK_CODE, UC_MODE_ARM, UC_TLB_VIRTUAL
from unicorn.arm64_const import (
    UC_ARM64_REG_PC, UC_ARM64_REG_Q0, UC_ARM64_REG_Q1, UC_ARM64_REG_SP, UC_ARM64_REG_TPIDR_EL0,
    UC_ARM64_REG_X0, UC_ARM64_REG_X1, UC_ARM64_REG_X2, UC_ARM64_REG_X3,
    UC_ARM64_REG_X4, UC_ARM64_REG_X5, UC_ARM64_REG_X6, UC_ARM64_REG_X7,
    UC_ARM64_REG_X8, UC_ARM64_REG_X30,
)

LIBRARY = (HERE / "resources/libswdap.so")
SHA256 = "207d04317997bbca253157d6038505605335a5982af39d465b249fb52a590539"
PAGE = 4096
IMAGE = 0x10000000
IMPORTS = 0x20000000
RETURN = 0x20010000
STACK = 0x30000000
STACK_BYTES = 0x80000
TLS = STACK + STACK_BYTES
HEAP = 0x40000000
HEAP_BYTES = 0x1000000
ARGUMENTS = (UC_ARM64_REG_X0, UC_ARM64_REG_X1, UC_ARM64_REG_X2, UC_ARM64_REG_X3,
             UC_ARM64_REG_X4, UC_ARM64_REG_X5, UC_ARM64_REG_X6, UC_ARM64_REG_X7,
             UC_ARM64_REG_X8)
RET = bytes.fromhex("c0035fd6")


def align(number, width=PAGE):
    return (number + width - 1) & -width


def signed_leb128(blob):
    value = shift = 0
    for byte in blob:
        value |= (byte & 0x7f) << shift
        shift += 7
        if not byte & 0x80:
            yield value - (1 << shift) if byte & 0x40 else value
            value = shift = 0
    if shift:
        raise ValueError("Incomplete packed relocation")


def android_rela(blob):
    if blob[:4] != b"APS2":
        raise ValueError("Unexpected Android relocation format")
    values = iter(signed_leb128(blob[4:]))
    count, offset = next(values), next(values)
    addend = 0
    while count:
        size, flags = next(values), next(values)
        if not 0 < size <= count or flags & ~15:
            raise ValueError("Invalid Android relocation group")
        grouped_info, grouped_offset, grouped_addend, has_addend = (
            flags & 1, flags & 2, flags & 4, flags & 8)
        delta = next(values) if grouped_offset else 0
        info = next(values) if grouped_info else 0
        if has_addend and grouped_addend:
            addend += next(values)
        if not has_addend:
            addend = 0
        for _ in range(size):
            offset += delta if grouped_offset else next(values)
            if not grouped_info:
                info = next(values)
            if has_addend and not grouped_addend:
                addend += next(values)
            yield offset, info, addend
        count -= size
    if next(values, None) is not None:
        raise ValueError("Extra bytes in Android relocation table")


def relr_offsets(blob):
    cursor = None
    for (word,) in struct.iter_unpack("<Q", blob):
        if word & 1:
            if cursor is None:
                raise ValueError("RELR bitmap before base address")
            for index in range(63):
                if (word >> (index + 1)) & 1:
                    yield cursor + index * 8
            cursor += 63 * 8
        else:
            yield word
            cursor = word + 8


class SoftwareDap:
    def __init__(self, *, call_timeout_us=0, virtual_tlb=False, fast_imports=None, fast_heap=False):
        if fast_heap and not fast_imports:
            raise ValueError("The host-backed heap requires the native import helper")
        payload = LIBRARY.read_bytes()
        if hashlib.sha256(payload).hexdigest() != SHA256:
            raise ValueError("The captured Nabu Dolby library changed")
        self.cpu = Uc(UC_ARCH_ARM64, UC_MODE_ARM)
        if virtual_tlb:
            self.cpu.ctl_set_tlb_mode(UC_TLB_VIRTUAL)
        self.call_limit = {}
        if call_timeout_us == 0:
            self.call_limit = {"count": 30_000_000}
        elif call_timeout_us is not None:
            self.call_limit = {"timeout": call_timeout_us}
        self.fast_imports = None
        self.fast_addresses = {}
        self.python_import_end = IMPORTS + PAGE
        if fast_imports:
            from FastDolbyImports import FastDolbyImports, NAMES
            self.python_import_end = IMPORTS + PAGE // 2
            self.fast_addresses = {name: self.python_import_end + 4 * index
                                   for index, name in enumerate(NAMES)}
        self.imports = {}
        self.parameters = {}
        self.cursor = HEAP
        self.token_cursor = 0
        self.failure = None
        elf = ELFFile(io.BytesIO(payload))
        segments = list(elf.iter_segments(type="PT_LOAD"))
        for segment in segments:
            start = segment["p_vaddr"]
            low = start & -PAGE
            end = align(start + segment["p_memsz"])
            self.cpu.mem_map(IMAGE + low, end - low)
            self.cpu.mem_write(IMAGE + start, segment.data())

        self.cpu.mem_map(IMPORTS, PAGE)
        self.cpu.mem_map(RETURN, PAGE)
        self.cpu.mem_map(STACK, STACK_BYTES + PAGE, 3)
        host_heap = None
        if fast_heap:
            backing = ctypes.create_string_buffer(HEAP_BYTES + PAGE - 1)
            host_heap = align(ctypes.addressof(backing))
            self.cpu.mem_map_ptr(HEAP, HEAP_BYTES, 3, host_heap)
            self.cpu.nabu_heap_backing = backing
        else:
            self.cpu.mem_map(HEAP, HEAP_BYTES, 3)
        self.cpu.reg_write(UC_ARM64_REG_TPIDR_EL0, TLS)
        self.exports = {}
        symbols = elf.get_section_by_name(".dynsym")
        for symbol in symbols.iter_symbols():
            if symbol["st_shndx"] != "SHN_UNDEF":
                self.exports[symbol.name] = IMAGE + symbol["st_value"]

        for off in relr_offsets(elf.get_section_by_name(".relr.dyn").data()):
            self.write_int(IMAGE + off, IMAGE + self.read_int(IMAGE + off))
        packed = android_rela(elf.get_section_by_name(".rela.dyn").data())
        plt = struct.iter_unpack("<QQq", elf.get_section_by_name(".rela.plt").data())
        for table in (packed, plt):
            for off, info, addend in table:
                kind = info & 0xffffffff
                if kind == 1027:
                    value = IMAGE + addend
                elif kind in (257, 1025, 1026):
                    symbol = symbols.get_symbol(info >> 32)
                    value = self.exports.get(symbol.name)
                    if value is None:
                        value = self.import_address(symbol.name)
                    value += addend
                else:
                    raise ValueError(f"Unexpected relocation type {kind}")
                self.write_int(IMAGE + off, value)

        for segment in segments:
            bits = segment["p_flags"]
            perms = ((bits & 4) >> 2) | (bits & 2) | ((bits & 1) << 2)
            start = segment["p_vaddr"]
            low = start & -PAGE
            self.cpu.mem_protect(IMAGE + low, align(start + segment["p_memsz"]) - low, perms)
        self.cpu.mem_protect(IMPORTS, PAGE, 5)
        self.cpu.mem_protect(RETURN, PAGE, 5)
        self.cpu.hook_add(UC_HOOK_CODE, self.call_stub, begin=IMPORTS, end=self.python_import_end - 1)
        if fast_imports:
            self.fast_imports = FastDolbyImports(fast_imports, self.cpu, self.python_import_end)
            if host_heap is not None:
                self.fast_imports.set_heap(HEAP, HEAP_BYTES, host_heap)
            # Keep the native hook context alive for as long as the Unicorn engine.
            self.cpu.nabu_fast_imports = self.fast_imports

    def read_int(self, address, size=8):
        return int.from_bytes(self.cpu.mem_read(address, size), "little")

    def write_int(self, address, value, size=8):
        self.cpu.mem_write(address, (value & ((1 << (8 * size)) - 1)).to_bytes(size, "little"))

    def alloc(self, size):
        start = self.cursor
        self.cursor += align(max(size, 1), 32)
        if self.cursor > HEAP + HEAP_BYTES:
            raise MemoryError("Dolby probe heap is full")
        return start

    def c_string(self, address, limit=4096):
        buffer = bytearray()
        for offset in range(limit):
            value = self.read_int(address + offset, 1)
            if value == 0:
                return bytes(buffer)
            buffer.append(value)
        return bytes(buffer)

    def import_address(self, name):
        for address, existing in self.imports.items():
            if name == existing:
                return address
        address = self.fast_addresses.get(name)
        if address is None:
            address = IMPORTS + 4 * len(self.imports)
            if address >= self.python_import_end:
                raise ValueError("Import trampoline page is full")
        self.imports[address] = name
        self.cpu.mem_write(address, RET)
        return address

    def call_stub(self, cpu, address, size, _):
        name = self.imports[address]
        a, b, length = (cpu.reg_read(reg) for reg in ARGUMENTS[:3])
        try:
            if length > HEAP_BYTES and name in ("memcpy", "memmove", "memset", "memcmp"):
                raise ValueError(f"Excessive {name} size {length}")
            if name in ("memcpy", "memmove"):
                if length:
                    cpu.mem_write(a, bytes(cpu.mem_read(b, length)))
                result = a
            elif name == "memset":
                if length:
                    cpu.mem_write(a, bytes([b & 0xff]) * length)
                result = a
            elif name == "memcmp":
                left, right = cpu.mem_read(a, length), cpu.mem_read(b, length)
                result = (left > right) - (left < right)
            elif name in ("malloc", "_Znam", "_Znwm"):
                result = self.alloc(a)
            elif name == "strncpy":
                if length > HEAP_BYTES:
                    raise ValueError("Excessive strncpy length")
                value = self.c_string(b, length)
                if length:
                    cpu.mem_write(a, value.ljust(length, b"\0"))
                result = a
            elif name in ("strcmp", "strncmp"):
                limit = length if name == "strncmp" else 4096
                left, right = self.c_string(a, limit), self.c_string(b, limit)
                result = (left > right) - (left < right)
            elif name in ("strtoul", "atoi"):
                import re
                radix = length if name == "strtoul" else 10
                if radix not in (0, 10):
                    raise ValueError(f"Unexpected numeric base {radix}")
                content = self.c_string(a)
                match = re.match(rb"\s*([+-]?\d+)", content)
                result = int(match[1]) if match else 0
                if name == "strtoul" and b:
                    self.write_int(b, a + (match.end() if match else 0))
            elif name == "strtok":
                cursor = a or self.token_cursor
                delimiters = set(self.c_string(b))
                result = 0
                if cursor:
                    content = self.c_string(cursor)
                    start = next((i for i, ch in enumerate(content) if ch not in delimiters), len(content))
                    if start < len(content):
                        result = cursor + start
                        end = next((i for i in range(start, len(content)) if content[i] in delimiters),
                                   len(content))
                        if end < len(content):
                            self.write_int(cursor + end, 0, 1)
                        self.token_cursor = cursor + end + 1 if end < len(content) else 0
                    else:
                        self.token_cursor = 0
            elif name in ("free", "_ZdaPv", "_ZdlPv"):
                if a and not HEAP <= a < self.cursor:
                    raise ValueError("Attempt to free outside guest heap")
                # The short-lived probe releases the whole guest at process exit.
                result = 0
            elif name == "__strlen_chk":
                data = bytes(cpu.mem_read(a, b))
                result = data.index(0)
            elif name in ("pthread_mutexattr_init", "pthread_mutexattr_destroy",
                          "pthread_mutexattr_settype", "pthread_mutex_init",
                          "pthread_mutex_lock", "pthread_mutex_unlock",
                          "pthread_mutex_trylock", "pthread_mutex_destroy"):
                # The probe executes one guest call at a time.
                result = 0
            elif name == "_ZN5dolby13DapParamCache3setEiPKii":
                count = cpu.reg_read(UC_ARM64_REG_X3)
                if not 0 < count <= 8192:
                    raise ValueError("Unexpected cached parameter size")
                self.parameters[(a, b)] = bytes(cpu.mem_read(length, count * 4))
                result = 0
            elif name in ("_ZN7android16SortedVectorImplC2Emj",
                          "_ZN7android10VectorImplC2Emj", "_ZN7android10VectorImpl5clearEv"):
                # Each constructor starts with a fresh zeroed guest buffer.
                result = a
            elif name == "_ZN7android7String86formatEPKcz":
                self.write_int(cpu.reg_read(UC_ARM64_REG_X8), a)
                result = 0
            elif name == "_ZN7android7String8C1EPKc":
                # Dap2Process::setParam uses these String8 calls for its debug messages.
                self.write_int(a, b)
                result = a
            elif name == "_ZN7android7String8C1ERKS0_":
                self.write_int(a, self.read_int(b))
                result = a
            elif name in ("_ZN7android7String812appendFormatEPKcz",
                          "_ZN7android7String86appendEPKc",
                          "_ZN7android7String86appendERKS0_",
                          "_ZN7android7String8D1Ev"):
                result = 0
            elif name in ("__android_log_print", "__android_log_is_loggable_len"):
                result = 0
            elif name in ("powf", "logf", "log10f", "sinf", "sincosf", "lrintf", "frexpf"):
                vec0 = cpu.reg_read(UC_ARM64_REG_Q0)
                f = struct.unpack("<f", struct.pack("<I", vec0 & 0xffffffff))[0]
                if name == "sincosf":
                    cpu.mem_write(a, struct.pack("<f", math.sin(f)))
                    cpu.mem_write(b, struct.pack("<f", math.cos(f)))
                    return
                if name == "lrintf":
                    cpu.reg_write(UC_ARM64_REG_X0, round(f) & 0xffffffffffffffff)
                    return
                if name == "frexpf":
                    value, exponent = math.frexp(f)
                    cpu.mem_write(a, struct.pack("<i", exponent))
                elif name == "powf":
                    vec1 = cpu.reg_read(UC_ARM64_REG_Q1)
                    power = struct.unpack("<f", struct.pack("<I", vec1 & 0xffffffff))[0]
                    value = math.pow(f, power)
                else:
                    value = {"logf": math.log, "log10f": math.log10, "sinf": math.sin}[name](f)
                cpu.reg_write(UC_ARM64_REG_Q0, struct.unpack("<I", struct.pack("<f", value))[0])
                return
            else:
                raise ValueError(f"Unimplemented Android import {name}")
            cpu.reg_write(UC_ARM64_REG_X0, result & 0xffffffffffffffff)
        except (UcError, ValueError, OverflowError) as error:
            self.failure = error
            cpu.emu_stop()

    def call(self, name, *args):
        if len(args) > len(ARGUMENTS):
            raise ValueError("Too many register arguments")
        self.cpu.reg_write(UC_ARM64_REG_SP, STACK + STACK_BYTES)
        self.cpu.reg_write(UC_ARM64_REG_X30, RETURN)
        for reg, value in zip(ARGUMENTS, (*args, *([0] * (len(ARGUMENTS) - len(args))))):
            self.cpu.reg_write(reg, value & 0xffffffffffffffff)
        self.failure = None
        try:
            self.cpu.emu_start(self.exports[name], RETURN, **self.call_limit)
        except UcError as error:
            pc = self.cpu.reg_read(UC_ARM64_REG_PC)
            raise RuntimeError(f"{name} stopped at {pc:#x}") from error
        if self.failure is not None:
            raise RuntimeError(f"{name} failed: {self.failure}") from self.failure
        if self.fast_imports:
            self.fast_imports.check()
        if self.cpu.reg_read(UC_ARM64_REG_PC) != RETURN:
            raise RuntimeError(f"{name} exceeded its emulation limit")
        return self.cpu.reg_read(UC_ARM64_REG_X0)


if __name__ == "__main__":
    dap = SoftwareDap()
    print("Loaded verified HyperOS software Dolby core in the ARM64 emulator.")
    print(f"Version return value: {dap.call('dap_cpdp_get_version'):#x}")
