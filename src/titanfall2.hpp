#pragma once

#include <cstddef>
#include <cstdint>

#include "common.hpp"


namespace titanfall2 {
    const int VERSION = 37;
    const int sprp_VERSION = 13;
    /* LUMP INDICES */
    const int GAME_LUMP        = 0x23;
    const int LIGHTPROBE_REFS  = 0x68;
    const int REAL_TIME_LIGHTS = 0x69;

    struct LightProbeRef {
        Vector    origin;
        uint32_t  probe;
        int32_t   unknown;
    };
    static_assert(sizeof(LightProbeRef) == 0x14);
    static_assert(offsetof(LightProbeRef, origin)  == 0x00);
    static_assert(offsetof(LightProbeRef, probe)   == 0x0C);
    static_assert(offsetof(LightProbeRef, unknown) == 0x10);
};
