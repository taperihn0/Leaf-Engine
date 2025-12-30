#pragma once

#include "Common.hpp"

/* Instruction sets are iterative, that is
*  they are build on top of each other.
*/

#ifndef LEAF_FORCE_PURE

#if defined(LEAF_SIMD_AVX512)  \
    || defined(LEAF_SIMD_AVX2) \
    || defined(LEAF_SIMD_AVX) 
#include <immintrin.h>   // AVX, AVX2, AVX-512, BMI, FMA, etc.

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

#elif defined(LEAF_SIMD_SEE)
#include <xmmintrin.h>   // SSE

#endif

#ifdef LEAF_SIMD_AVX512
#define _NN_USE_AVX512

#elif defined(LEAF_SIMD_AVX2)
#define _NN_USE_AVX2

#elif  defined(LEAF_SIMD_SSE4_2) \
    || defined(LEAF_SIMD_SSE4_1) \
    || defined(LEAF_SIMD_SSSE3)  \
    || defined(LEAF_SIMD_SSE3)   \
    || defined(LEAF_SIMD_SSE2) 
#define _NN_USE_SSE2

#elif defined(LEAF_SIMD_SEE)
#define _NN_NO_SIMD

#endif

#endif // LEAF_FORCE_PURE

#if defined(LEAF_SIMD_AVX512)
#define _NN_USE_AVX512

#elif defined(LEAF_SIMD_AVX2)
#define _NN_USE_AVX2

#elif defined(LEAF_SIMD_SSE2)
#define _NN_USE_SSE2

#else
#define _NN_NO_SIMD

#endif

#ifdef _NN_USE_AVX512

static constexpr int MaxRegisterSizeBits = 512;
static constexpr int AlignmentBound = 64;

using _max_platf_register_t = __m512;
using _max_platf_register_i_t = __m512i;

#define _max_register_aligned_load_i(mem_addr)     _mm512_load_si512((mem_addr))
#define _max_register_aligned_store_i(mem_addr, a) _mm512_store_si512((mem_addr), (a))
#define _max_register_add_i8(a, b)                 _mm512_add_epi8((a), (b))
#define _max_register_add_i16(a, b)                _mm512_add_epi16((a), (b))
#define _max_register_add_i32(a, b)                _mm512_add_epi32((a), (b))
#define _max_register_add_i64(a, b)                _mm512_add_epi64((a), (b))
#define _max_register_sub_i8(a, b)                 _mm512_sub_epi8((a), (b))
#define _max_register_sub_i16(a, b)                _mm512_sub_epi16((a), (b))
#define _max_register_sub_i32(a, b)                _mm512_sub_epi32((a), (b))
#define _max_register_sub_i64(a, b)                _mm512_sub_epi64((a), (b))

#elif defined(_NN_USE_AVX2)

static constexpr int MaxRegisterSizeBits = 256;
static constexpr int AlignmentBound = 32;

using _max_platf_register_t = __m256;
using _max_platf_register_i_t = __m256i;

#define _max_register_aligned_load_i(mem_addr)     _mm256_load_si256((mem_addr))
#define _max_register_aligned_store_i(mem_addr, a) _mm256_store_si256((mem_addr), (a))
#define _max_register_add_i8(a, b)                 _mm256_add_epi8((a), (b))
#define _max_register_add_i16(a, b)                _mm256_add_epi16((a), (b))
#define _max_register_add_i32(a, b)                _mm256_add_epi32((a), (b))
#define _max_register_add_i64(a, b)                _mm256_add_epi64((a), (b))
#define _max_register_sub_i8(a, b)                 _mm256_sub_epi8((a), (b))
#define _max_register_sub_i16(a, b)                _mm256_sub_epi16((a), (b))
#define _max_register_sub_i32(a, b)                _mm256_sub_epi32((a), (b))
#define _max_register_sub_i64(a, b)                _mm256_sub_epi64((a), (b))

#elif defined(_NN_USE_SSE2)

static constexpr int MaxRegisterSizeBits = 128;
static constexpr int AlignmentBound = 16;

using _max_platf_register_t = __m128;
using _max_platf_register_i_t = __m128i;

#define _max_register_aligned_load_i(mem_addr)     _mm_load_si128((mem_addr))
#define _max_register_aligned_store_i(mem_addr, a) _mm_store_si128((mem_addr), (a))
#define _max_register_add_i8(a, b)                 _mm_add_epi8((a), (b))
#define _max_register_add_i16(a, b)                _mm_add_epi16((a), (b))
#define _max_register_add_i32(a, b)                _mm_add_epi32((a), (b))
#define _max_register_add_i64(a, b)                _mm_add_epi64((a), (b))
#define _max_register_sub_i8(a, b)                 _mm_sub_epi8((a), (b))
#define _max_register_sub_i16(a, b)                _mm_sub_epi16((a), (b))
#define _max_register_sub_i32(a, b)                _mm_sub_epi32((a), (b))
#define _max_register_sub_i64(a, b)                _mm_sub_epi64((a), (b))

#endif

