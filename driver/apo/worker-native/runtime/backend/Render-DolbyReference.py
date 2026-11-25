"""Original stereo DAP renderer with a packaged, prevalidated factory profile."""
import hashlib,json,math,struct,time
from pathlib import Path
from ProbeDolbyCore import SHA256,SoftwareDap
RATE=48000
BLOCK=256
MAX_BATCH_BLOCKS=8
FLOATS=struct.Struct('<512f')
PCM=struct.Struct('<512h')
class StereoRenderer:
    def __init__(self, parameters, virtual_bass, *, call_timeout_us=0, virtual_tlb=False,
                 fast_imports=None, fast_heap=False):
        self.dap = SoftwareDap(call_timeout_us=call_timeout_us, virtual_tlb=virtual_tlb,
                               fast_imports=fast_imports, fast_heap=fast_heap)
        tag = self.dap.alloc(16)
        self.dap.cpu.mem_write(tag, b"Nabu-Software\0")
        self.engine = engine = self.dap.alloc(0x300)  # Constructor initializes through 0x2f4.
        self.dap.call("_ZN5dolby11Dap2ProcessC2EbbbPKc", engine, virtual_bass, 0, 1, tag)
        if self.dap.call("_ZN5dolby11Dap2Process4openEv", engine) & 0xffffffff:
            raise RuntimeError("The original Dolby stereo initializer failed")
        self.context = self.dap.read_int(engine + 0x30)
        self.scratch = self.dap.read_int(engine + 0x48)
        if not self.context or not self.scratch:
            raise RuntimeError("Dolby did not allocate its processing buffers")
        # Dap2Process::updateOutputMode selects mode 1 for plain stereo output.
        self.dap.call("dap_cpdp_output_mode_set", self.context, 1, 2, 0)
        self.metadata = self.dap.alloc(0x50)
        self.buffers = {1: (self.make_buffer(BLOCK), self.make_buffer(BLOCK), FLOATS)}
        for tag, words in parameters.items():
            self.set_parameter(tag, words)
        self.seconds = 0.0
        self.blocks = 0

    def set_parameter(self, tag, words):
        block = struct.pack(f"<{len(words)}i", *words)
        address = self.dap.alloc(len(block))
        self.dap.cpu.mem_write(address, block)
        key = int.from_bytes(tag.encode("ascii"), "little")
        result = self.dap.call("_ZN5dolby11Dap2Process8setParamEiPKii", self.engine, key,
                               address, len(words)) & 0xffffffff
        if result or self.dap.parameters.get((self.engine + 8, key)) != block:
            raise RuntimeError(f"Original Dolby parameter {tag!r} was not applied ({result:#x})")

    def make_buffer(self, frames):
        provider = self.dap.alloc(0x40)
        self.dap.call("_ZN5dolby17DlbBufferProviderC2Ev", provider)
        self.dap.call("_ZN5dolby17DlbBufferProvider9configureEi14audio_format_tj",
                      provider, frames, 5, 3)  # Android PCM_FLOAT, stereo mask.
        pointer = self.dap.read_int(provider + 0x38)
        if (not pointer or self.dap.read_int(provider + 8, 4) != 2
                or self.dap.read_int(provider + 0x28, 4) != frames):
            raise RuntimeError("Original buffer provider rejected stereo")
        return provider + 8, pointer

    def prepare_buffers(self, blocks):
        if not 1 <= blocks <= MAX_BATCH_BLOCKS:
            raise ValueError(f"Expected 1 to {MAX_BATCH_BLOCKS} whole stereo blocks")
        if blocks not in self.buffers:
            frames = blocks * BLOCK
            self.buffers[blocks] = (self.make_buffer(frames), self.make_buffer(frames),
                                    struct.Struct(f"<{frames * 2}f"))
        return self.buffers[blocks]

    def render(self, samples):
        blocks, tail = divmod(len(samples), 2 * BLOCK)
        if tail:
            raise ValueError("Expected whole stereo blocks")
        incoming, outgoing, frame_format = self.prepare_buffers(blocks)
        self.dap.cpu.mem_write(incoming[1], frame_format.pack(*samples))
        start = time.perf_counter()
        self.dap.cpu.mem_write(outgoing[1], struct.pack("<I", 0x7fc00000) * (2 * BLOCK * blocks))
        channels = self.dap.call("dap_cpdp_prepare", self.context, blocks, incoming[0], 0x22, 0, 0, 0)
        if channels != 2:
            raise RuntimeError(f"Dolby returned {channels} output channels for stereo")
        self.dap.call("dap_cpdp_process", self.context, outgoing[0], self.scratch,
                      0, 0, 0, 0, 0, self.metadata)  # AAPCS64 indirect return in X8.
        self.seconds += time.perf_counter() - start
        self.blocks += blocks
        result = frame_format.unpack(self.dap.cpu.mem_read(outgoing[1], frame_format.size))
        if any(not math.isfinite(sample) for sample in result):
            raise RuntimeError("Dolby returned a non-finite sample")
        return result

def original_configuration():
    root=Path(__file__).resolve().parent
    manifest=json.loads((root/'ResourceHashes.json').read_text())
    path=root/'resources/CapturedProfile.json'
    if hashlib.sha256(path.read_bytes()).hexdigest()!=manifest['resources/CapturedProfile.json']:
        raise ValueError('Captured profile hash mismatch')
    profile=json.loads(path.read_text())
    if profile['FormatVersion']!=1 or profile['OriginalLibrarySha256']!=SHA256:
        raise ValueError('Captured profile identity mismatch')
    return {key:tuple(value) for key,value in profile['Parameters'].items()},profile['VirtualBass'],profile['Pregain']
