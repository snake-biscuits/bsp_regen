#include <cmath>
#include <cstdint>
#include <immintrin.h>

#include "titanfall.hpp"


struct MinMax {
    __m128 min, max;

    MinMax() {
        min = _mm_setzero_ps();
        max = _mm_setzero_ps();
    }

    MinMax(const __m128 &start) {
        min = start;
        max = start;
    }

    void addVector(const __m128 &vec) {
        min = _mm_min_ps(min, vec);
        max = _mm_max_ps(max, vec);
    }
};


// NOTE: I know the _mm_shuffle_ps & _mm_cvtss_f32 looks nasty
// -- GCC-only:  __m128_var[0]
// -- MSVC-only: __m128_var.m128_f32[0]
// -- if you have a better cross-platform solution: implement it! please!
bool testCollision(float *cell_mins, float *cell_maxs, __m128 prop_mins, __m128 prop_maxs) {
    float prop_mins_x = _mm_cvtss_f32(prop_mins);
    prop_mins = _mm_shuffle_ps(prop_mins, prop_mins, _MM_SHUFFLE(1, 1, 2, 3));
    float prop_mins_y = _mm_cvtss_f32(prop_mins);
    float prop_maxs_x = _mm_cvtss_f32(prop_maxs);
    prop_maxs = _mm_shuffle_ps(prop_maxs, prop_maxs, _MM_SHUFFLE(1, 1, 2, 3));
    float prop_maxs_y = _mm_cvtss_f32(prop_maxs);
    if (((cell_mins[0] - 1) > prop_maxs_x) || ((cell_maxs[0] + 1) < prop_mins_x)) { return false; }
    if (((cell_mins[1] - 1) > prop_maxs_y) || ((cell_maxs[1] + 1) < prop_mins_y)) { return false; }
    return true;
}


titanfall::Bounds bounds_from_minmax(MinMax &mm) {
    __m128 origin = _mm_div_ps(_mm_add_ps(mm.min, mm.max), _mm_set1_ps(2));
    int16_t origin_x = static_cast<int16_t>(_mm_cvtss_f32(origin));
    origin = _mm_shuffle_ps(origin, origin, _MM_SHUFFLE(1, 1, 2, 3));
    int16_t origin_y = static_cast<int16_t>(_mm_cvtss_f32(origin));
    origin = _mm_shuffle_ps(origin, origin, _MM_SHUFFLE(2, 1, 2, 3));
    int16_t origin_z = static_cast<int16_t>(_mm_cvtss_f32(origin));
    // NOTE: we add 2 to each axis in extents to make sure we cover the full bounds
    __m128 extents = _mm_add_ps(_mm_sub_ps(mm.max, origin), _mm_set1_ps(2));
    int16_t extents_x = static_cast<int16_t>(_mm_cvtss_f32(extents));
    extents = _mm_shuffle_ps(extents, extents, _MM_SHUFFLE(1, 1, 2, 3));
    int16_t extents_y = static_cast<int16_t>(_mm_cvtss_f32(extents));
    extents = _mm_shuffle_ps(extents, extents, _MM_SHUFFLE(2, 1, 2, 3));
    int16_t extents_z = static_cast<int16_t>(_mm_cvtss_f32(extents));
    titanfall::Bounds bounds = {
        .origin = {origin_x, origin_y, origin_z},
        .sin = 0x0000,
        .extents = {extents_x, extents_y, extents_z},
        .cos = 0x0080};
    return bounds;
}


__m128 rotate(const __m128 &vec, Vector3 angles) {
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


MinMax minmax_from_instance_bounds(Vector3 mins, Vector3 maxs, __m128 origin, Vector3 angles, __m128 scale) {
    MinMax mm;
    mm.addVector(_mm_add_ps(origin, rotate(_mm_mul_ps(_mm_set_ps(0, mins.z, mins.y, mins.x), scale), angles)));
    mm.addVector(_mm_add_ps(origin, rotate(_mm_mul_ps(_mm_set_ps(0, mins.z, mins.y, maxs.x), scale), angles)));
    mm.addVector(_mm_add_ps(origin, rotate(_mm_mul_ps(_mm_set_ps(0, mins.z, maxs.y, mins.x), scale), angles)));
    mm.addVector(_mm_add_ps(origin, rotate(_mm_mul_ps(_mm_set_ps(0, mins.z, maxs.y, maxs.x), scale), angles)));
    mm.addVector(_mm_add_ps(origin, rotate(_mm_mul_ps(_mm_set_ps(0, maxs.z, mins.y, mins.x), scale), angles)));
    mm.addVector(_mm_add_ps(origin, rotate(_mm_mul_ps(_mm_set_ps(0, maxs.z, mins.y, maxs.x), scale), angles)));
    mm.addVector(_mm_add_ps(origin, rotate(_mm_mul_ps(_mm_set_ps(0, maxs.z, maxs.y, mins.x), scale), angles)));
    mm.addVector(_mm_add_ps(origin, rotate(_mm_mul_ps(_mm_set_ps(0, maxs.z, maxs.y, maxs.x), scale), angles)));
    return mm;
}
