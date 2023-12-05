#pragma once

#include <cstdint>
#include <fstream>
#include <cstring>
#include <stdexcept>
#include <vector>

#include "common.hpp"


#define MAGIC_rBSP  MAGIC('r', 'B', 'S', 'P')

// file reading
#define INTO(x)  reinterpret_cast<char*>(&x)
#define GET_LUMP(bsp, T, n, i)  std::vector<T>  n;  bsp.load_lump<T>(i, n)


struct LumpHeader {
    uint32_t offset;
    uint32_t length;
    uint32_t version;
    uint32_t fourCC;
};

static_assert(sizeof(LumpHeader) == 0x10);
static_assert(offsetof(LumpHeader, offset)  == 0x0);
static_assert(offsetof(LumpHeader, length)  == 0x4);
static_assert(offsetof(LumpHeader, version) == 0x8);
static_assert(offsetof(LumpHeader, fourCC)  == 0xC);


struct BspHeader {
    uint32_t   magic;
    uint32_t   version;
    uint32_t   revision;
    uint32_t   _127;
    LumpHeader lumps[128];
};

static_assert(sizeof(BspHeader) == 0x810);
static_assert(offsetof(BspHeader, magic)    == 0x00);
static_assert(offsetof(BspHeader, version)  == 0x04);
static_assert(offsetof(BspHeader, revision) == 0x08);
static_assert(offsetof(BspHeader, _127)     == 0x0C);
static_assert(offsetof(BspHeader, lumps)    == 0x10);


class Bsp { public:
    BspHeader      header;
    std::ifstream  file;

    Bsp(const char* filename) {  // load from file
        this->file = std::ifstream(filename, std::ios::in | std::ios::binary);
        if   (!this->file.fail()) { this->file.read(INTO(this->header), sizeof(BspHeader)); }
        else                      { throw std::runtime_error("Failed to open file");        }
    }

    ~Bsp() {}

    bool is_valid() {
        if (this->header.magic == MAGIC_rBSP) {
            switch (this->header.version) {
                case 29:  // Titanfall
                case 36:  // Titanfall 2 [PS4] (Tech Test)
                case 37:  // Titanfall 2
                    return (this->header._127 == 127);
                default:
                    return false;
            }
        } else {
            return false;
        }
    }

    template <typename T>
    void load_lump(const int lump_index, std::vector<T> &lump_vector) {
        auto header = this->header.lumps[lump_index];
        lump_vector.clear();  lump_vector.resize(header.length / sizeof(T));
        this->file.seekg(header.offset);
        this->file.read(INTO(lump_vector[0]), header.length);
    }

    void load_lump_raw(const int lump_index, char* raw_lump) {
        auto header = this->header.lumps[lump_index];
        this->file.seekg(header.offset);
        this->file.read(raw_lump, header.length);
    }
};
