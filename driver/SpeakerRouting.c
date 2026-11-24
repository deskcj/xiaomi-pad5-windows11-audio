#include "SpeakerRouting.h"

int NabuResolveSpeakerRoute(NabuRouteWord uid, NabuSpeakerRoute *route)
{
    static const NabuSpeakerRoute routes[] = {
        { 0, NabuBottomRight, NabuChannelRight, 0, 1, 0x400000 },
        { 1, NabuTopRight,    NabuChannelRight, 0, 1, 0x400000 },
        { 2, NabuBottomLeft,  NabuChannelLeft,  0, 1, 0 },
        { 3, NabuTopLeft,     NabuChannelLeft,  0, 1, 0 }
    };
    if (!route || uid >= sizeof(routes) / sizeof(routes[0]))
        return 1;
    *route = routes[uid];
    return 0;
}
