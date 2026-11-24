#ifndef NABU_DOLBY_BLOCK_QUEUE_H
#define NABU_DOLBY_BLOCK_QUEUE_H

#include <windows.h>
#include <intrin.h>
#include "DolbyFallback.h"

/* Power of two for cursor wrap. A 128 ms delay spans 26 callbacks at 5 ms. */
enum { DOLBY_QUEUE_SLOTS = 32, DOLBY_BLOCK_FRAMES = 1024 };

typedef struct {
    uint64_t first_frame;
    uint32_t generation;
    uint32_t count;
    StereoFrame frames[DOLBY_BLOCK_FRAMES];
} DolbyAudioBlock;

typedef struct {
    volatile LONG write_index;
    volatile LONG read_index;
    DolbyAudioBlock slots[DOLBY_QUEUE_SLOTS];
} DolbyBlockQueue;

/* One producer and one consumer. Initialize to zero before either uses the queue. */
static BOOL dolby_queue_write(DolbyBlockQueue *queue, uint32_t generation,
    uint64_t first_frame, const StereoFrame *frames, uint32_t count)
{
    if (!frames || count == 0 || count > DOLBY_BLOCK_FRAMES)
        return FALSE;

    uint32_t write = (uint32_t)InterlockedCompareExchange(&queue->write_index, 0, 0);
    uint32_t read = (uint32_t)InterlockedCompareExchange(&queue->read_index, 0, 0);
    if (write - read >= DOLBY_QUEUE_SLOTS)
        return FALSE;

    DolbyAudioBlock *slot = &queue->slots[write % DOLBY_QUEUE_SLOTS];
    slot->first_frame = first_frame;
    slot->generation = generation;
    slot->count = count;
    memcpy(slot->frames, frames, count * sizeof(*frames));
    InterlockedExchange(&queue->write_index, (LONG)(write + 1));
    return TRUE;
}

static BOOL dolby_queue_read(DolbyBlockQueue *queue, DolbyAudioBlock *destination)
{
    uint32_t read = (uint32_t)InterlockedCompareExchange(&queue->read_index, 0, 0);
    uint32_t write = (uint32_t)InterlockedCompareExchange(&queue->write_index, 0, 0);
    if (write == read)
        return FALSE;

    const DolbyAudioBlock *slot = &queue->slots[read % DOLBY_QUEUE_SLOTS];
    uint32_t count = slot->count;
    BOOL valid = count > 0 && count <= DOLBY_BLOCK_FRAMES;
    if (valid) {
        destination->first_frame = slot->first_frame;
        destination->generation = slot->generation;
        destination->count = count;
        memcpy(destination->frames, slot->frames, count * sizeof(slot->frames[0]));
    }
    InterlockedExchange(&queue->read_index, (LONG)(read + 1));
    return valid;
}

#endif
