"""Restore the exact post-initialization guest state, without inventing DSP settings."""
import copy

class OriginalState:
    def __init__(self, player):
        dap = player.dap
        self.memory = [(begin, bytes(dap.cpu.mem_read(begin, end-begin+1)))
                       for begin, end, permissions in dap.cpu.mem_regions() if permissions & 2]
        self.cpu = dap.cpu.context_save()
        self.cursor = dap.cursor
        self.token_cursor = dap.token_cursor
        self.parameters = copy.deepcopy(dap.parameters)
        self.buffers = player.buffers.copy()
        self.bytes = sum(len(data) for _, data in self.memory)

    def restore(self, player):
        dap = player.dap
        for begin, data in self.memory: dap.cpu.mem_write(begin, data)
        dap.cpu.context_restore(self.cpu)
        dap.cursor, dap.token_cursor = self.cursor, self.token_cursor
        dap.parameters = copy.deepcopy(self.parameters)
        dap.failure = None
        player.buffers = self.buffers.copy()
        player.seconds, player.blocks = 0, 0
        if dap.fast_imports: dap.fast_imports.check()
