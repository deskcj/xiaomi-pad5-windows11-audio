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

enum { DOLBY_DRY_HISTORY = 16, DOLBY_EFFECT_RAMP_FRAMES = 480 };
typedef struct { uint64_t first_frame, last_frame; } DolbyDryRange;

typedef struct {
    StereoFrame *dry_frames;
    size_t delay;
    size_t slot;
    uint64_t next_frame;
    uint64_t wet_count;
    uint64_t dry_count;
    uint64_t first_dry_frame;
    uint64_t last_dry_frame;
    uint64_t dry_runs;
    DolbyDryRange dry_ranges[DOLBY_DRY_HISTORY];
    uint64_t bypass_count;
    uint32_t wet_mix;
} DolbyFallback;

/* The caller owns the delay buffer and allocates it outside the audio callback. */
static int dolby_fallback_init(DolbyFallback *state, StereoFrame *buffer, size_t delay)
{
    if (!state || !buffer || !delay || delay > SIZE_MAX / sizeof(*buffer))
        return 0;
    memset(buffer, 0, delay * sizeof(*buffer));
    DolbyFallback initial = {buffer, delay, 0, 0, 0, 0, UINT64_MAX, UINT64_MAX, 0, {{0, 0}}, 0, DOLBY_EFFECT_RAMP_FRAMES};
    *state = initial;
    return 1;
}

/*
 * Worker frames use the input frame index. The caller must keep the worker's
 * immutable result readable for the duration of one callback. Late or unsafe
 * output falls back to the delayed original at the same position.
 */
static void dolby_fallback_process_effect(DolbyFallback *state, const StereoFrame *input,
    StereoFrame *output, size_t count, const DolbyOutput *worker, int enabled)
{
    for (size_t i = 0; i < count; ++i) {
        StereoFrame chosen = state->dry_frames[state->slot];
        state->dry_frames[state->slot] = input[i];
        state->slot++;
        if (state->slot == state->delay)
            state->slot = 0;

        uint64_t current = state->next_frame++;
        if (enabled && state->wet_mix < DOLBY_EFFECT_RAMP_FRAMES) ++state->wet_mix;
        else if (!enabled && state->wet_mix) --state->wet_mix;
        int used_wet = 0;
        if (state->wet_mix && worker && worker->frames && current >= state->delay) {
            uint64_t source = current - state->delay;
            if (source >= worker->first_frame && source - worker->first_frame < worker->count) {
                StereoFrame wet = worker->frames[source - worker->first_frame];
                if (wet.left >= -1.0f && wet.left <= 1.0f && wet.right >= -1.0f && wet.right <= 1.0f) {
                    if (state->wet_mix == DOLBY_EFFECT_RAMP_FRAMES) chosen = wet;
                    else {
                        const double mix = (double)state->wet_mix / DOLBY_EFFECT_RAMP_FRAMES;
                        chosen.left = (float)(chosen.left + mix * ((double)wet.left - chosen.left));
                        chosen.right = (float)(chosen.right + mix * ((double)wet.right - chosen.right));
                    }
                    used_wet = 1;
                }
            }
        }
        output[i] = chosen;
        if (used_wet) ++state->wet_count;
        else if (current >= state->delay && !enabled) ++state->bypass_count;
        else if (current >= state->delay) {
            ++state->dry_count;
            if (state->first_dry_frame == UINT64_MAX) state->first_dry_frame = current;
            if (state->last_dry_frame == UINT64_MAX || current != state->last_dry_frame + 1) {
                DolbyDryRange *range = &state->dry_ranges[state->dry_runs % DOLBY_DRY_HISTORY];
                range->first_frame = range->last_frame = current;
                ++state->dry_runs;
            } else {
                state->dry_ranges[(state->dry_runs - 1) % DOLBY_DRY_HISTORY].last_frame = current;
            }
            state->last_dry_frame = current;
        }
    }
}

static void dolby_fallback_process(DolbyFallback *state, const StereoFrame *input,
    StereoFrame *output, size_t count, const DolbyOutput *worker)
{
    dolby_fallback_process_effect(state, input, output, count, worker, 1);
}

#endif
