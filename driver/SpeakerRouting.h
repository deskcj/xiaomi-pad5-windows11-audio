#pragma once
#if defined(_KERNEL_MODE)
/* Avoid importing the user-mode CRT headers into a KMDF build. */
typedef unsigned int NabuRouteWord;
#else
#include <stdint.h>
typedef uint32_t NabuRouteWord;
#endif
typedef char NabuRouteWordMustBe32Bits[(sizeof(NabuRouteWord) == 4) ? 1 : -1];

/* Nabu board positions, as identified by Android DTS and the captured ACPI
 * I2C addresses. Channel selection matches HyperOS Channel Swap controls.
 * This describes register routing; it does not prove acoustic L/R separation.
 */
enum NabuSpeakerPosition { NabuBottomRight, NabuTopRight, NabuBottomLeft, NabuTopLeft };
enum NabuSpeakerChannel { NabuChannelLeft = 1, NabuChannelRight = 2 };
typedef struct NabuSpeakerRoute {
    NabuRouteWord Uid;
    NabuRouteWord Position;
    NabuRouteWord Channel;
    NabuRouteWord Rx1Slot;
    NabuRouteWord Rx2Slot;
    NabuRouteWord ChannelBalance;
} NabuSpeakerRoute;

#ifdef __cplusplus
extern "C" {
#endif
#ifdef NABU_ROUTING_TEST_DLL
#define NABU_ROUTING_API __declspec(dllexport)
#else
#define NABU_ROUTING_API
#endif
/* Invalid input leaves the output unchanged; there is no guessed UID fallback. */
NABU_ROUTING_API int NabuResolveSpeakerRoute(NabuRouteWord uid, NabuSpeakerRoute *route);
#ifdef __cplusplus
}
#endif
