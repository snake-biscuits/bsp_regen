#pragma once

#include <cstddef>
#include <cstdint>


#define MAGIC_sprp  MAGIC('p', 'r', 'p', 's')


namespace source {
    struct GameLumpHeader {
        uint32_t  id;
        uint16_t  flags;
        uint16_t  version;
        uint32_t  offset;
        uint32_t  length;
    };

    static_assert(sizeof(GameLumpHeader) == 0x10);
    static_assert(offsetof(GameLumpHeader, id)      == 0x0);
    static_assert(offsetof(GameLumpHeader, flags)   == 0x4);
    static_assert(offsetof(GameLumpHeader, version) == 0x6);
    static_assert(offsetof(GameLumpHeader, offset)  == 0x8);
    static_assert(offsetof(GameLumpHeader, length)  == 0xC);
}
