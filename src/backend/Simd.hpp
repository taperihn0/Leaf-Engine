#pragma once

#include "Common.hpp"

/* Instruction sets are iterative, that is
*  they are build on top of each other.
*/

#ifndef LEAF_FORCE_PURE

#if defined(LEAF_SIMD_AVX512)  \
    or defined(LEAF_SIMD_AVX2) \
    or defined(LEAF_SIMD_AVX) 
#include <immintrin.h>   // AVX, AVX2, AVX-512, BMI, FMA, etc.
#elif defined(LEAF_SIMD_NEON)
#include <arm_neon.h>    // ARM NEON
#elif defined(LEAF_SIMD_SSE4_2)
#include <nmmintrin.h>   // SSE4.2
#elif defined(LEAF_SIMD_SSE4_1)
#include <smmintrin.h>   // SSE4.1
#elif defined(LEAF_SIMD_SSSE3)
#include <tmmintrin.h>   // SSSE3
#elif defined(LEAF_SIMD_SSE3)
#include <pmmintrin.h>   // SSE3
#elif defined(LEAF_SIMD_SSE2)
#include <emmintrin.h>   // SSE2
#elif defined(LEAF_SIMD_SSE)
#include <xmmintrin.h>   // SSE
#endif

#ifdef LEAF_SIMD_AVX512
#define _NN_USE_AVX512
#elif defined(LEAF_SIMD_AVX2)
#define _NN_USE_AVX2
#elif defined(LEAF_SIMD_NEON)
#define _NN_USE_NEON
#elif defined(LEAF_SIMD_SSE4_2)  \
    or defined(LEAF_SIMD_SSE4_1) \
    or defined(LEAF_SIMD_SSSE3)  \
    or defined(LEAF_SIMD_SSE3)   \
    or defined(LEAF_SIMD_SSE2) 
#define _NN_USE_SSE2
#elif defined(LEAF_SIMD_SSE)
#define _NN_NO_SIMD
#endif

#endif // !LEAF_FORCE_PURE

#ifdef _NN_USE_AVX512

// adjust mode for summing accumulator with SCReLU activation function
#define _NN_USE_SCRELU_SIMD

#ifdef DEBUG
#define _NN_VERIFY_SCRELU_OVERFLOW
#endif

static constexpr int MaxRegisterSizeBits = 512;
static constexpr int AlignmentBound = 64;

using _max_platf_register_t = __m512;
using _max_platf_register_i_t = __m512i;

// Loading and storing AVX512 registers
#define _max_register_aligned_load_i(mem_addr)     _mm512_load_si512((mem_addr))
#define _max_register_aligned_store_i(mem_addr, a) _mm512_store_si512((mem_addr), (a))
// Adding AVX512 registers
#define _max_register_add_i8(a, b)                 _mm512_add_epi8((a), (b))
#define _max_register_add_i16(a, b)                _mm512_add_epi16((a), (b))
#define _max_register_add_i32(a, b)                _mm512_add_epi32((a), (b))
#define _max_register_add_i64(a, b)                _mm512_add_epi64((a), (b))
// Subtracting AVX512 registers
#define _max_register_sub_i8(a, b)                 _mm512_sub_epi8((a), (b))
#define _max_register_sub_i16(a, b)                _mm512_sub_epi16((a), (b))
#define _max_register_sub_i32(a, b)                _mm512_sub_epi32((a), (b))
#define _max_register_sub_i64(a, b)                _mm512_sub_epi64((a), (b))
// Filling AVX512 registers
#define _max_register_zero_i                       _mm512_setzero_si512()
#define _max_register_fill_i8(x)                   _mm512_set1_epi8((x))
#define _max_register_fill_i16(x)                  _mm512_set1_epi16((x))
#define _max_register_fill_i32(x)                  _mm512_set1_epi32((x))
#define _max_register_fill_i64(x)                  _mm512_set1_epi64x((x))
// Min and max AVX512 registers
#define _max_register_max_i8(a, b)                 _mm512_max_epi8((a), (b))
#define _max_register_max_i16(a, b)                _mm512_max_epi16((a), (b))
#define _max_register_max_i32(a, b)                _mm512_max_epi32((a), (b))
#define _max_register_max_i64(a, b)                _mm512_max_epi64((a), (b))
#define _max_register_min_i8(a, b)                 _mm512_min_epi8((a), (b))
#define _max_register_min_i16(a, b)                _mm512_min_epi16((a), (b))
#define _max_register_min_i32(a, b)                _mm512_min_epi32((a), (b))
#define _max_register_min_i64(a, b)                _mm512_min_epi64((a), (b))
#define _max_register_mul_i16(a, b)                _mm512_mullo_epi16((a), (b))
// Multiplicating AVX512 registers
#define _max_register_mul_i32(a, b)                _mm512_mullo_epi32((a), (b))
#define _max_register_mul_i64(a, b)                _mm512_mullo_epi64((a), (b))
#define _max_register_madd_i16(a, b)               _mm512_madd_epi16((a), (b))

// Accumulate elements in AVX512 register
static _FORCEINLINE int32_t sumRegisterElements_i16(_max_platf_register_i_t a) {
    return _mm512_reduce_add_epi32(a);
}

#elif defined(_NN_USE_AVX2)

// adjust mode for summing accumulator with SCReLU activation function
#define _NN_USE_SCRELU_SIMD

#ifdef DEBUG
#define _NN_VERIFY_SCRELU_OVERFLOW
#endif

static constexpr int MaxRegisterSizeBits = 256;
static constexpr int AlignmentBound = 32;

using _max_platf_register_t = __m256;
using _max_platf_register_i_t = __m256i;

// Loading and storing AVX2 registers
#define _max_register_aligned_load_i(mem_addr)     _mm256_load_si256((mem_addr))
#define _max_register_aligned_store_i(mem_addr, a) _mm256_store_si256((mem_addr), (a))
// Adding AVX2 registers
#define _max_register_add_i8(a, b)                 _mm256_add_epi8((a), (b))
#define _max_register_add_i16(a, b)                _mm256_add_epi16((a), (b))
#define _max_register_add_i32(a, b)                _mm256_add_epi32((a), (b))
#define _max_register_add_i64(a, b)                _mm256_add_epi64((a), (b))
// Subtracting AVX2 registers
#define _max_register_sub_i8(a, b)                 _mm256_sub_epi8((a), (b))
#define _max_register_sub_i16(a, b)                _mm256_sub_epi16((a), (b))
#define _max_register_sub_i32(a, b)                _mm256_sub_epi32((a), (b))
#define _max_register_sub_i64(a, b)                _mm256_sub_epi64((a), (b))
// Filling AVX2 registers
#define _max_register_zero_i                       _mm256_setzero_si256()
#define _max_register_fill_i8(x)                   _mm256_set1_epi8((x))
#define _max_register_fill_i16(x)                  _mm256_set1_epi16((x))
#define _max_register_fill_i32(x)                  _mm256_set1_epi32((x))
#define _max_register_fill_i64(x)                  _mm256_set1_epi64x((x))
// Max and min AVX2 registers
#define _max_register_max_i8(a, b)                 _mm256_max_epi8((a), (b))
#define _max_register_max_i16(a, b)                _mm256_max_epi16((a), (b))
#define _max_register_max_i32(a, b)                _mm256_max_epi32((a), (b))
#define _max_register_max_i64(a, b)                _mm256_max_epi64((a), (b))
#define _max_register_min_i8(a, b)                 _mm256_min_epi8((a), (b))
#define _max_register_min_i16(a, b)                _mm256_min_epi16((a), (b))
#define _max_register_min_i32(a, b)                _mm256_min_epi32((a), (b))
#define _max_register_min_i64(a, b)                _mm256_min_epi64((a), (b))
// Multiplicating AVX2 registers
#define _max_register_mul_i16(a, b)                _mm256_mullo_epi16((a), (b))
#define _max_register_mul_i32(a, b)                _mm256_mullo_epi32((a), (b))
#define _max_register_madd_i16(a, b)               _mm256_madd_epi16((a), (b))

// Accumulate elements in AVX2 register
static _FORCEINLINE int32_t sumRegisterElements_i16(_max_platf_register_i_t a) {
    const __m128i lo = _mm256_castsi256_si128(a);
    const __m128i hi = _mm256_extracti128_si256(a, 1);

    __m128i s = _mm_add_epi32(lo, hi);
    s = _mm_hadd_epi32(s, s);
    s = _mm_hadd_epi32(s, s);

    return _mm_cvtsi128_si32(s);
}

#elif defined(_NN_USE_SSE2)

// adjust mode for summing accumulator with SCReLU activation function
#define _NN_USE_SCRELU_SIMD

#ifdef DEBUG
#define _NN_VERIFY_SCRELU_OVERFLOW
#endif

static constexpr int MaxRegisterSizeBits = 128;
static constexpr int AlignmentBound = 16;

using _max_platf_register_t = __m128;
using _max_platf_register_i_t = __m128i;

// Loading and storing SSE2 registers
#define _max_register_aligned_load_i(mem_addr)     _mm_load_si128((mem_addr))
#define _max_register_aligned_store_i(mem_addr, a) _mm_store_si128((mem_addr), (a))
// Adding SSE2 registers
#define _max_register_add_i8(a, b)                 _mm_add_epi8((a), (b))
#define _max_register_add_i16(a, b)                _mm_add_epi16((a), (b))
#define _max_register_add_i32(a, b)                _mm_add_epi32((a), (b))
#define _max_register_add_i64(a, b)                _mm_add_epi64((a), (b))
// Subtracting SSE2 registers
#define _max_register_sub_i8(a, b)                 _mm_sub_epi8((a), (b))
#define _max_register_sub_i16(a, b)                _mm_sub_epi16((a), (b))
#define _max_register_sub_i32(a, b)                _mm_sub_epi32((a), (b))
#define _max_register_sub_i64(a, b)                _mm_sub_epi64((a), (b))
// Filling SSE2 registers
#define _max_register_zero_i                       _mm_setzero_si128()
#define _max_register_fill_i8(x)                   _mm_set1_epi8((x))
#define _max_register_fill_i16(x)                  _mm_set1_epi16((x))
#define _max_register_fill_i32(x)                  _mm_set1_epi32((x))
#define _max_register_fill_i64(x)                  _mm_set1_epi64x((x))
// Max and min on SSE2 registers
#define _max_register_max_i8(a, b)                 _mm_max_epi8((a), (b))
#define _max_register_max_i16(a, b)                _mm_max_epi16((a), (b))
#define _max_register_max_i32(a, b)                _mm_max_epi32((a), (b))
#define _max_register_max_i64(a, b)                _mm_max_epi64((a), (b))
#define _max_register_min_i8(a, b)                 _mm_min_epi8((a), (b))
#define _max_register_min_i16(a, b)                _mm_min_epi16((a), (b))
#define _max_register_min_i32(a, b)                _mm_min_epi32((a), (b))
#define _max_register_min_i64(a, b)                _mm_min_epi64((a), (b))
// Multiplicating SSE2 registers
#define _max_register_mul_i16(a, b)                _mm_mullo_epi16((a), (b))
#define _max_register_mul_i32(a, b)                _mm_mullo_epi32((a), (b))
#define _max_register_madd_i16(a, b)               _mm_madd_epi16((a), (b))

// Accumulate elements in SSE2 register
static _FORCEINLINE int32_t sumRegisterElements_i16(_max_platf_register_i_t a) {
    __m128i s = _mm_hadd_epi32(a, a);
    s = _mm_hadd_epi32(s, s);
    return _mm_cvtsi128_si32(s);
}

#elif defined(_NN_USE_NEON)

// adjust mode for summing accumulator with SCReLU activation function
#define _NN_USE_SCRELU_SIMD

#ifdef DEBUG
#define _NN_VERIFY_SCRELU_OVERFLOW
#endif

static constexpr int MaxRegisterSizeBits = 128;
static constexpr int AlignmentBound = 16;

using _max_platf_register_t = float32x4_t;
using _max_platf_register_i_t = int32x4_t; 

// Loading and storing NEON registers
#define _max_register_aligned_load_i(mem_addr)     vld1q_s32(reinterpret_cast<const int32_t*>(mem_addr))
#define _max_register_aligned_store_i(mem_addr, a) vst1q_s32(reinterpret_cast<int32_t*>(mem_addr), (a))
// Adding NEON registers
#define _max_register_add_i8(a, b)                 vaddq_s8(vreinterpretq_s8_s32(a), vreinterpretq_s8_s32(b))
#define _max_register_add_i16(a, b)                vaddq_s16(vreinterpretq_s16_s32(a), vreinterpretq_s16_s32(b))
#define _max_register_add_i32(a, b)                vaddq_s32((a), (b))
#define _max_register_add_i64(a, b)                vaddq_s64(vreinterpretq_s64_s32(a), vreinterpretq_s64_s32(b))
// Subtracting NEON registers
#define _max_register_sub_i8(a, b)                 vsubq_s8(vreinterpretq_s8_s32(a), vreinterpretq_s8_s32(b))
#define _max_register_sub_i16(a, b)                vsubq_s16(vreinterpretq_s16_s32(a), vreinterpretq_s16_s32(b))
#define _max_register_sub_i32(a, b)                vsubq_s32((a), (b))
#define _max_register_sub_i64(a, b)                vsubq_s64(vreinterpretq_s64_s32(a), vreinterpretq_s64_s32(b))
// Filling NEON registers
#define _max_register_zero_i                       vdupq_n_s32(0)
#define _max_register_fill_i8(x)                   vreinterpretq_s32_s8(vdupq_n_s8(x))
#define _max_register_fill_i16(x)                  vreinterpretq_s32_s16(vdupq_n_s16(x))
#define _max_register_fill_i32(x)                  vdupq_n_s32(x)
#define _max_register_fill_i64(x)                  vreinterpretq_s32_s64(vdupq_n_s64(x))
// Min and max on NEON registers
#define _max_register_max_i8(a, b)                 vreinterpretq_s32_s8(vmaxq_s8(vreinterpretq_s8_s32(a), vreinterpretq_s8_s32(b)))
#define _max_register_max_i16(a, b)                vreinterpretq_s32_s16(vmaxq_s16(vreinterpretq_s16_s32(a), vreinterpretq_s16_s32(b)))
#define _max_register_max_i32(a, b)                vmaxq_s32((a), (b))
#define _max_register_max_i64(a, b)                vreinterpretq_s32_s64(vbslq_s64(vcgtq_s64(vreinterpretq_s64_s32(a), vreinterpretq_s64_s32(b)), vreinterpretq_s64_s32(a), vreinterpretq_s64_s32(b)))
#define _max_register_min_i8(a, b)                 vreinterpretq_s32_s8(vminq_s8(vreinterpretq_s8_s32(a), vreinterpretq_s8_s32(b)))
#define _max_register_min_i16(a, b)                vreinterpretq_s32_s16(vminq_s16(vreinterpretq_s16_s32(a), vreinterpretq_s16_s32(b)))
#define _max_register_min_i32(a, b)                vminq_s32((a), (b))
#define _max_register_min_i64(a, b)                vreinterpretq_s64_s32(vminq_s64(vreinterpretq_s64_s32(a), vreinterpretq_s64_s32(b)))
// Multiplicating NEON registers
#define _max_register_mul_i16(a, b)                vreinterpretq_s32_s16(vmulq_s16(vreinterpretq_s16_s32(a), vreinterpretq_s16_s32(b)))
#define _max_register_mul_i32(a, b)                vmulq_s32((a), (b))

// TODO: Use generic hardware instruction
static _FORCEINLINE int32x4_t _max_register_madd_i16_neon(int32x4_t a, int32x4_t b) {
    int16x8_t a_16 = vreinterpretq_s16_s32(a);
    int16x8_t b_16 = vreinterpretq_s16_s32(b);
    
    int32x4_t mul_lo = vmull_s16(vget_low_s16(a_16), vget_low_s16(b_16));
    int32x4_t mul_hi = vmull_high_s16(a_16, b_16);
    
    int32x4_t p1 = vpaddq_s32(mul_lo, mul_hi);
    return p1;
}

#define _max_register_madd_i16(a, b)               _max_register_madd_i16_neon((a), (b))

// Accumulate elements in NEON register
static _FORCEINLINE int32_t sumRegisterElements_i16(_max_platf_register_i_t a) {
    return vaddvq_s32(a);
}

#endif
