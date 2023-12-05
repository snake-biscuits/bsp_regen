#pragma once

#include <cstddef>


#define MAGIC(a, b, c, d)  ((a << 0) | (b << 8) | (c << 16) | (d << 24))


struct Vector {
    float x;
    float y;
    float z;
};

static_assert(sizeof(Vector) == 0xC);
static_assert(offsetof(Vector, x) == 0x0);
static_assert(offsetof(Vector, y) == 0x4);
static_assert(offsetof(Vector, z) == 0x8);
