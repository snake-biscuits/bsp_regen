#include <algorithm>
#include <cstdio>
#include <fstream>
#include <immintrin.h>
#include <map>
#include <set>

#include "bounds.hpp"
#include "bsp.hpp"
#include "memory_mapped_file.hpp"
#include "models.hpp"
#include "source.hpp"  // GameLumpHeader
#include "titanfall.hpp"
#include "titanfall2.hpp"
#include "tricoll.hpp"


#define PI 3.1415926536f
typedef const char ModelDictEntry[128];


void print_usage(char* argv0) {
    printf("USAGE: %s titanfall.bsp titanfall2.bsp\n", argv0);
    // printf("USAGE: %s -d titanfall_dir/ titanfall2_dir/\n", argv0);
}


int main(int argc, char* argv[]) {
    if (argc != 3) {
        print_usage(argv[0]);
        return 0;
    }
    char* in_filename  = argv[1];
    char* out_filename = argv[2];

    int ret = 0;
    try {
        int convert(char* in_filename, char* out_filename);
        ret = convert(in_filename, out_filename);
    } catch (std::exception &e) {
        fprintf(stderr, "Exception: %s\n", e.what());
        return 1;
    }
    return ret;
}


void addPropsToCmGrid(
    Bsp                              &r1bsp,
    std::vector<titanfall::GeoSet>   &r2GeoSets,
    std::vector<titanfall::Bounds>   &r2GeoSetBounds,
    std::vector<titanfall::GridCell> &r2GridCells,
    std::vector<uint32_t>            &r2Contents,
    std::vector<uint32_t>            &r2Primitives,
    std::vector<titanfall::Bounds>   &r2PrimitiveBounds) {

    auto r1GameLump        = r1bsp.get_lump<char>               (titanfall::GAME_LUMP);
    auto r1Grid            = r1bsp.get_lump<titanfall::Grid>    (titanfall::CM_GRID)[0];
    auto r1GridCells       = r1bsp.get_lump<titanfall::GridCell>(titanfall::CM_GRID_CELLS);
    auto r1GeoSets         = r1bsp.get_lump<titanfall::GeoSet>  (titanfall::CM_GEO_SETS);
    auto r1GeoSetBounds    = r1bsp.get_lump<titanfall::Bounds>  (titanfall::CM_GEO_SET_BOUNDS);
    auto r1Contents        = r1bsp.get_lump<uint32_t>           (titanfall::CM_UNIQUE_CONTENTS);
    auto r1Primitives      = r1bsp.get_lump<uint32_t>           (titanfall::CM_PRIMITIVES);
    auto r1PrimitiveBounds = r1bsp.get_lump<titanfall::Bounds>  (titanfall::CM_PRIMITIVE_BOUNDS);

    // copy base data (we will add to these vectors later)
    for (size_t i = 0; i < r1Primitives.size(); i++) {
        r2Primitives.push_back(r1Primitives[i]);
        r2PrimitiveBounds.push_back(r1PrimitiveBounds[i]);
    }

    for (size_t i = 0; i < r1Contents.size(); i++) {
        r2Contents.push_back(r1Contents[i]);
    }

    // parse gamelump
    uint32_t subLumpCount = *(uint32_t*)&r1GameLump[0];
    uint32_t readPtr = 4;
    for (uint32_t lumpIndex = 0; lumpIndex < subLumpCount; lumpIndex++) {
        source::GameLumpHeader header = *(source::GameLumpHeader*)&r1GameLump[readPtr];
        readPtr += sizeof(source::GameLumpHeader);
        if (header.id != MAGIC_sprp) {
            readPtr += header.length;
            continue;
        }
        uint32_t num_models = *(uint32_t*)&r1GameLump[readPtr];
        readPtr += 4;
        ModelDictEntry *modelDict = (ModelDictEntry*)&r1GameLump[readPtr];
        readPtr += 128 * num_models;
        uint32_t num_leaves = *(uint32_t*)&r1GameLump[readPtr];
        readPtr += 4 + 2 * num_leaves;
        uint32_t num_props = *(uint32_t*)&r1GameLump[readPtr];
        readPtr += 12;  // num_props, unknown_1, unknown_2
        titanfall::StaticProp *props = (titanfall::StaticProp*)&r1GameLump[readPtr];

        // base model bounds & contents flags
        std::vector<mstudiopertrihdr_t>  modelBoundingBoxes;
        std::vector<uint32_t>            modelContents;
        for (uint32_t i = 0; i < num_models; i++) {
            char buffer[1024];
            snprintf(buffer, 1024, "r1/%s", modelDict[i]);
            Model model {buffer};
            modelBoundingBoxes.push_back(*model.getPerTriHeader());
            modelContents.push_back(model.getContents());
        }


        struct PropData {
            uint32_t index;  // index in GAME_LUMP.sprp.props
            MinMax   bounds;
            uint32_t collision_flags;
            int      unique_contents;  // index into UniqueContents
        };
        // can be turned into Primitive + Bounds or GeoSet + Bounds
        // NOTE: we can't use bitfields for primitives, since order varies depending on compiler
        // titanfall::Primitive p {.type=96, .index=index, .unique_contents=unique_contents};
        // titanfall::GeoSet gs {.straddle_group=..., .num_primitives=1, .primitive={^^^}};
        // for GeoSets w/ multiple props: {.num_primitives=..., .primitive={.type=0, .index=first_primitive}};

        // sort props into straddle groups while collecting metadata
        std::map<std::set<int>, std::vector<PropData>> straddleGroupProps;
        for (uint32_t i = 0; i < num_props; i++) {
            if (props[i].solid_type == 0) {
                continue;  // prop isn't collidable, skip it
            }

            // bounding box
            __m128 origin = _mm_set_ps(0, props[i].origin.z, props[i].origin.y, props[i].origin.x);
            __m128 scale = _mm_set1_ps(props[i].scale);
            mstudiopertrihdr_t &perTri = modelBoundingBoxes[props[i].model_name];
            MinMax bounds = minmax_from_instance_bounds(perTri.bbmin, perTri.bbmax, origin, props[i].angles, scale);

            // collision flags
            uint32_t collisionFlags = modelContents[props[i].model_name];
            if ((collisionFlags & 1) != 0 || !collisionFlags) {
                collisionFlags = (collisionFlags & 0xFFFFFFFE) | 0xEB0280;
            }
            if ((collisionFlags & 2) != 0) {
                collisionFlags = (collisionFlags & 0xFFF7FFFD) | 0xE30240;
            }
            if ((collisionFlags & 8) != 0) {
                collisionFlags = (collisionFlags & 0xFFB7FFF7) | 0xA30240;
            }
            collisionFlags &= ~props[i].collision_flags_remove;

            // uniqueContentsIndex
            int uniqueContentsIndex = 0;
            for (uint32_t uniqueContents : r2Contents) {
                if (uniqueContents != collisionFlags) {
                    break;
                }
                uniqueContentsIndex++;
            }
            if (uniqueContentsIndex == r2Contents.size()) {
                if (uniqueContentsIndex > 0xFF) {
                    // NOTE: this should never happen, but we should still assert assumptions
                    fprintf(stderr, "UniqueContents too big\n");
                    exit(1);
                }
                r2Contents.push_back(collisionFlags);
            }

            // GridCells containing this prop
            std::set<int> gridCellsTouched;
            float gridCellMins[2], gridCellMaxs[2];  // x & ys
            for (int y = 0; y < r1Grid.num_cells[1]; y++) {
                gridCellMins[1] = (y + r1Grid.cell_offset[1]) * r1Grid.scale;
                gridCellMaxs[1] = gridCellMins[1] + r1Grid.scale;
                for (int x = 0; x < r1Grid.num_cells[0]; x++) {
                    gridCellMins[0] = (x + r1Grid.cell_offset[0]) * r1Grid.scale;
                    gridCellMaxs[0] = gridCellMins[0] + r1Grid.scale;
                    if (testCollision(gridCellMins, gridCellMaxs, bounds.min, bounds.max)) {
                        int gridCellIndex = y * r1Grid.num_cells[0] + x;
                        gridCellsTouched.insert(gridCellIndex);
                    }
                }
            }

            PropData prop_data = {
                .index           = i,
                .bounds          = bounds,
                .collision_flags = collisionFlags,
                .unique_contents = uniqueContentsIndex};

            straddleGroupProps[gridCellsTouched].push_back(prop_data);

        }

        // TODO: seperate list for oversize props
        // -- extents.x >= 2048 on either X or Y axis seems reasonable
        // -- all go into a single GeoSet
        // -- that GeoSet will be indexed by the Worldspawn GridCell

        // assemble straddle groups
        std::vector<std::pair<titanfall::GeoSet, titanfall::Bounds>>  propGeoSets;
        std::map<int, std::set<int>>  cellStraddleGroups;
        // ^ {cell_index: {geo_set_index}}
        int32_t group_id = r1Grid.num_straddle_groups;
        for (auto [cells_set, props_data] : straddleGroupProps) {
            titanfall::GeoSet  geo_set;
            titanfall::Bounds  bounds;
            // straddle group
            if (cells_set.size() == 1) {
                geo_set.straddle_group = 0;
            } else {
                geo_set.straddle_group = static_cast<uint16_t>(group_id);
                group_id++;
            }
            // primitive(s)
            if (props_data.size() == 1) {
                geo_set.num_primitives = 1;
                PropData  prop_data = props_data[0];
                geo_set.primitive = (0x60 << 24) | (prop_data.index << 8) | (prop_data.unique_contents);
                // bounds
                bounds = bounds_from_minmax(props_data[0].bounds);
            } else {
                geo_set.num_primitives = static_cast<uint16_t>(props_data.size());
                uint16_t  index = static_cast<uint16_t>(r2Primitives.size());
                uint32_t  collision_flags = 0x00000000;
                // bounds
                MinMax  geoSetBounds;
                for (auto prop_data : props_data) {
                    // per-prop primitive & bounds
                    uint32_t  prop_primitive = (0x60 << 24) | (prop_data.index << 8) | (prop_data.unique_contents);
                    r2Primitives.push_back(prop_primitive);
                    titanfall::Bounds  prop_bounds = bounds_from_minmax(props_data[0].bounds);
                    r2PrimitiveBounds.push_back(prop_bounds);
                    // expand bounds
                    geoSetBounds.addVector(prop_data.bounds.min);
                    geoSetBounds.addVector(prop_data.bounds.max);
                    // combine contents_flags
                    collision_flags |= prop_data.collision_flags;
                }
                // get unique_contents_index of GeoSet
                int unique_contents_index = 0;
                for (uint32_t unique_contents : r2Contents) {
                    if (unique_contents != collision_flags) {
                        break;
                    }
                    unique_contents_index++;
                }
                if (unique_contents_index == r2Contents.size()) {
                    if (unique_contents_index > 0xFF) {
                        // NOTE: this should never happen, but we should still assert assumptions
                        fprintf(stderr, "UniqueContents too big\n");
                        exit(1);
                    }
                    r2Contents.push_back(collision_flags);
                }
                // index child Primitives & UniqueContents
                // NOTE: type is always 0 when num_primitives == 1
                geo_set.primitive = (index << 8) | (unique_contents_index);
                bounds = bounds_from_minmax(geoSetBounds);
            }

            // link GeoSet to GridCell(s)
            for (int cell_index : cells_set) {
                cellStraddleGroups[cell_index].insert(static_cast<int>(propGeoSets.size()));
            }
            propGeoSets.push_back({geo_set, bounds});
        }


        // add props to worldspawn GridCells
        int numWorldspawnGridCells = r1Grid.num_cells[0] * r1Grid.num_cells[1];
        for (int i = 0; i < numWorldspawnGridCells; i++) {
            titanfall::GridCell  r1GridCell = r1GridCells[i];
            titanfall::GridCell  r2GridCell;

            // copy GeoSets from r1
            r2GridCell.first_geo_set = static_cast<uint16_t>(r2GeoSets.size());
            r2GridCell.num_geo_sets  = r1GridCell.num_geo_sets;
            for (uint32_t j = 0; j < r1GridCell.num_geo_sets; j++) {
                r2GeoSets.push_back(r1GeoSets[r1GridCell.first_geo_set + j]);
                r2GeoSetBounds.push_back(r1GeoSetBounds[r1GridCell.first_geo_set + j]);
            }

            // TODO: optimisation:
            // if (r1Cell.num_geo_sets == 0) {
            //     r2Cell.num_geo_sets  = static_cast<uint16_t>(cellStraddleGroups[i].size());
            //     r2Cell.first_geo_set = ...;  // index previous appearance of cellStraddleGroups[i]
            // }

            // append prop GeoSets
            r2GridCell.num_geo_sets += static_cast<uint16_t>(cellStraddleGroups[i].size());
            for (auto geo_set_index : cellStraddleGroups[i]) {
                std::pair<titanfall::GeoSet, titanfall::Bounds>  pair;
                pair = propGeoSets[geo_set_index];  // {geo_set, bounds}
                r2GeoSets.push_back(pair.first);
                r2GeoSetBounds.push_back(pair.second);
            }
            r2GridCells.push_back(r2GridCell);
        }

        // copy GeoSets for each bsp Model
        uint32_t numBspModels = r1bsp.get_lump_length(titanfall::MODELS) / 32;
        for (uint32_t i = 0; i < numBspModels; i++) {
            titanfall::GridCell r1GridCell = r1GridCells[numWorldspawnGridCells + i];
            // copy GeoSets from r1
            for (uint32_t j = 0; j < r1GridCell.num_geo_sets; j++) {
                r2GeoSets.push_back(r1GeoSets[r1GridCell.first_geo_set + j]);
                r2GeoSetBounds.push_back(r1GeoSetBounds[r1GridCell.first_geo_set + j]);
            }
            r1GridCell.first_geo_set = static_cast<uint16_t>(r2GeoSets.size());
            r2GridCells.push_back(r1GridCell);
        }

        // check GeoSets limit
        if (r2GeoSets.size() > 0xFFFF) {
            fprintf(stderr, "Geosets too big: %d > 65535\n", (int)r2GeoSets.size());
            exit(1);
        }
    }
}


void convertTricoll(
    Bsp                                   &r1bsp,
    std::vector<titanfall::TricollHeader> &r2Header,
    std::vector<uint16_t>                 &r2BevelStarts,
    std::vector<uint32_t>                 &r2BevelIndices) {

    auto r1TricollHeader = r1bsp.get_lump<titanfall::TricollHeader>(titanfall::TRICOLL_HEADER);
    auto r1Indices       = r1bsp.get_lump<uint32_t>(titanfall::TRICOLL_BEVEL_INDICES);
    auto r1Starts        = r1bsp.get_lump<uint16_t>(titanfall::TRICOLL_BEVEL_STARTS);
    auto r1Tris          = r1bsp.get_lump<uint32_t>(titanfall::TRICOLL_TRIS);
    int headerCount = r1bsp.get_lump_length(titanfall::TRICOLL_HEADER) / sizeof(titanfall::TricollHeader);

    for (int i = 0; i < headerCount; i++) {
        titanfall::TricollHeader header = r1TricollHeader[i];
        uint32_t num_bevel_indices = header.num_bevel_indices;
        uint32_t first_bevel_index = header.first_bevel_index;
        header.first_bevel_index = (uint32_t)r2BevelIndices.size();
        r2Header.push_back(header);
        if (!num_bevel_indices) {
            continue;
        }

        uint32_t bevel_bytes = 4 * (((num_bevel_indices * 11) + 31) / 32) + 4;
        uint32_t *writeBuffer = (uint32_t*)malloc(bevel_bytes);
        memset(writeBuffer, 0, bevel_bytes);

        uint16_t *r1LocalStarts = &r1Starts[header.first_triangle];
        uint32_t *r1LocalTris = &r1Tris[header.first_triangle];
        uint32_t readIndices = 0;
        std::map<uint16_t, uint16_t> starts;
        for (int k = 0; k < header.num_triangles; k++) {
            uint16_t num_bevels = (r1LocalTris[k] >> 24) & 0xF;
            uint16_t start = r1LocalStarts[k];
            if (starts.contains(start)) {
                starts[start] = starts[start] > num_bevels ? starts[start] : num_bevels;
            } else {
                starts.emplace(start, num_bevels);
            }
        }
        for (std::pair<uint16_t, uint16_t> pair : starts) {
            uint16_t start = pair.first;
            uint16_t num_bevels = pair.second;
            BitReader read {&r1Indices[first_bevel_index], (uint64_t)(10 * start)};
            uint16_t writePtr = start;
            if (num_bevels == 15) {
                uint32_t index;
                do {
                    uint32_t data = read.Read10();
                    data |= (read.Read10() << 10);
                    write11Bit(writeBuffer, writePtr++ * 11, data & 0x7FF);
                    write11Bit(writeBuffer, writePtr++ * 11, data >> 11);
                    num_bevels = data & 0x7F;
                    index = data >> 7;
                    if (index >= r1TricollHeader.size()) {
                        fprintf(stderr, "Error Tricoll out of range\n");
                    }
                    for (uint32_t j = 0; j < num_bevels; j++) {
                        uint32_t val = read.Read10();
                        write11Bit(writeBuffer, writePtr++ * 11, val);
                        readIndices++;
                    }
                } while ((index != i) && num_bevels);
            } else {
                for (uint32_t j = 0; j < num_bevels; j++) {
                    uint32_t val = read.Read10();;
                    write11Bit(writeBuffer, writePtr++ * 11, val);
                }
            }
        }
        for (uint32_t j = 0; j < (num_bevel_indices * 11 + 31) / 32; j++) {
            r2BevelIndices.push_back(writeBuffer[j]);
        }
        free(writeBuffer);
    }
}


int convert(char *in_filename, char *out_filename) {
    Bsp  r1bsp(in_filename);
    if (!r1bsp.is_valid() || r1bsp.header_->version != titanfall::VERSION) {
        fprintf(stderr, "'%s' is not a Titanfall map!\n", in_filename);
        return 1;
    }

    memory_mapped_file outfile;
    const size_t reserved_size = 2 * r1bsp.file_.size();
    if (!outfile.open_new(out_filename, reserved_size)) {
        fprintf(stderr, "Could not open file for writing: '%s'\n", out_filename);
        return 1;
    }
    outfile.fill(0xAA);

    BspHeader &r2bsp_header = *outfile.rawdata<BspHeader>(0);
    r2bsp_header = {
        .magic    = MAGIC_rBSP,
        .version  = titanfall2::VERSION,
        .revision = r1bsp.header_->revision,
        ._127     = 127
    };

    // NOTE: we'll come back to write the new LumpHeaders later
    size_t write_cursor = sizeof(r2bsp_header);

    struct SortKey { int offset, index; };
    std::vector<SortKey> lumps;
    for (int i = 0; i < 128; i++) {
        int offset = static_cast<int>(r1bsp.header_->lumps[i].offset);
        if (offset != 0) {
            lumps.push_back({ offset, i });
        }
    }
    std::sort(lumps.begin(), lumps.end(), [](auto a, auto b) { return a.offset < b.offset; });

    #define WRITE_NULLS(byte_count) \
        memset(outfile.rawdata(write_cursor), 0, byte_count);

    // calculate Tricoll Data
    std::vector<titanfall::TricollHeader> r2TricollHeader;
    std::vector<uint32_t>                 r2BevelIndices;
    std::vector<uint16_t>                 r2BevelStarts;
    convertTricoll(r1bsp, r2TricollHeader, r2BevelStarts, r2BevelIndices);

    std::vector<titanfall::GeoSet>   r2GeoSets;
    std::vector<titanfall::Bounds>   r2GeoSetBounds;
    std::vector<titanfall::GridCell> r2GridCells;
    std::vector<uint32_t>            r2UniqueContents;
    std::vector<uint32_t>            r2Primitves;
    std::vector<titanfall::Bounds>   r2PrimitiveBounds;
    addPropsToCmGrid(r1bsp, r2GeoSets, r2GeoSetBounds, r2GridCells, r2UniqueContents, r2Primitves, r2PrimitiveBounds);

    for (auto &k : lumps) {
        int padding = 4 - (write_cursor % 4);
        if (padding != 4) {
            WRITE_NULLS(padding);
            write_cursor += padding;
        }

        LumpHeader &r1lump = r1bsp.header_->lumps[k.index];
        LumpHeader &r2lump = r2bsp_header.lumps[k.index];
        r2lump = {
            .offset  = static_cast<uint32_t>(write_cursor),
            .length  = r1lump.length,
            .version = r1lump.version,
            .fourCC  = r1lump.fourCC
        };

        #define WRITE_NEW_LUMP(T, v) \
            r2lump.length = static_cast<uint32_t>(sizeof(T) * v.size()); \
            memcpy(outfile.rawdata(write_cursor), reinterpret_cast<char*>(&v[0]), r2lump.length);

        switch (k.index) {

        case titanfall::GAME_LUMP: {
            auto r1GameLump = r1bsp.get_lump<char>(titanfall::GAME_LUMP);
            uint32_t readPtr = 20;
            uint32_t writePtr = static_cast<uint32_t>(write_cursor + 20);
            uint32_t num_model_names;
            memcpy(&num_model_names, &r1GameLump[readPtr], 4);
            // copy num_model_names + model_name table
            memcpy(outfile.rawdata(writePtr), &r1GameLump[readPtr], 4 + 128 * num_model_names);
            readPtr += 4 + num_model_names * 128; writePtr += 4 + num_model_names * 128;
            // NOTE: num_leaves is always 0 in r1; we can just ignore it
            uint32_t num_leaves;
            memcpy(&num_leaves, &r1GameLump[readPtr], 4);
            readPtr += 4 + 2 * num_leaves;
            uint32_t num_props;
            memcpy(&num_props, &r1GameLump[readPtr], 4);
            // copy num_props + unknown_1 & unknown_2
            memcpy(outfile.rawdata(writePtr), &r1GameLump[readPtr], 12);
            writePtr += 12; readPtr += 12;
            for (uint32_t i = 0; i < num_props; i++) {
                titanfall::StaticProp  r1Prop;
                memcpy(&r1Prop, &r1GameLump[readPtr], sizeof(titanfall::StaticProp));
                readPtr += sizeof(titanfall::StaticProp);
                titanfall2::StaticProp r2Prop;
                r2Prop = {
                    .origin                 = r1Prop.origin,
                    .angles                 = r1Prop.angles,
                    .scale                  = r1Prop.scale,
                    .model_name             = r1Prop.model_name,
                    .solid_type             = r1Prop.solid_type,
                    .flags                  = r1Prop.flags,
                    .skin                   = r1Prop.skin,
                    .cubemap                = r1Prop.cubemap,
                    .forced_fade_scale      = r1Prop.forced_fade_scale,
                    .lighting_origin        = r1Prop.lighting_origin,
                    .diffuse_modulation_r   = r1Prop.diffuse_modulation_r,
                    .diffuse_modulation_g   = r1Prop.diffuse_modulation_g,
                    .diffuse_modulation_b   = r1Prop.diffuse_modulation_b,
                    .diffuse_modulation_a   = r1Prop.diffuse_modulation_a,
                    .collision_flags_add    = r1Prop.collision_flags_add,
                    .collision_flags_remove = r1Prop.collision_flags_remove};
                memcpy(outfile.rawdata(writePtr), &r2Prop, sizeof(titanfall2::StaticProp));
                writePtr += sizeof(r2Prop);
            }
            memset(outfile.rawdata(writePtr), 0, 4);  // unknown3 count
            writePtr += 4;
            uint32_t  num_game_lumps = 1;
            source::GameLumpHeader glh;
            glh = {
                .id       = MAGIC_sprp,
                .flags    = 0x0000,
                .version  = 13,
                .offset   = (uint32_t)write_cursor + 20,
                .length   = writePtr - (uint32_t)write_cursor - 20};
            r2lump.length = writePtr - (uint32_t)write_cursor;
            memcpy(outfile.rawdata(write_cursor), &num_game_lumps, 4);
            memcpy(outfile.rawdata(write_cursor + 4), &glh, sizeof(glh));
        }
        break;

        case titanfall::LIGHTPROBE_REFS: {  // optional?
            auto lprs = r1bsp.get_lump<titanfall::LightProbeRef>(titanfall::LIGHTPROBE_REFS);
            std::vector<titanfall2::LightProbeRef> new_lprs;
            for (auto& lpr : lprs) {
                new_lprs.push_back({
                    .origin = lpr.origin,
                    .probe = lpr.probe,
                    .unknown = 0 });
            }
            WRITE_NEW_LUMP(titanfall2::LightProbeRef, new_lprs);

        }
        break;

        case titanfall::CM_GEO_SETS:
            WRITE_NEW_LUMP(titanfall::GeoSet, r2GeoSets);
            break;
        case titanfall::CM_GEO_SET_BOUNDS:
            WRITE_NEW_LUMP(titanfall::Bounds, r2GeoSetBounds);
            break;
        case titanfall::CM_GRID_CELLS:
            WRITE_NEW_LUMP(titanfall::GridCell, r2GridCells);
            break;
        case titanfall::CM_UNIQUE_CONTENTS:
            WRITE_NEW_LUMP(uint32_t, r2UniqueContents);
            break;
        case titanfall::CM_PRIMITIVES:
            WRITE_NEW_LUMP(uint32_t, r2Primitves);
            break;
        case titanfall::CM_PRIMITIVE_BOUNDS:
            WRITE_NEW_LUMP(titanfall::Bounds, r2PrimitiveBounds);
            break;

        case titanfall::REAL_TIME_LIGHTS: {  // NULLED OUT
            int texels = r1lump.length / 4;
            r2lump.length = texels * 9;
            WRITE_NULLS(r2lump.length);
        }
        break;

        case titanfall::TRICOLL_HEADER:
            WRITE_NEW_LUMP(titanfall::TricollHeader, r2TricollHeader);
            break;
        case titanfall::TRICOLL_BEVEL_INDICES:
            WRITE_NEW_LUMP(uint32_t, r2BevelIndices);
            break;
        default:  // copy raw lump bytes
            memcpy(outfile.rawdata(write_cursor), r1bsp.file_.rawdata(r1lump.offset), r1lump.length);
        }
        write_cursor += r2lump.length;
    }
    outfile.set_size_and_close(write_cursor);
    return 0;
}
