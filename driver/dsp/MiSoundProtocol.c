#include "MiSoundProtocol.h"

int NabuMiSoundEncodeI32(uint32_t parameterType, uint32_t androidDeviceMask,
    const NabuMiSoundRoute *route, uint32_t value, NabuMiSoundI32Command *output)
{
    NabuMiSoundI32Command command;
    uint32_t parameter, topology, module;
    uint8_t b0, b1, b2, b3;
    static const char alphabet[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    if (!route || !output)
        return NabuMiSoundInvalidArgument;
    topology = route->Topology;
    module = route->Module;
    switch (parameterType) {
    case 1: parameter = 0x1000a103; break;
    case 2: parameter = 0x1000d003; break;
    case 3: parameter = 0x1000a104; break;
    case 4: parameter = 0x1000d005; break;
    case 5: parameter = 0x1000a105; break;
    case 7: parameter = 0x1000d003 | 0x158; break;
    case 8: parameter = 0x1000d003 | 500; break;
    case 9: parameter = 0x1000a107; break;
    case 10: parameter = 0x1000d068; break;
    case 15: parameter = 0x1000a108; break;
    case 16: parameter = 0x1000a121; topology = module = 0x1000a101; break;
    case 17: parameter = 0x1000a120; topology = module = 0x1000a101; break;
    case 18: parameter = 0x1000a109; break;
    case 19: parameter = 0x1000a111; break;
    case 20: parameter = 0x1000a112; break;
    case 21: parameter = 0x10013d14; break;
    case 22:
        if (androidDeviceMask & 2) {
            topology = 0x1000a115; module = 0x1000a110; parameter = 0x1000a131;
        } else {
            topology = 0x1000a101; module = 0x1000ad00; parameter = 0x1000ad03;
        }
        break;
    case 23: parameter = 0x1000a125; topology = module = 0x1000a101; break;
    default: return NabuMiSoundUnsupported;
    }

    command.Route = *route;
    command.Route.Topology = topology;
    command.Route.Module = module;
    command.Route.Parameter = parameter;
    command.Value = value;

    /* Android's original encoder uses little-endian I32 and omits '=' padding. */
    b0 = (uint8_t)value; b1 = (uint8_t)(value >> 8);
    b2 = (uint8_t)(value >> 16); b3 = (uint8_t)(value >> 24);
    command.PayloadBase64[0] = alphabet[b0 >> 2];
    command.PayloadBase64[1] = alphabet[((b0 & 3) << 4) | (b1 >> 4)];
    command.PayloadBase64[2] = alphabet[((b1 & 15) << 2) | (b2 >> 6)];
    command.PayloadBase64[3] = alphabet[b2 & 63];
    command.PayloadBase64[4] = alphabet[b3 >> 2];
    command.PayloadBase64[5] = alphabet[(b3 & 3) << 4];
    command.PayloadBase64[6] = 0;
    command.PayloadBase64[7] = 0;
    *output = command;
    return NabuMiSoundEncoded;
}
