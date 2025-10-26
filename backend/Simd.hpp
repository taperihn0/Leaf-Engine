#pragma once

/* Instruction sets are iterative, that is
*  they are build on top of each other.
*  I safely assume I can use instructions from older standards.
*/

#ifndef _USE_SIMD
#define LEAF_FORCE_PURE
#endif

#ifdef LEAF_SIMD_AVX512
#define LEAF_SIMD_AVX2
#endif

#ifdef LEAF_SIMD_AVX2
#define LEAF_SIMD_AVX
#endif

#ifdef LEAF_SIMD_AVX
#define LEAF_SIMD_SSE4_2
#endif

#ifdef LEAF_SIMD_SSE4_2
#define LEAF_SIMD_SSE4_1
#endif

#ifdef LEAF_SIMD_SSE4_1
#define LEAF_SIMD_SSSE3
#endif

#ifdef LEAF_SIMD_SSSE3
#define LEAF_SIMD_SSE3
#endif

#ifdef LEAF_SIMD_SSE3
#define LEAF_SIMD_SSE2
#endif

#ifdef LEAF_SIMD_AVX512
#define _AVX512
#endif

#ifdef LEAF_SIMD_AVX2
#define _AVX2
#endif

#ifdef LEAF_SIMD_SSE2
#define _SSE
#endif

#ifndef LEAF_FORCE_PURE

#ifdef LEAF_SIMD_AVX
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

#endif // LEAF_FORCE_PURE
