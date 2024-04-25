#pragma once

#include <cstddef>
#include <immintrin.h>


#define MAGIC(a, b, c, d)  ((a << 0) | (b << 8) | (c << 16) | (d << 24))


struct Vector3 {
    float x;
    float y;
    float z;
};

static_assert(sizeof(Vector3) == 0xC);
static_assert(offsetof(Vector3, x) == 0x0);
static_assert(offsetof(Vector3, y) == 0x4);
static_assert(offsetof(Vector3, z) == 0x8);
