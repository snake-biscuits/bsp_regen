#pragma once

#include <cstddef>
#include <cstdint>

#include "common.hpp"


namespace titanfall {
    const int VERSION = 29;
    const int sprp_VERSION = 12;
    /* LUMP INDICES */
    const int GAME_LUMP = 0x23;
    const int TRICOLL_TRIS = 0x42;
    const int TRICOLL_HEADER = 0x45;
    const int TRICOLL_BEVEL_STARTS = 0x60;
    const int TRICOLL_BEVEL_INDICES = 0x61;
    const int LIGHTPROBE_REFS  = 0x68;
    const int REAL_TIME_LIGHTS = 0x69;

    struct LightProbeRef {
        Vector3    origin;
        uint32_t  probe;
    };
    static_assert(sizeof(LightProbeRef) == 0x10);
    static_assert(offsetof(LightProbeRef, origin) == 0x00);
    static_assert(offsetof(LightProbeRef, probe)  == 0x0C);
    /*
    struct StaticProp {  // Titanfall
        Vector3    origin;
        Vector3    angles;  // technically Y Z X, but we're just copying data
        uint16_t  model_name;
        uint16_t  first_leaf;
        uint16_t  num_leaves;
        uint8_t   solid_mode;
        uint8_t   flags;
        int32_t   skin;
        int32_t   cubemap;
        float     fade_distance_min;
        float     fade_distance_max;
        Vector3    lighting_origin;
        float     forced_fade_scale;
        int8_t    cpu_level_min;
        int8_t    cpu_level_max;
        int8_t    gpu_level_min;
        int8_t    gpu_level_max;
        uint8_t   diffuse_modulation[4];  // rgba
        float     scale;
        
        uint32_t  collision_flags_add;
        uint32_t  collision_flags_remove;
    };
    */

    struct StaticProp
    {
        Vector3 m_Origin;
        Vector3 m_Angles;
        uint16_t modelIndex;
        uint16_t firstLeaf;
        uint16_t num_leafs;
        BYTE solidType;
        BYTE flags;
        WORD skin;
        WORD word_22;
        float forced_fade_scale;
        BYTE gap_28[4];
        Vector3 lightingOrigin;
        BYTE gap_38[4];
        int8_t cpu_level_min;
        int8_t cpu_level_max;
        int8_t gpu_level_min;
        int8_t gpu_level_max;
        uint8_t m_DiffuseModulation_r;
        uint8_t m_DiffuseModulation_g;
        uint8_t m_DiffuseModulation_b;
        uint8_t m_DiffuseModulation_a;
        BYTE gap_44[4];
        float scale;
        BYTE gap_4C[4];
        uint32_t collision_flags_remove;
    };
    static_assert(sizeof(StaticProp)==0x54);
};
