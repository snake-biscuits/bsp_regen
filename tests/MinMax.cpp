#include <cstdio>
#include <string>

#include "bounds.hpp"


std::string simd_string(__m128 vector) {
    char buf[1024];

    #define ROT_LEFT(v) \
        v = _mm_shuffle_ps(v, v, _MM_SHUFFLE(1, 2, 3, 0))

    float x = _mm_cvtss_f32(vector);  ROT_LEFT(vector);
    float y = _mm_cvtss_f32(vector);  ROT_LEFT(vector);
    float z = _mm_cvtss_f32(vector);  ROT_LEFT(vector);
    float w = _mm_cvtss_f32(vector);  ROT_LEFT(vector);

    #undef ROT_LEFT

    snprintf(buf, 1024, "\n\tx=%f, \n\ty=%f, \n\tz=%f, \n\tw=%f", x, y, z, w);
    return std::string(buf);
}


int main(int argc, char* argv[]) {
    // NOTE: currently requires a human to read & verify printed output
    MinMax mm;
    __m128 point;

    #define PRINT_VECTOR(v) \
        printf(#v "={%s}\n", simd_string(v).c_str())

    // raw initialised MinMax
    PRINT_VECTOR(mm.min);
    printf("EXPECTED: %f on all axes\n", std::numeric_limits<float>::max());
    PRINT_VECTOR(mm.max);
    printf("EXPECTED: %f on all axes\n", std::numeric_limits<float>::lowest());
    // TODO: verify values in code

    printf("----===----===----===----===----\n");

    // MinMax + 1 point
    point = _mm_set1_ps(256.0f);
    mm.addVector(point);

    printf("mm.addVector(point={%s})\n", simd_string(point).c_str());
    printf("EXPECTED: %f on all axes\n", 256.0f);
    
    printf("----===----===----===----===----\n");
   
    PRINT_VECTOR(mm.min);
    printf("EXPECTED: %f on all axes\n", 256.0f);
    PRINT_VECTOR(mm.max);
    printf("EXPECTED: %f on all axes\n", 256.0f);
    // TODO: verify values in code

    #undef PRINT_VECTOR

    return 0;
}
