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
#include "filesystem.h"

#define PI 3.1415926536f
typedef const char ModelDictEntry[128];

void print_usage(char* argv0) {
    printf("USAGE: %s <game_dir> <titanfall.bsp> <titanfall2.bsp>\n", argv0);
    // printf("USAGE: %s -d titanfall_dir/ titanfall2_dir/\n", argv0);
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        print_usage(argv[0]);
        return 0;
    }
    char* game_dir = argv[1];
    char* in_filename = argv[2];
    char* out_filename = argv[3];

    // Initialize the file system with the provided game directory
    if (!InitFileSystem(game_dir, in_filename)) {
        fprintf(stderr, "Failed to initialize filesystem\n");
        return 1;
    }
    int ret = 0;
    try {
        int convert(char* in_filename, char* out_filename);
        ret = convert(in_filename, out_filename);
    }
    catch (std::exception& e) {
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
    auto r1PrimiviveBounds = r1bsp.get_lump<titanfall::Bounds>  (titanfall::CM_PRIMITIVE_BOUNDS);
    uint32_t submodelCount = r1bsp.get_lump_length(titanfall::MODELS) / 32;
    uint32_t subLumpCount = *(uint32_t*)&r1GameLump[0];
    uint32_t readPtr = 4;
    for (int i = 0; i < r1bsp.get_lump_length(titanfall::CM_PRIMITIVES) / 4; i++) {
        r2Primitives.push_back(r1Primitives[i]);
        r2PrimitiveBounds.push_back(r1PrimiviveBounds[i]);
    }
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

        std::vector<mstudiopertrihdr_t>  modelBoundingBoxes;
        std::vector<MinMax>              propBoundingBoxes;
        std::vector<uint32_t>            modelContents;
        for (int i = 0; i < r1bsp.get_lump_length(titanfall::CM_UNIQUE_CONTENTS) / 4; i++) {
            r2Contents.push_back(r1Contents[i]);
        }
        for (uint32_t i = 0; i < num_models; i++) {
            char buffer[1024];
            snprintf(buffer, 1024, "r1/%s", modelDict[i]);
            Model model {buffer};
            modelBoundingBoxes.push_back(*model.getPerTriHeader());
            modelContents.push_back(model.getContents());
        }
        for (uint32_t i = 0; i < num_props; i++) {
            if (props[i].solid_type == 0) {  // non-solid
                propBoundingBoxes.push_back(MinMax());
            } else {
                __m128 origin = _mm_set_ps(0, props[i].origin.z, props[i].origin.y, props[i].origin.x);
                __m128 scale = _mm_set1_ps(props[i].scale);
                mstudiopertrihdr_t &perTri = modelBoundingBoxes[props[i].model_name];
                MinMax minMax = minmax_from_instance_bounds(perTri.bbmin, perTri.bbmax, origin, props[i].angles, scale);
                // extend minMax to prevent semi solid collision
                propBoundingBoxes.push_back(minMax);
            }
        }
        float cellMins[2], cellMaxs[2];
        std::vector<uint32_t> modelUseCount;
        for (uint32_t i = 0; i < num_props; i++) { modelUseCount.push_back(0); }
        for (int y = 0; y < r1Grid.num_cells[1]; y++) {
            cellMins[1] = (y + r1Grid.cell_offset[1]) * r1Grid.scale;
            cellMaxs[1] = cellMins[1] + r1Grid.scale;
            for (int x = 0; x < r1Grid.num_cells[0]; x++) {
                cellMins[0] = (x + r1Grid.cell_offset[0]) * r1Grid.scale;
                cellMaxs[0] = cellMins[0] + r1Grid.scale;
                titanfall::GridCell  r1Cell = r1GridCells[y * r1Grid.num_cells[0] + x];
                titanfall::GridCell &r2Cell = r2GridCells.emplace_back();
                r2Cell.first_geo_set = (uint16_t)r2GeoSets.size();
                r2Cell.num_geo_sets = r1Cell.num_geo_sets;
                // add existing geosets to r2 cell
                for (uint32_t i = 0; i < r1Cell.num_geo_sets; i++) {
                    r2GeoSets.push_back(r1GeoSets[r1Cell.first_geo_set + i]);
                    r2GeoSetBounds.push_back(r1GeoSetBounds[r1Cell.first_geo_set + i]);
                }
                std::vector<MinMax> propPrimitiveBounds;
                std::vector<uint32_t> propPrimitives;
                uint32_t geoSetCollisionFlags = 0;
                for (uint32_t i = 0; i < num_props; i++) {
                    if (props[i].solid_type == 0) { continue; }
                    MinMax &propBound = propBoundingBoxes[i];
                    if (!testCollision(cellMins, cellMaxs, propBound.min, propBound.max)) {
                        continue;
                    }
                    // add prop to cell
                    modelUseCount[i]++;
                    // this is from the prop loading function
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
                    geoSetCollisionFlags |= collisionFlags;
                    // this could be optimized to do it for each model
                    uint32_t contentsIndex = 0;
                    for (uint32_t cont : r2Contents) {
                        if (cont == collisionFlags) { break; }
                        contentsIndex++;
                    }
                    if (contentsIndex == r2Contents.size()) {
                        r2Contents.push_back(collisionFlags);
                    }
                    uint32_t prim = (contentsIndex & 0xFF) | ((i & 0x1FFFFF) << 8) | (3 << 29);
                    propPrimitives.push_back(prim);
                    propPrimitiveBounds.push_back(propBound);
                }
                if (propPrimitives.size() == 1) {
                    titanfall::Bounds &propGeoSetBounds = r2GeoSetBounds.emplace_back();
                    propGeoSetBounds = bounds_from_minmax(propPrimitiveBounds[0]);
                    titanfall::GeoSet &propGeoSet = r2GeoSets.emplace_back();
                    propGeoSet = {
                        .straddle_group = 0,
                        .num_primitives = 1,
                        .first_primitive = propPrimitives[0]};
                    r2Cell.num_geo_sets++;
                }
                else if (propPrimitives.size() > 1) {
                    MinMax bounds = propPrimitiveBounds[0];
                    for (uint32_t i = 1; i < propPrimitiveBounds.size(); i++) {
                        bounds.addVector(propPrimitiveBounds[i].min);
                        bounds.addVector(propPrimitiveBounds[i].max);
                    }
                    titanfall::Bounds &propGeoSetBounds = r2GeoSetBounds.emplace_back();
                    propGeoSetBounds = bounds_from_minmax(bounds);
                    titanfall::GeoSet &propGeoSet = r2GeoSets.emplace_back();
                    propGeoSet = {
                        .straddle_group = 0,
                        .num_primitives = (uint16_t)propPrimitives.size(),
                        .first_primitive = (uint32_t)((propPrimitives.size() & 0x1FFFFF) << 8)};
                    r2Cell.num_geo_sets++;
                    for (size_t i = 0; i < propPrimitives.size(); i++) {
                        r2Primitives.push_back(propPrimitives[i]);
                        titanfall::Bounds &primBounds = r2PrimitiveBounds.emplace_back();
                        primBounds = bounds_from_minmax(propPrimitiveBounds[i]);
                    }
                }
            }
        }
        if (r2GeoSets.size() > 0xFFFF) {
            fprintf(stderr, "Geosets too big\n");
            exit(1);
        }
        for (uint32_t i = 0; i < submodelCount; i++) {
            titanfall::GridCell cell = r1GridCells[r2GridCells.size()];
            uint32_t start = cell.first_geo_set;
            cell.first_geo_set = r2GeoSets.size();
            for (uint32_t j = 0; j < cell.num_geo_sets; j++) {
                r2GeoSets.push_back(r1GeoSets[start + j]);
                r2GeoSetBounds.push_back(r1GeoSetBounds[start + j]);
            }
            r2GridCells.push_back(cell);
        }
        if (r2GeoSets.size() > 0xFFFF) {
            fprintf(stderr, "Geosets too big\n");
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
    Bsp  r1bsp((std::string("maps/") + in_filename).c_str());
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
