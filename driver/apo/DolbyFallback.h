#ifndef NABU_DOLBY_FALLBACK_H
#define NABU_DOLBY_FALLBACK_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef struct {
    float left;
    float right;
} StereoFrame;

typedef struct {
    const StereoFrame *frames;
    uint64_t first_frame;
    size_t count;
} DolbyOutput;

typedef struct {
    StereoFrame *dry_frames;
    size_t delay;
    size_t slot;
    uint64_t next_frame;
    uint64_t wet_count;
    uint64_t dry_count;
    uint64_t first_dry_frame;
} DolbyFallback;

/* The caller owns the delay buffer and allocates it outside the audio callback. */
static int dolby_fallback_init(DolbyFallback *state, StereoFrame *buffer, size_t delay)
{
    if (!state || !buffer || !delay || delay > SIZE_MAX / sizeof(*buffer))
        return 0;
    memset(buffer, 0, delay * sizeof(*buffer));
    DolbyFallback initial = {buffer, delay, 0, 0, 0, 0, UINT64_MAX};
    *state = initial;
    return 1;
}

/*
 * Worker frames use the input frame index. The caller must keep the worker's
 * immutable result readable for the duration of one callback. Late or unsafe
 * output falls back to the delayed original at the same position.
 */
static void dolby_fallback_process(DolbyFallback *state, const StereoFrame *input,
    StereoFrame *output, size_t count, const DolbyOutput *worker)
{
    for (size_t i = 0; i < count; ++i) {
        StereoFrame chosen = state->dry_frames[state->slot];
        state->dry_frames[state->slot] = input[i];
        state->slot++;
        if (state->slot == state->delay)
            state->slot = 0;

        uint64_t current = state->next_frame++;
        int used_wet = 0;
        if (worker && worker->frames && current >= state->delay) {
            uint64_t source = current - state->delay;
            if (source >= worker->first_frame && source - worker->first_frame < worker->count) {
                StereoFrame wet = worker->frames[source - worker->first_frame];
                if (wet.left >= -1.0f && wet.left <= 1.0f && wet.right >= -1.0f && wet.right <= 1.0f) {
                    chosen = wet;
                    used_wet = 1;
                }
            }
        }
        output[i] = chosen;
        if (used_wet) ++state->wet_count;
        else if (current >= state->delay) {
            ++state->dry_count;
            if (state->first_dry_frame == UINT64_MAX) state->first_dry_frame = current;
        }
    }
}

#endif
