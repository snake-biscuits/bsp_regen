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

    struct Tricoll_Header
    {
        int16_t flags;
        int16_t texInfoFlags;
        int16_t texData;
        int16_t vetCount;
        uint16_t trisCount;
        uint16_t bevelIndicesCount;
        int32_t vertStart;
        uint32_t trisStart;
        uint32_t nodeStart;
        uint32_t bevelIndicesStart;
        Vector3 origin;
        float scale;
    };


    static_assert(sizeof(Tricoll_Header)==0x2C);
    static_assert(offsetof(Tricoll_Header,flags)==0x0);
    static_assert(offsetof(Tricoll_Header,texInfoFlags)==0x2);
    static_assert(offsetof(Tricoll_Header,texData)==0x4);
    static_assert(offsetof(Tricoll_Header,vetCount)==0x6);
    static_assert(offsetof(Tricoll_Header,trisCount)==0x8);
    static_assert(offsetof(Tricoll_Header,bevelIndicesCount)==0xA);
    static_assert(offsetof(Tricoll_Header,vertStart)==0xC);
    static_assert(offsetof(Tricoll_Header,trisStart)==0x10);
    static_assert(offsetof(Tricoll_Header,nodeStart)==0x14);
    static_assert(offsetof(Tricoll_Header,bevelIndicesStart)==0x18);
    static_assert(offsetof(Tricoll_Header,origin)==0x1C);
    static_assert(offsetof(Tricoll_Header,scale)==0x28);


}
