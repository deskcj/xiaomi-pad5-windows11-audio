#ifndef NABU_DOLBY_STREAM_H
#define NABU_DOLBY_STREAM_H

#include "DolbyBlockQueue.h"

typedef struct {
    DolbyFallback fallback;
    DolbyBlockQueue *requests;
    DolbyBlockQueue *replies;
    DolbyAudioBlock pending;
    uint32_t generation;
    volatile LONG *requests_dropped;
} DolbyStream;

/* Transport and delay storage are allocated by the caller before rendering. */
static BOOL dolby_stream_init(DolbyStream *stream, StereoFrame *delay_frames, size_t delay,
    DolbyBlockQueue *requests, DolbyBlockQueue *replies, uint32_t generation)
{
    if (!requests || !replies || requests == replies ||
            !dolby_fallback_init(&stream->fallback, delay_frames, delay))
        return FALSE;
    stream->requests = requests;
    stream->replies = replies;
    stream->pending.count = 0;
    stream->generation = generation;
    return TRUE;
}

static void dolby_stream_process(DolbyStream *stream, const StereoFrame *input,
    StereoFrame *output, uint32_t count)
{
    /* Bound discarded stale replies, not valid replies consumed by a callback.
     * Otherwise several stale records can exhaust the budget and incorrectly
     * force valid later blocks to the dry path in the same callback. */
    unsigned remaining_stale_reads = 8;
    for (uint32_t offset = 0; offset < count;) {
        uint32_t block_frames = count - offset;
        if (block_frames > DOLBY_BLOCK_FRAMES)
            block_frames = DOLBY_BLOCK_FRAMES;
        if (!dolby_queue_write(stream->requests, stream->generation, stream->fallback.next_frame,
                input + offset, block_frames) && stream->requests_dropped)
            InterlockedIncrement(stream->requests_dropped);

        uint32_t block_end = offset + block_frames;
        while (offset < block_end) {
            uint64_t now = stream->fallback.next_frame;
            uint64_t delay = stream->fallback.delay;
            uint64_t run = block_end - offset;
            DolbyOutput wet = {0};
            DolbyAudioBlock *pending = &stream->pending;

            if (now < delay) {
                if (run > delay - now)
                    run = delay - now;
            } else {
                uint64_t target = now - delay;
                for (;;) {
                    if (pending->count && pending->generation == stream->generation &&
                            (target < pending->first_frame || target - pending->first_frame < pending->count))
                        break;
                    BOOL consumed = pending->count && pending->generation == stream->generation &&
                        target >= pending->first_frame && target - pending->first_frame == pending->count;
                    if (pending->count && !consumed) {
                        if (!remaining_stale_reads) {
                            pending->count = 0;
                            break;
                        }
                        --remaining_stale_reads;
                    }
                    pending->count = 0;
                    if (!remaining_stale_reads && !consumed)
                        break;
                    if (!dolby_queue_read(stream->replies, pending))
                        break;
                }

                if (pending->count && target < pending->first_frame) {
                    uint64_t gap = pending->first_frame - target;
                    if (run > gap)
                        run = gap;
                } else if (pending->count) {
                    uint64_t available = pending->count - (target - pending->first_frame);
                    if (run > available)
                        run = available;
                    wet.frames = pending->frames;
                    wet.first_frame = pending->first_frame;
                    wet.count = pending->count;
                }
            }

            dolby_fallback_process(&stream->fallback, input + offset, output + offset, (size_t)run, &wet);
            offset += (uint32_t)run;
        }
    }
}

#endif
