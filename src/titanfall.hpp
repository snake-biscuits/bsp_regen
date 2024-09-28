#pragma once

#include <cstddef>
#include <cstdint>

#include "common.hpp"


namespace titanfall {
    const int VERSION = 29;
    const int sprp_VERSION = 12;
    /* LUMP INDICES */
    const int MODELS                = 0x0E;
    const int GAME_LUMP             = 0x23;
    const int TRICOLL_TRIS          = 0x42;
    const int TRICOLL_HEADER        = 0x45;
    const int CM_GRID               = 0x55;
    const int CM_GRID_CELLS         = 0x56;
    const int CM_GEO_SETS           = 0x57;
    const int CM_GEO_SET_BOUNDS     = 0x58;
    const int CM_PRIMITIVES         = 0x59;
    const int CM_PRIMITIVE_BOUNDS   = 0x5A;
    const int CM_UNIQUE_CONTENTS    = 0x5B;
    const int TRICOLL_BEVEL_STARTS  = 0x60;
    const int TRICOLL_BEVEL_INDICES = 0x61;
    const int LIGHTPROBE_REFS       = 0x68;
    const int REAL_TIME_LIGHTS      = 0x69;
    /* PRIMITIVE TYPES */
    // const int BRUSH   = 0x00;
    // const int TRICOLL = 0x40;
    const int PROP    = 0x60;


    struct Bounds {  // bounds for both GeoSet & Primitives
        int16_t  origin[3];  // xyz
        int16_t  sin;
        int16_t  extents[3];  // xyz
        int16_t  cos;
    };

    static_assert(sizeof(Bounds) == 0x10);
    static_assert(offsetof(Bounds, origin)  == 0x00);
    static_assert(offsetof(Bounds, sin)     == 0x06);
    static_assert(offsetof(Bounds, extents) == 0x08);
    static_assert(offsetof(Bounds, cos)     == 0x0E);


    struct GeoSet {
        uint16_t  straddle_group;
        uint16_t  num_primitives;
        uint32_t  primitive;
    };
    // NOTE: primitive is a bitfield, but C++ isn't consistent enough to use them
    // -- order & number of bytes used varies depending on implementation & platform
    // -- we also can't confirm the member order with static_assert afaik
    // struct Primitive { uint32_t type: 8, index: 16, unique_contents: 8; };
    // uint32_t primitive = (type << 24) | (index << 8) | (unique_contents);
    // -- don't forget to check the bounds of each member before composing the uint32_t!

    static_assert(sizeof(GeoSet) == 0x08);
    static_assert(offsetof(GeoSet, straddle_group)  == 0x00);
    static_assert(offsetof(GeoSet, num_primitives)  == 0x02);
    static_assert(offsetof(GeoSet, primitive)       == 0x04);


    struct Grid {
        float    scale;
        int32_t  cell_offset[2];
        int32_t  num_cells[2];
        int32_t  num_straddle_groups;
        int32_t  first_brush_plane;
    };

    static_assert(sizeof(Grid) == 0x1C);
    static_assert(offsetof(Grid, scale)               == 0x00);
    static_assert(offsetof(Grid, cell_offset)         == 0x04);
    static_assert(offsetof(Grid, num_cells)           == 0x0C);
    static_assert(offsetof(Grid, num_straddle_groups) == 0x14);
    static_assert(offsetof(Grid, first_brush_plane)   == 0x18);


    struct GridCell {
        uint16_t first_geo_set;
        uint16_t num_geo_sets;
    };

    static_assert(sizeof(GridCell) == 0x04);
    static_assert(offsetof(GridCell, first_geo_set) == 0x00);
    static_assert(offsetof(GridCell, num_geo_sets)  == 0x02);


    struct LightProbeRef {
        Vector3   origin;
        uint32_t  probe;
    };

    static_assert(sizeof(LightProbeRef) == 0x10);
    static_assert(offsetof(LightProbeRef, origin) == 0x00);
    static_assert(offsetof(LightProbeRef, probe)  == 0x0C);


    struct StaticProp {
        Vector3   origin;
        Vector3   angles;
        uint16_t  model_name;
        uint16_t  first_leaf;
        uint16_t  num_leaves;
        uint8_t   solid_type;
        uint8_t   flags;
        uint16_t  skin;
        uint16_t  cubemap;
        float     fade_distance_min;
        float     fade_distance_max;
        Vector3   lighting_origin;
        float     forced_fade_scale;
        int8_t    cpu_level_min;
        int8_t    cpu_level_max;
        int8_t    gpu_level_min;
        int8_t    gpu_level_max;
        uint8_t   diffuse_modulation_r;
        uint8_t   diffuse_modulation_g;
        uint8_t   diffuse_modulation_b;
        uint8_t   diffuse_modulation_a;
        uint32_t  disable_x360;
        float     scale;
        uint32_t  collision_flags_add;
        uint32_t  collision_flags_remove;
    };

    static_assert(sizeof(StaticProp) == 0x54);
    static_assert(offsetof(StaticProp, origin)                  == 0x00);
    static_assert(offsetof(StaticProp, angles)                  == 0x0C);
    static_assert(offsetof(StaticProp, model_name)              == 0x18);
    static_assert(offsetof(StaticProp, first_leaf)              == 0x1A);
    static_assert(offsetof(StaticProp, num_leaves)              == 0x1C);
    static_assert(offsetof(StaticProp, solid_type)              == 0x1E);
    static_assert(offsetof(StaticProp, flags)                   == 0x1F);
    static_assert(offsetof(StaticProp, skin)                    == 0x20);
    static_assert(offsetof(StaticProp, cubemap)                 == 0x22);
    static_assert(offsetof(StaticProp, fade_distance_min)       == 0x24);
    static_assert(offsetof(StaticProp, fade_distance_max)       == 0x28);
    static_assert(offsetof(StaticProp, lighting_origin)         == 0x2C);
    static_assert(offsetof(StaticProp, forced_fade_scale)       == 0x38);
    static_assert(offsetof(StaticProp, cpu_level_min)           == 0x3C);
    static_assert(offsetof(StaticProp, cpu_level_max)           == 0x3D);
    static_assert(offsetof(StaticProp, gpu_level_min)           == 0x3E);
    static_assert(offsetof(StaticProp, gpu_level_max)           == 0x3F);
    static_assert(offsetof(StaticProp, diffuse_modulation_r)    == 0x40);
    static_assert(offsetof(StaticProp, diffuse_modulation_g)    == 0x41);
    static_assert(offsetof(StaticProp, diffuse_modulation_b)    == 0x42);
    static_assert(offsetof(StaticProp, diffuse_modulation_a)    == 0x43);
    static_assert(offsetof(StaticProp, disable_x360)            == 0x44);
    static_assert(offsetof(StaticProp, scale)                   == 0x48);
    static_assert(offsetof(StaticProp, collision_flags_add)     == 0x4C);
    static_assert(offsetof(StaticProp, collision_flags_remove)  == 0x50);


    struct TricollHeader {
        int16_t   flags;
        int16_t   texture_flags;
        int16_t   texture_data;
        int16_t   num_vertices;
        uint16_t  num_triangles;
        uint16_t  num_bevel_indices;
        int32_t   first_vertex;
        uint32_t  first_triangle;
        uint32_t  first_node;
        uint32_t  first_bevel_index;
        Vector3   origin;
        float     scale;
    };

    static_assert(sizeof(TricollHeader) == 0x2C);
    static_assert(offsetof(TricollHeader, flags)             == 0x00);
    static_assert(offsetof(TricollHeader, texture_flags)     == 0x02);
    static_assert(offsetof(TricollHeader, texture_data)      == 0x04);
    static_assert(offsetof(TricollHeader, num_vertices)      == 0x06);
    static_assert(offsetof(TricollHeader, num_triangles)     == 0x08);
    static_assert(offsetof(TricollHeader, num_bevel_indices) == 0x0A);
    static_assert(offsetof(TricollHeader, first_vertex)      == 0x0C);
    static_assert(offsetof(TricollHeader, first_triangle)    == 0x10);
    static_assert(offsetof(TricollHeader, first_node)        == 0x14);
    static_assert(offsetof(TricollHeader, first_bevel_index) == 0x18);
    static_assert(offsetof(TricollHeader, origin)            == 0x1C);
    static_assert(offsetof(TricollHeader, scale)             == 0x28);
};
