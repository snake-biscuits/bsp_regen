#include <algorithm>
#include <cstdio>
#include <fstream>

#include "memory_mapped_file.hpp"
#include "bsp.hpp"
#include "source.hpp"  // GameLumpHeader
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
    char* in_filename  = argv[1];
    char* out_filename = argv[2];

    int ret = 0;
    try {
        int convert(char* in_filename, char* out_filename);
        ret = convert(in_filename, out_filename);
    } catch (std::exception& e) {
        fprintf(stderr, "Exception: %s\n", e.what());
        return 1;
    }

    return ret;
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
        .magic    = MAGIC_rBSP,
        .version  = titanfall2::VERSION,
        .revision = r1bsp.header_->revision,
        ._127     = 127
    };
    // NOTE: we'll come back to write the new LumpHeaders later
    int write_cursor = sizeof(r2bsp_header);

    struct SortKey { int offset, index; };
    std::vector<SortKey> lumps;
    for (int i = 0; i < 128; i++) {
        int offset = static_cast<int>(r1bsp.header_->lumps[i].offset);
        if (offset != 0) {
            lumps.push_back({offset, i});
        }
    }
    std::sort(lumps.begin(), lumps.end(), [](auto a, auto b) { return a.offset < b.offset; });

    #define WRITE_NULLS(byte_count) \
        memset(outfile.rawdata(write_cursor), 0, byte_count);

    for (auto &k : lumps) {
        int padding = 4 - (write_cursor % 4);
        if (padding != 4) {
            WRITE_NULLS(padding);
            write_cursor += padding;
        }

        LumpHeader& r1lump = r1bsp.header_->lumps[k.index];
        LumpHeader& r2lump = r2bsp_header.lumps[k.index];
        r2lump = {
            .offset  = static_cast<uint32_t>(write_cursor),
            .length  = r1lump.length,
            .version = r1lump.version,
            .fourCC  = r1lump.fourCC
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

                /* valid empty lump */
                uint32_t  num_game_lumps = 1;
                source::GameLumpHeader  glh = {
                    .id      = MAGIC_sprp,
                    .flags   = 0,
                    .version = titanfall2::sprp_VERSION,
                    .offset  = static_cast<uint32_t>(write_cursor + 4 + sizeof(source::GameLumpHeader)),
                    .length  = 0x14
                };
                memcpy(outfile.rawdata(write_cursor), &num_game_lumps, sizeof(uint32_t));
                memcpy(outfile.rawdata(write_cursor + sizeof(uint32_t)), &glh, sizeof(source::GameLumpHeader));
                memset(outfile.rawdata(write_cursor + sizeof(uint32_t) + sizeof(source::GameLumpHeader)), 0, glh.length);
                r2lump.length = sizeof(uint32_t) + sizeof(source::GameLumpHeader) + glh.length;
            }
            case titanfall::LIGHTPROBE_REFS: {  // optional?
                auto lprs = r1bsp.get_lump<titanfall::LightProbeRef>(titanfall::LIGHTPROBE_REFS);
                std::vector<titanfall2::LightProbeRef> new_lprs;
                for (auto &lpr : lprs) {
                    new_lprs.push_back({
                        .origin  = lpr.origin,
                        .probe   = lpr.probe,
                        .unknown = 0});
                }
                WRITE_NEW_LUMP(titanfall2::LightProbeRef, new_lprs);
            }
            case titanfall::REAL_TIME_LIGHTS: {  // NULLED OUT
                int texels = r1lump.length / 4;
                r2lump.length = texels * 9;
                WRITE_NULLS(r2lump.length);
            }
            default:  // copy raw lump bytes
                memcpy(outfile.rawdata(write_cursor), r1bsp.file_.rawdata(r1lump.offset), r1lump.length);
        }
        write_cursor += r2lump.length;
    }

    outfile.set_size_and_close(write_cursor);

    return 0;
}
