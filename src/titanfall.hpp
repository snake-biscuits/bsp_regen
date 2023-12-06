#pragma once

#include <cstddef>
#include <cstdint>

#include "common.hpp"


namespace titanfall {
    const int VERSION = 29;
    const int sprp_VERSION = 13;
    /* LUMP INDICES */
    const int GAME_LUMP        = 0x23;
    const int LIGHTPROBE_REFS  = 0x68;
    const int REAL_TIME_LIGHTS = 0x69;

    struct LightProbeRef {
        Vector    origin;
        uint32_t  probe;
    };
    static_assert(sizeof(LightProbeRef) == 0x10);
    static_assert(offsetof(LightProbeRef, origin) == 0x00);
    static_assert(offsetof(LightProbeRef, probe)  == 0x0C);
};
