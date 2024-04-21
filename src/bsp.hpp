#pragma once

#include <cstdint>
#include <fstream>
#include <cstring>
#include <stdexcept>
#include <vector>
#include <span>

#include "common.hpp"
#include "memory_mapped_file.hpp"


#define MAGIC_rBSP  MAGIC('r', 'B', 'S', 'P')


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
    BspHeader*         header_;
    memory_mapped_file file_;

    Bsp(const char* filename) {  // load from file
        if (!file_.open_existing(filename))
            throw std::runtime_error("Failed to open file");
        header_ = file_.rawdata<BspHeader>();
    }

    ~Bsp() {}

    bool is_valid() {
        if (header_->magic == MAGIC_rBSP) {
            switch (header_->version) {
                case 29:  // Titanfall
                case 36:  // Titanfall 2 [PS4] (Tech Test)
                case 37:  // Titanfall 2
                    return (header_->_127 == 127);
                default:
                    return false;
            }
        } else {
            return false;
        }
    }

    template <typename T>
    void load_lump(const int lump_index, std::vector<T> &lump_vector) {
        auto& lump_header = header_->lumps[lump_index];
        auto* lump_data = file_.rawdata<T>(lump_header.offset);
        lump_vector.assign(lump_data, lump_data[lump_header.length / sizeof(T)]);
    }

    template<size_t N>
    void load_lump_raw(const int lump_index, const char (&raw_lump)[N]) {
        load_lump_raw(lump_index, raw_lump, N);
    }

    void load_lump_raw(const int lump_index, char* raw_lump, size_t raw_lump_size) {
        auto& lump_header = header_->lumps[lump_index];
        memcpy_s(raw_lump, raw_lump_size, file_.rawdata(lump_header.offset), lump_header.length);
    }

    /*std::string_view lump_view(const int lump_index) {
        auto& lump_header = header_->lumps[lump_index];
        return { file_.rawdata(lump_header.offset), lump_header.length };
    }*/

    template <typename T>
    std::span<T> get_lump(const int lump_index) {
        auto& lump_header = header_->lumps[lump_index];
        return { file_.rawdata<T>(lump_header.offset), lump_header.length / sizeof(T) };
    }

    template <typename T>
    T* get_lump_raw(const int lump_index) {
        auto& lump_header = header_->lumps[lump_index];
        return file_.rawdata<T>(lump_header.offset);
    }

    int get_lump_length(const int lump_index) {
        return header_->lumps[lump_index].length;
    
    }
};
