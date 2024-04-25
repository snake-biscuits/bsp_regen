#pragma once

#include <cstddef>
#include <cstdint>

#include "common.hpp"


namespace titanfall2 {
    const int VERSION = 37;
    const int sprp_VERSION = 13;
    /* LUMP INDICES */
    const int GAME_LUMP             = 0x23;
    const int TRICOLL_HEADER        = 0x45;
    const int TRICOLL_BEVEL_STARTS  = 0x60;
    const int TRICOLL_BEVEL_INDICES = 0x61;
    const int LIGHTPROBE_REFS       = 0x68;
    const int REAL_TIME_LIGHTS      = 0x69;

    struct LightProbeRef {
        Vector3   origin;
        uint32_t  probe;
        int32_t   unknown;
    };

    static_assert(sizeof(LightProbeRef) == 0x14);
    static_assert(offsetof(LightProbeRef, origin)  == 0x00);
    static_assert(offsetof(LightProbeRef, probe)   == 0x0C);
    static_assert(offsetof(LightProbeRef, unknown) == 0x10);


    struct StaticProp {
        Vector3   origin;
        Vector3   angles;
        float     scale;
        uint16_t  model_name;
        uint8_t   solid_type;
        uint8_t   flags;
        uint16_t  skin;
        uint16_t  cubemap;
        float     forced_fade_scale;
        Vector3   lighting_origin;
        uint8_t   diffuse_modulation_r;
        uint8_t   diffuse_modulation_g;
        uint8_t   diffuse_modulation_b;
        uint8_t   diffuse_modulation_a;
        uint32_t  collision_flags_add;
        uint32_t  collision_flags_remove;
    };

    static_assert(sizeof(StaticProp) == 0x40);
    static_assert(offsetof(StaticProp, origin)                  == 0x00);
    static_assert(offsetof(StaticProp, angles)                  == 0x0C);
    static_assert(offsetof(StaticProp, scale)                   == 0x18);
    static_assert(offsetof(StaticProp, model_name)              == 0x1C);
    static_assert(offsetof(StaticProp, solid_type)              == 0x1E);
    static_assert(offsetof(StaticProp, flags)                   == 0x1F);
    static_assert(offsetof(StaticProp, skin)                    == 0x20);
    static_assert(offsetof(StaticProp, cubemap)                 == 0x22);
    static_assert(offsetof(StaticProp, forced_fade_scale)       == 0x24);
    static_assert(offsetof(StaticProp, lighting_origin)         == 0x28);
    static_assert(offsetof(StaticProp, diffuse_modulation_r)    == 0x34);
    static_assert(offsetof(StaticProp, diffuse_modulation_g)    == 0x35);
    static_assert(offsetof(StaticProp, diffuse_modulation_b)    == 0x36);
    static_assert(offsetof(StaticProp, diffuse_modulation_a)    == 0x37);
    static_assert(offsetof(StaticProp, collision_flags_add)     == 0x38);
    static_assert(offsetof(StaticProp, collision_flags_remove)  == 0x3C);
};
