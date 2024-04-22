#include <algorithm>
#include <cstdio>
#include <fstream>
#include <set>
#include <map>
#include <immintrin.h>


#include "memory_mapped_file.hpp"
#include "bsp.hpp"
#include "source.hpp"  // GameLumpHeader
#include "models.h"
#include "titanfall.hpp"
#include "titanfall2.hpp"


void print_usage(char* argv0) {
    printf("USAGE: %s titanfall.bsp titanfall2.bsp\n", argv0);
    // printf("USAGE: %s -d titanfall_dir/ titanfall2_dir/\n", argv0);
}


int main(int argc, char* argv[]) {
    if (argc != 3) {
        print_usage(argv[0]);
        return 0;
    }
    char* in_filename = argv[1];
    char* out_filename = argv[2];

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

struct BitReader {
    uint64_t buffer;
    uint64_t bitsReadFromBuffer;
    uint32_t* fillBuffer;

    BitReader(uint32_t* startBuffer, uint64_t startBit) {
        fillBuffer = &startBuffer[startBit / 32];
        buffer = *fillBuffer++;
        buffer |= (uint64_t)*fillBuffer++ << 32;
        buffer = buffer >> (startBit & 0x1F);
        bitsReadFromBuffer = startBit & 0x1F;

    }
    uint32_t Read10() {


        uint32_t res = buffer & 0x3FF;
        buffer = buffer >> 10;
        bitsReadFromBuffer += 10i64;
        if (bitsReadFromBuffer >= 0x20)
        {
            buffer = buffer | ((uint64_t)*fillBuffer++ << (64 - bitsReadFromBuffer));
            bitsReadFromBuffer -= 32;
        }
        return res;
    }
};

void write11Bit(uint32_t* writeBuffer, uint64_t offset, uint32_t data) {
    uint64_t intOffset = offset / 32;
    uint32_t bitOffset = offset & 0x1F;
    uint64_t buffer = (uint64_t)writeBuffer[intOffset] | (((uint64_t)writeBuffer[intOffset + 1]) << 32);
    buffer &= ~(0x7FFll << bitOffset);
    buffer |= ((uint64_t)(data & 0x7FF)) << bitOffset;
    writeBuffer[intOffset] = (uint32_t)buffer;
    writeBuffer[intOffset + 1] = buffer >> 32;
}

typedef const char modelDictEntry[128];

#define PI 3.1415926536f
__m128 rotate(const __m128& vec, Vector3 angles) {

    __m128 res = vec;
    float cosVal = cos(angles.x);
    float sinVal = sin(angles.x);
    __m128 cosIntrin = _mm_set_ps(0, cosVal, 1, cosVal);
    __m128 sinIntrin = _mm_set_ps(0, -sinVal, 0, -sinVal);


    res = _mm_add_ps(_mm_mul_ps(res, cosIntrin), _mm_mul_ps(sinIntrin, _mm_shuffle_ps(res, res, _MM_SHUFFLE(3, 0, 3, 2))));

    cosVal = cos(angles.y);
    sinVal = sin(angles.y);
    cosIntrin = _mm_set_ps(0, 1, cosVal, cosVal);
    sinIntrin = _mm_set_ps(0, 0, sinVal, -sinVal);

    res = _mm_add_ps(_mm_mul_ps(res, cosIntrin), _mm_mul_ps(sinIntrin, _mm_shuffle_ps(res, res, _MM_SHUFFLE(3, 3, 0, 1))));

    cosVal = cos(angles.y);
    sinVal = sin(angles.y);
    cosIntrin = _mm_set_ps(0, cosVal, cosVal, 1);
    sinIntrin = _mm_set_ps(0, sinVal, -sinVal, 0);

    res = _mm_add_ps(_mm_mul_ps(res, cosIntrin), _mm_mul_ps(sinIntrin, _mm_shuffle_ps(res, res, _MM_SHUFFLE(3, 1, 2, 3))));

    return res;
}

bool testCollision(float* cellMins, float* cellMaxs, __m128 propMin, __m128 propMax) {
    if (((cellMins[0] - 1) > propMax.m128_f32[0]) || ((cellMaxs[0] + 1) < propMin.m128_f32[0]))return false;
    if (((cellMins[1] - 1) > propMax.m128_f32[1]) || ((cellMaxs[1] + 1) < propMin.m128_f32[1]))return false;
    return true;
}
struct MinMax {
    __m128 min;
    __m128 max;
    MinMax() {
        min = _mm_setzero_ps();
        max = _mm_setzero_ps();
    }
    MinMax(const __m128& start) {
        min = start;
        max = start;
    }
    void addVector(const __m128& vec) {
        min = _mm_min_ps(min, vec);
        max = _mm_max_ps(max, vec);
    }
};
void addPropsToCmGrid(Bsp& r1bsp, std::vector<source::GeoSet>& r2GeoSets, std::vector<source::GeoSetBounds>& r2GeoSetBounds, std::vector<source::GridCell>& r2GridCells, std::vector<uint32_t>& r2Contents, std::vector<uint32_t>& r2Primitives, std::vector<source::GeoSetBounds>& r2PrimitiveBounds) {

    auto GameLump = r1bsp.get_lump<char>(titanfall::GAME_LUMP);
    source::CMGrid cmGrid = r1bsp.get_lump<source::CMGrid>(titanfall::CM_GRID)[0];
    auto r1GridCells = r1bsp.get_lump<source::GridCell>(titanfall::CM_GRID_CELLS);
    auto r1GeoSets = r1bsp.get_lump<source::GeoSet>(titanfall::CM_GEO_SETS);
    auto r1GeoSetBounds = r1bsp.get_lump<source::GeoSetBounds>(titanfall::CM_GEO_SET_BOUNDS);
    auto r1Contents = r1bsp.get_lump<uint32_t>(titanfall::CM_UNIQUE_CONTENTS);
    auto r1Primitives = r1bsp.get_lump<uint32_t>(titanfall::CM_PRIMITIVES);
    auto r1PrimiviveBounds = r1bsp.get_lump<source::GeoSetBounds>(titanfall::CM_PRIMITIVE_BOUNDS);
    uint32_t submodelCount = r1bsp.get_lump_length(titanfall::MODELS) / 32;
    uint32_t subLumpCount = *(uint32_t*)&GameLump[0];
    uint32_t readPtr = 4;

    for (uint32_t i = 0; i < r1bsp.get_lump_length(titanfall::CM_PRIMITIVES) / 4; i++) {
        r2Primitives.push_back(r1Primitives[i]);
        r2PrimitiveBounds.push_back(r1PrimiviveBounds[i]);
    }

    for (uint32_t lumpIndex = 0; lumpIndex < subLumpCount; lumpIndex++) {
        source::GameLumpHeader header = *(source::GameLumpHeader*)&GameLump[readPtr];
        readPtr += sizeof(source::GameLumpHeader);
        if (header.id != MAGIC_sprp) {
            readPtr += header.length;
            continue;
        }
        uint32_t modelCount = *(uint32_t*)&GameLump[readPtr];
        readPtr += 4;
        modelDictEntry* modelDict = (modelDictEntry*)&GameLump[readPtr];
        readPtr += 128 * modelCount;
        uint32_t leafCount = *(uint32_t*)&GameLump[readPtr];
        readPtr += 4 + 2 * leafCount;
        uint32_t propCount = *(uint32_t*)&GameLump[readPtr];
        readPtr += 12;
        titanfall::StaticProp* props = (titanfall::StaticProp*)&GameLump[readPtr];
        std::vector<mstudiopertrihdr_t> modelBoundingBoxes;
        std::vector<MinMax> propBoundingBoxes;
        std::vector<uint32_t> modelContents;
        for (int i = 0; i < r1bsp.get_lump_length(titanfall::CM_UNIQUE_CONTENTS) / 4; i++) {
            r2Contents.push_back(r1Contents[i]);
        }
        for (uint32_t i = 0; i < modelCount; i++) {
            char buffer[1024];
            snprintf(buffer, 1024, "r1/%s", modelDict[i]);
            Model model{ buffer };
            modelBoundingBoxes.push_back(*model.getPerTriHeader());
            modelContents.push_back(model.getContents());
        }

        for (uint32_t i = 0; i < propCount; i++) {
            if (props[i].solidType == 0) {
                propBoundingBoxes.push_back(MinMax());
            }
            else {
                __m128 scale = _mm_set1_ps(props[i].scale);
                __m128 origin = _mm_set_ps(0, props[i].m_Origin.z, props[i].m_Origin.y, props[i].m_Origin.x);
                mstudiopertrihdr_t& perTri = modelBoundingBoxes[props[i].modelIndex];
                MinMax minMax(_mm_add_ps(origin, rotate(_mm_set_ps(perTri.bbmin.x, perTri.bbmin.y, perTri.bbmin.z, 0), props[i].m_Angles)));
                minMax.addVector(_mm_add_ps(origin, rotate(_mm_mul_ps(_mm_set_ps(0, perTri.bbmin.z, perTri.bbmin.y, perTri.bbmax.x), scale), props[i].m_Angles)));
                minMax.addVector(_mm_add_ps(origin, rotate(_mm_mul_ps(_mm_set_ps(0, perTri.bbmin.z, perTri.bbmax.y, perTri.bbmin.x), scale), props[i].m_Angles)));
                minMax.addVector(_mm_add_ps(origin, rotate(_mm_mul_ps(_mm_set_ps(0, perTri.bbmin.z, perTri.bbmax.y, perTri.bbmax.x), scale), props[i].m_Angles)));
                minMax.addVector(_mm_add_ps(origin, rotate(_mm_mul_ps(_mm_set_ps(0, perTri.bbmax.z, perTri.bbmin.y, perTri.bbmin.x), scale), props[i].m_Angles)));
                minMax.addVector(_mm_add_ps(origin, rotate(_mm_mul_ps(_mm_set_ps(0, perTri.bbmax.z, perTri.bbmin.y, perTri.bbmax.x), scale), props[i].m_Angles)));
                minMax.addVector(_mm_add_ps(origin, rotate(_mm_mul_ps(_mm_set_ps(0, perTri.bbmax.z, perTri.bbmax.y, perTri.bbmin.x), scale), props[i].m_Angles)));
                minMax.addVector(_mm_add_ps(origin, rotate(_mm_mul_ps(_mm_set_ps(0, perTri.bbmax.z, perTri.bbmax.y, perTri.bbmax.x), scale), props[i].m_Angles)));
                //extend minMax to prevent semi solid collision
                propBoundingBoxes.push_back(minMax);

            }

        }
        float cellMins[2];
        float cellMaxs[2];
        std::vector<uint32_t> modelUseCount;
        for (uint32_t i = 0; i < propCount; i++) {
            modelUseCount.push_back(0);
        }
        for (int y = 0; y < cmGrid.cellCount[1]; y++) {
            cellMins[1] = (y + cmGrid.cellOrg[1]) * cmGrid.cellSize;
            cellMaxs[1] = cellMins[1] + cmGrid.cellSize;
            for (int x = 0; x < cmGrid.cellCount[0]; x++) {
                cellMins[0] = (x + cmGrid.cellOrg[0]) * cmGrid.cellSize;
                cellMaxs[0] = cellMins[0] + cmGrid.cellSize;
                source::GridCell r1Cell = r1GridCells[y * cmGrid.cellCount[0] + x];
                source::GridCell& r2Cell = r2GridCells.emplace_back();
                r2Cell.geoSetStart = (uint16_t)r2GeoSets.size();
                r2Cell.geoSetCount = r1Cell.geoSetCount;
                //add existing geosets to r2 cell
                for (uint32_t i = 0; i < r1Cell.geoSetCount; i++) {
                    r2GeoSets.push_back(r1GeoSets[r1Cell.geoSetStart + i]);
                    r2GeoSetBounds.push_back(r1GeoSetBounds[r1Cell.geoSetStart + i]);
                }
                std::vector<MinMax> propPrimitiveBounds;
                std::vector<uint32_t> propPrimitives;
                uint32_t geoSetCollisionFlags = 0;
                for (uint32_t i = 0; i < propCount; i++) {

                    if (props[i].solidType == 0)continue;
                    MinMax& propBound = propBoundingBoxes[i];
                    if (!testCollision(cellMins, cellMaxs, propBound.min, propBound.max))
                        continue;
                    //add prop to cell

                    modelUseCount[i]++;


                    //this is from the prop loading function
                    uint32_t collisionFlags = modelContents[props[i].modelIndex];
                    if ((collisionFlags & 1) != 0 || !collisionFlags)
                        collisionFlags = collisionFlags & 0xFFFFFFFE | 0xEB0280;
                    if ((collisionFlags & 2) != 0)
                        collisionFlags = collisionFlags & 0xFFF7FFFD | 0xE30240;
                    if ((collisionFlags & 8) != 0)
                        collisionFlags = collisionFlags & 0xFFB7FFF7 | 0xA30240;
                    collisionFlags &= ~props[i].collision_flags_remove;
                    geoSetCollisionFlags |= collisionFlags;
                    //this could be optimized to do it for each model
                    uint32_t contentsIndex = 0;
                    for (uint32_t cont : r2Contents) {
                        if (cont == collisionFlags)break;
                        contentsIndex++;
                    }
                    if (contentsIndex == r2Contents.size())
                        r2Contents.push_back(collisionFlags);
                    uint32_t prim = (contentsIndex & 0xFF) | ((i & 0x1FFFFF) << 8) | (3 << 29);
                    propPrimitives.push_back(prim);
                    propPrimitiveBounds.push_back(propBound);

                }
                if (propPrimitives.size() == 1) {
                    source::GeoSet& propGeoSet = r2GeoSets.emplace_back();

                    source::GeoSetBounds& propGeoSetBounds = r2GeoSetBounds.emplace_back();

                    __m128 origin = _mm_div_ps(_mm_add_ps(propPrimitiveBounds[0].min, propPrimitiveBounds[0].max), _mm_set1_ps(2));
                    __m128 extends = _mm_add_ps(_mm_sub_ps(propPrimitiveBounds[0].max, origin), _mm_set1_ps(2));
                    propGeoSetBounds.origin[0] = (short)origin.m128_f32[0];
                    propGeoSetBounds.origin[1] = (short)origin.m128_f32[1];
                    propGeoSetBounds.origin[2] = (short)origin.m128_f32[2];
                    propGeoSetBounds.cos = 0x8000;
                    propGeoSetBounds.sin = 0;
                    propGeoSetBounds.extends[0] = (short)extends.m128_f32[0];
                    propGeoSetBounds.extends[1] = (short)extends.m128_f32[1];
                    propGeoSetBounds.extends[2] = (short)extends.m128_f32[2];


                    r2Cell.geoSetCount++;
                    propGeoSet.primCount = 1;
                    propGeoSet.primStart = propPrimitives[0];
                    propGeoSet.straddleGroup = 0;
                }
                else if (propPrimitives.size() > 1) {
                    MinMax bounds = propPrimitiveBounds[0];
                    for (uint32_t i = 1; i < propPrimitiveBounds.size(); i++) {
                        bounds.addVector(propPrimitiveBounds[i].min);
                        bounds.addVector(propPrimitiveBounds[i].max);
                    }
                    source::GeoSet& propGeoSet = r2GeoSets.emplace_back();

                    source::GeoSetBounds& propGeoSetBounds = r2GeoSetBounds.emplace_back();

                    __m128 origin = _mm_div_ps(_mm_add_ps(bounds.min, bounds.max), _mm_set1_ps(2));
                    __m128 extends = _mm_add_ps(_mm_sub_ps(bounds.max, origin), _mm_set1_ps(2));
                    propGeoSetBounds.origin[0] = (short)origin.m128_f32[0];
                    propGeoSetBounds.origin[1] = (short)origin.m128_f32[1];
                    propGeoSetBounds.origin[2] = (short)origin.m128_f32[2];
                    propGeoSetBounds.cos = 0x8000;
                    propGeoSetBounds.sin = 0;
                    propGeoSetBounds.extends[0] = (short)extends.m128_f32[0];
                    propGeoSetBounds.extends[1] = (short)extends.m128_f32[1];
                    propGeoSetBounds.extends[2] = (short)extends.m128_f32[2];


                    r2Cell.geoSetCount++;
                    propGeoSet.primCount = propPrimitives.size();
                    propGeoSet.primStart = ((r2Primitives.size() & 0x1FFFFF) << 8);
                    propGeoSet.straddleGroup = 0;
                    for (int i = 0; i < propPrimitives.size(); i++) {
                        r2Primitives.push_back(propPrimitives[i]);
                        source::GeoSetBounds& primBounds = r2PrimitiveBounds.emplace_back();
                        __m128 origin = _mm_div_ps(_mm_add_ps(propPrimitiveBounds[i].min, propPrimitiveBounds[i].max), _mm_set1_ps(2));
                        __m128 extends = _mm_add_ps(_mm_sub_ps(propPrimitiveBounds[i].max, origin), _mm_set1_ps(2));
                        primBounds.origin[0] = (short)origin.m128_f32[0];
                        primBounds.origin[1] = (short)origin.m128_f32[1];
                        primBounds.origin[2] = (short)origin.m128_f32[2];
                        primBounds.cos = 0x8000;
                        primBounds.sin = 0;
                        primBounds.extends[0] = (short)extends.m128_f32[0];
                        primBounds.extends[1] = (short)extends.m128_f32[1];
                        primBounds.extends[2] = (short)extends.m128_f32[2];
                    }
                }
            }
        }
        if (r2GeoSets.size() > 0xFFFF) {
            fprintf(stderr, "Geosets too big\n");
            exit(1);
        }
        for (uint32_t i = 0; i < submodelCount; i++) {
            source::GridCell cell = r1GridCells[r2GridCells.size()];
            uint32_t start = cell.geoSetStart;
            cell.geoSetStart = r2GeoSets.size();
            for (uint32_t j = 0; j < cell.geoSetCount; j++) {
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

void convertTricoll(Bsp& r1bsp, std::vector<source::Tricoll_Header>& r2Header, std::vector<uint16_t>& r2BevelStarts, std::vector<uint32_t>& r2BevelIndices) {
    auto r1TricollHeader = r1bsp.get_lump<source::Tricoll_Header>(titanfall::TRICOLL_HEADER);
    int headerCount = r1bsp.get_lump_length(titanfall::TRICOLL_HEADER) / sizeof(source::Tricoll_Header);
    auto r1Indices = r1bsp.get_lump<uint32_t>(titanfall::TRICOLL_BEVEL_INDICES);
    auto r1Starts = r1bsp.get_lump<uint16_t>(titanfall::TRICOLL_BEVEL_STARTS);
    auto r1Tris = r1bsp.get_lump<uint32_t>(titanfall::TRICOLL_TRIS);

    for (int i = 0; i < headerCount; i++) {
        source::Tricoll_Header header = r1TricollHeader[i];
        uint32_t indicesCount = header.bevelIndicesCount;
        uint32_t indicesStart = header.bevelIndicesStart;
        header.bevelIndicesStart = (uint32_t)r2BevelIndices.size();
        r2Header.push_back(header);


        if (!indicesCount)continue;


        size_t maxWritePtr = 0;

        uint32_t* writeBuffer = (uint32_t*)malloc(4 * (((indicesCount * 11) + 31) / 32) + 4);
        memset(writeBuffer, 0, 4 * (((indicesCount * 11) + 31) / 32) + 4);

        uint16_t* r1LocalStarts = &r1Starts[header.trisStart];
        uint32_t* r1LocalTris = &r1Tris[header.trisStart];
        uint32_t readIndices = 0;
        std::map<uint16_t, uint16_t> starts;
        for (int k = 0; k < header.trisCount; k++) {
            uint16_t bevelCount = (r1LocalTris[k] >> 24) & 0xF;
            uint16_t start = r1LocalStarts[k];
            if (starts.contains(start)) {
                starts[start] = starts[start] > bevelCount ? starts[start] : bevelCount;
            }
            else {
                starts.emplace(start, bevelCount);
            }

        }
        for (std::pair<uint16_t, uint16_t> pair : starts) {
            uint16_t start = pair.first;
            uint16_t bevelCount = pair.second;

            BitReader read{ &r1Indices[indicesStart],10u * start };
            uint16_t writePtr = start;

            if (bevelCount == 15) {
                uint32_t index;
                uint32_t startReadIndices = readIndices;
                do {
                    uint32_t data = read.Read10();
                    data |= (read.Read10() << 10);
                    write11Bit(writeBuffer, writePtr++ * 11, data & 0x7FF);
                    write11Bit(writeBuffer, writePtr++ * 11, data >> 11);
                    bevelCount = data & 0x7F;
                    index = data >> 7;

                    if (index >= r1TricollHeader.size()) {
                        fprintf(stderr, "Error Tricoll out of range\n");
                    }

                    for (uint32_t j = 0; j < bevelCount; j++) {
                        uint32_t val = read.Read10();
                        write11Bit(writeBuffer, writePtr++ * 11, val);
                        readIndices++;

                    }
                } while ((index != i) && bevelCount);

            }
            else {

                for (uint32_t j = 0; j < bevelCount; j++) {
                    uint32_t val = read.Read10();;
                    write11Bit(writeBuffer, writePtr++ * 11, val);
                }
            }


        }

        for (uint32_t j = 0; j < (indicesCount * 11 + 31) / 32; j++) {
            r2BevelIndices.push_back(writeBuffer[j]);
        }
        free(writeBuffer);


    }
}

int convert(char* in_filename, char* out_filename) {
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

    BspHeader& r2bsp_header = *outfile.rawdata<BspHeader>(0);
    r2bsp_header = {
        .magic = MAGIC_rBSP,
        .version = titanfall2::VERSION,
        .revision = r1bsp.header_->revision,
        ._127 = 127
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


    //calculate Tricoll Data
    std::vector<source::Tricoll_Header> r2TricollHeader;
    std::vector<uint32_t> r2BevelIndices;
    std::vector<uint16_t> r2BevelStarts;

    convertTricoll(r1bsp, r2TricollHeader, r2BevelStarts, r2BevelIndices);

    std::vector<source::GeoSet> r2GeoSets;
    std::vector<source::GeoSetBounds> r2GeoSetBounds;
    std::vector<source::GridCell> r2GridCells;
    std::vector<uint32_t> r2UniqueContents;
    std::vector<uint32_t> r2Primitves;
    std::vector<source::GeoSetBounds> r2PrimitiveBounds;
    addPropsToCmGrid(r1bsp, r2GeoSets, r2GeoSetBounds, r2GridCells, r2UniqueContents, r2Primitves, r2PrimitiveBounds);

    for (auto& k : lumps) {
        int padding = 4 - (write_cursor % 4);
        if (padding != 4) {
            WRITE_NULLS(padding);
            write_cursor += padding;
        }

        LumpHeader& r1lump = r1bsp.header_->lumps[k.index];
        LumpHeader& r2lump = r2bsp_header.lumps[k.index];
        r2lump = {
            .offset = static_cast<uint32_t>(write_cursor),
            .length = r1lump.length,
            .version = r1lump.version,
            .fourCC = r1lump.fourCC
        };

#define WRITE_NEW_LUMP(T, v) \
            r2lump.length = static_cast<uint32_t>(sizeof(T) * v.size()); \
            memcpy(outfile.rawdata(write_cursor), reinterpret_cast<char*>(&v[0]), r2lump.length);

        // TODO: Tricoll (https://github.com/snake-biscuits/bsp_tool/discussions/106)
        switch (k.index) {

        case titanfall::GAME_LUMP: {  // NULLED OUT
            /* TODO: modify sprp GAME_LUMP */
            // uint32_t  num_mdl_names; char     mdl_names[num_mdl_names][128];  /* COPY */
            // uint32_t  leaf_count;    uint16_t leaves[leaf_count];             /* SKIP */
            // uint32_t  unknown[2];                                             /* COPY */
            // uint32_t  num_props;     StaticProp props[num_props];             /* CONVERT */
            // uint32_t  num_unknown;                                            /* ADD (0) */

            auto r1GameLump = r1bsp.get_lump<char>(titanfall::GAME_LUMP);






            uint32_t readPtr = 20;
            uint32_t writePtr = (uint32_t)write_cursor + 20;
            uint32_t modelNameCount, leafCount, propCount;
            memcpy(&modelNameCount, &r1GameLump[readPtr], 4);
            memcpy(outfile.rawdata(writePtr), &r1GameLump[readPtr], 4 + 128 * modelNameCount);
            readPtr += 4 + modelNameCount * 128;
            writePtr += 4 + modelNameCount * 128;
            memcpy(&leafCount, &r1GameLump[readPtr], 4);
            readPtr += 4 + 2 * leafCount;//skipLeafs
            memcpy(&propCount, &r1GameLump[readPtr], 4);
            memcpy(outfile.rawdata(writePtr), &r1GameLump[readPtr], 12);
            writePtr += 12;
            readPtr += 12;

            for (uint32_t i = 0; i < propCount; i++) {
                titanfall::StaticProp r1Prop;
                titanfall2::StaticProp r2Prop;
                memset(&r2Prop, 0, sizeof(r2Prop));
                memcpy(&r1Prop, &r1GameLump[readPtr], sizeof(titanfall::StaticProp));
                readPtr += sizeof(titanfall::StaticProp);


                r2Prop.m_Origin = r1Prop.m_Origin;
                r2Prop.m_Angles = r1Prop.m_Angles;
                r2Prop.scale = r1Prop.scale;
                r2Prop.modelIndex = r1Prop.modelIndex;
                r2Prop.m_Solid = r1Prop.solidType;
                r2Prop.m_flags = r1Prop.flags;
                r2Prop.skin = r1Prop.skin;
                r2Prop.word_22 = r1Prop.word_22;
                r2Prop.forced_fade_scale = r1Prop.forced_fade_scale;
                r2Prop.m_LightingOrigin = r1Prop.lightingOrigin;

                r2Prop.m_DiffuseModulation_r = r1Prop.m_DiffuseModulation_r;
                r2Prop.m_DiffuseModulation_g = r1Prop.m_DiffuseModulation_g;
                r2Prop.m_DiffuseModulation_b = r1Prop.m_DiffuseModulation_b;
                r2Prop.m_DiffuseModulation_a = r1Prop.m_DiffuseModulation_a;
                r2Prop.unk = 0;
                r2Prop.collision_flags_remove = r1Prop.collision_flags_remove;
                //printf("%08X %08X\n",r1Prop.collision_flags_add,r1Prop.collision_flags_remove);
                memcpy(outfile.rawdata(writePtr), &r2Prop, sizeof(r2Prop));
                writePtr += sizeof(r2Prop);
            }


            memset(outfile.rawdata(writePtr), 0, 4);//unknown3 count
            writePtr += 4;
            uint32_t  num_game_lumps = 1;
            source::GameLumpHeader  glh;
            glh.id = 0x73707270;//'sprp'
            glh.offset = (uint32_t)write_cursor + 20;
            glh.version = 13;
            glh.length = writePtr - (uint32_t)write_cursor - 20;
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
            WRITE_NEW_LUMP(source::GeoSet, r2GeoSets);
            break;
        case titanfall::CM_GEO_SET_BOUNDS:
            WRITE_NEW_LUMP(source::GeoSetBounds, r2GeoSetBounds);
            break;
        case titanfall::CM_GRID_CELLS:
            WRITE_NEW_LUMP(source::GridCell, r2GridCells);
            break;
        case titanfall::CM_UNIQUE_CONTENTS:
            WRITE_NEW_LUMP(uint32_t, r2UniqueContents);
            break;
        case titanfall::CM_PRIMITIVES:
            WRITE_NEW_LUMP(uint32_t, r2Primitves);
            break;
        case titanfall::CM_PRIMITIVE_BOUNDS:
            WRITE_NEW_LUMP(source::GeoSetBounds, r2PrimitiveBounds);
            break;
        case titanfall::REAL_TIME_LIGHTS: {  // NULLED OUT
            int texels = r1lump.length / 4;
            r2lump.length = texels * 9;
            WRITE_NULLS(r2lump.length);
        }
        break;
        case titanfall::TRICOLL_HEADER:
            WRITE_NEW_LUMP(source::Tricoll_Header, r2TricollHeader);
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
