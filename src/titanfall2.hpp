#pragma once

#include <cstddef>
#include <cstdint>

#include "common.hpp"


namespace titanfall2 {
    const int VERSION = 37;
    const int sprp_VERSION = 13;
    /* LUMP INDICES */
    const int GAME_LUMP        = 0x23;
    const int TRICOLL_HEADER = 0x45;
    const int TRICOLL_BEVEL_STARTS = 0x60;
    const int TRICOLL_BEVEL_INDICES = 0x61;
    const int LIGHTPROBE_REFS  = 0x68;
    const int REAL_TIME_LIGHTS = 0x69;

    struct LightProbeRef {
        Vector3    origin;
        uint32_t  probe;
        int32_t   unknown;
    };
    static_assert(sizeof(LightProbeRef) == 0x14);
    static_assert(offsetof(LightProbeRef, origin)  == 0x00);
    static_assert(offsetof(LightProbeRef, probe)   == 0x0C);
    static_assert(offsetof(LightProbeRef, unknown) == 0x10);
    /*
    struct StaticProp {  // Titanfall | 2
        Vector3    origin;
        Vector3    angles;  // technically Y Z X, but we're just copying data
        float     scale;
        uint16_t  model_name;
        uint8_t   solid_mode;
        uint8_t   flags;
        uint32_t  unknown;
        float     forced_fade_scale;
        Vector3    lighting_origin;
        uint8_t   diffuse_modulation[4];  // rgba
        uint32_t  collision_flags_add;
        uint32_t  collision_flags_remove;
    };
    */

    struct StaticProp
    {
        Vector3 m_Origin;
        Vector3 m_Angles;
        float scale;
        uint16_t modelIndex;
        BYTE m_Solid;
        BYTE m_flags;
        WORD skin;
        WORD word_22;
        float forced_fade_scale;
        Vector3 m_LightingOrigin;
        uint8_t m_DiffuseModulation_r;
        uint8_t m_DiffuseModulation_g;
        uint8_t m_DiffuseModulation_b;
        uint8_t m_DiffuseModulation_a;
        int unk;
        DWORD collision_flags_remove;
    };


    static_assert(sizeof(StaticProp)==0x40);
};

