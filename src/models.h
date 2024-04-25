#pragma once

#include <cstdint>

#include "common.hpp"
#include "memory_mapped_file.hpp"


struct mstudiopertrihdr_t {
    short     version;  // game requires this to be 2 or else it errors
    int16_t   unk;  // may or may not exist, version gets cast as short in ida
    Vector3   bbmin;
    Vector3   bbmax;
    uint32_t  unused[8];
};


struct studiohdr_t {
    int32_t   id;  // Model format ID (e.g. "IDST" [0x49, 0x44, 0x53, 0x54])
    int32_t   version;  // Format version number
    int32_t   checksum;  // This has to be the same in the .phy and .vtx files to load!
    char      name[64];  // The internal name of the model, padded with null bytes.
    int       length;  // Data size of MDL file in bytes.
    Vector3   eye_position;  // ideal eye position
    Vector3   illum_position;     // illumination center
    Vector3   hull_min, hull_max;  // ideal movement hull size
    Vector3   view_bbmin, view_bbmax;  // clipping bounding box
    int32_t   flags;
    int32_t   num_bones, bone_index;  // highest observed: 250
    int32_t   num_bone_controllers, bone_controller_index;
    int32_t   num_hitbox_sets, hitbox_set_index;
    int32_t   num_local_anim, local_anim_index;  // animation / pose descriptions
    int32_t   num_local_seq, local_seq_index;
    int32_t   activity_list_version;  // initialization flag - have the sequences been indexed?
    int32_t   events_indexed;
    int32_t   num_textures, texture_index;  // raw textures
    int32_t   numcdtextures, cdtextureindex;  // raw textures search paths
    int32_t   num_skin_ref, num_skin_families, skin_index;  // replaceable textures tables
    int32_t   num_body_parts, body_part_index;
    int32_t   num_local_attachments, local_attachment_index;
    int32_t   num_local_nodes, local_node_index, local_node_name_index;
    int32_t   deprecated_num_flex_desc, deprecated_flex_desc_index;
    int32_t   deprecated_num_flex_controllers, deprecated_flex_controller_index;
    int32_t   deprecated_num_flex_rules, deprecated_flex_rule_index;
    int32_t   num_ik_chains, ik_chain_index;
    int32_t   deprecated_num_mouths, deprecated_mouth_index;
    int32_t   num_local_pose_parameters, local_pose_param_index;
    int32_t   surfaceprop_index;
    int32_t   keyvalue_index, keyvalue_size;
    int32_t   num_local_ik_autoplay_locks, local_ik_autoplay_lock_index;
    float     mass;
    uint32_t  contents;
    int32_t   num_include_models, include_model_index;  // external animations, models, etc.
    int32_t   virtualModel;  // mutable void* (implementation specific back pointer to virtual data)
    int32_t   anim_block_name_index;  // for demand loaded animation blocks
    int32_t   num_anim_blocks, anim_block_index;
    int32_t   anim_block_model;  // mutable void*
    int32_t   bone_table_by_name_index;
    // used by tools only that don't cache, but persist mdl's peer data
    // engine uses virtualModel to back link to cache pointers
    int32_t   vertex_base;  // void*
    int32_t   index_base;  // void*
    // if STUDIOHDR_FLAGS_CONSTANT_DIRECTIONAL_LIGHT_DOT is set:
    // this value is used to calculate directional components of lighting on static props
    uint8_t   const_directional_light_dot;
    // set during load of mdl data to track *desired* lod configuration (not actual)
    // the *actual* clamped root lod is found in studiohwdata
    // this is stored here as a global store to ensure the staged loading matches the rendering
    uint8_t   root_lod;
    // set in the mdl data to specify that lod configuration should only allow first numAllowRootLODs
    // to be set as root LOD:
    //  numAllowedRootLODs = 0  means no restriction, any lod can be set as root lod.
    //  numAllowedRootLODs = N  means that lod0 - lod(N-1) can be set as root lod, but not lodN or lower.
    uint8_t   num_allowed_root_lods;
    uint8_t   unused;
    float     fade_distance;
    int32_t   deprecated_num_flex_controller_ui, deprecated_flex_controller_ui_index;
    float     vert_anim_fixed_point_scale;
    int32_t   surfaceprop_lookup;  // this index must be cached by the loader, not saved in the file
    // NOTE: No room to add stuff? Up the .mdl file format version
    // [and move all fields in studiohdr2_t into studiohdr_t and kill studiohdr2_t],
    // or add your stuff to studiohdr2_t. See NumSrcBoneTransforms/SrcBoneTransform for the pattern to use.
    int32_t   studiohdr2_index;
    int32_t   source_filename_offset;  // in v52 not every model has these strings, only four bytes when not present.
};


struct studiohdr2_t {
    int32_t  num_src_bone_transform, src_bone_transform_index;
    int32_t  illum_position_attachment_index;
    float    max_eye_deflection;  // defaults to cos(30) if not set
    int32_t  linearboneindex;
    int32_t  name_index;
    int32_t  num_bone_flex_drivers, bone_flex_driver_index;
    // for static props (and maybe others)
    // Precomputed Per-Triangle AABB data
    int32_t  per_tri_AABB_index;
    int32_t  per_tri_AABB_node_count;
    int32_t  per_tri_AABB_leaf_count;
    int32_t  per_tri_AABB_vert_count;
    // always "" or "Titan"
    int32_t  unknown_string_index;
    int32_t  reserved[39];
};


class Model {
    memory_mapped_file  file_;
    studiohdr_t        *header_;
    studiohdr2_t       *header2_;
public:
    Model(const char *filename) {
        if (!file_.open_existing(filename)) {
            char buffer[1024];
            snprintf(buffer, 1024, "Failed to open file %s", filename);
            throw std::runtime_error(buffer);
        }
        header_ = file_.rawdata<studiohdr_t>();
        header2_ = file_.rawdata<studiohdr2_t>(header_->studiohdr2_index);
    }

    ~Model() { file_.close(); }

    mstudiopertrihdr_t *getPerTriHeader() {
        if (header2_->per_tri_AABB_index == 0) { return 0; }
        return file_.rawdata<mstudiopertrihdr_t>(header_->studiohdr2_index + header2_->per_tri_AABB_index);
    }

    uint32_t getContents() { return header_->contents; }
};
