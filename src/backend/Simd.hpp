/*
 * Leaf, a UCI Chess Engine
 * Copyright (C) 2026 taperihn0
 *
 * Leaf is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Leaf is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

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
#else 
#define _NN_NO_SIMD
#endif

#ifndef _NN_NO_SIMD
#define _NN_USE_SCRELU_SIMD

#ifdef DEBUG
#define _NN_VERIFY_SCRELU_OVERFLOW
#endif

#endif
#endif

#ifdef LEAF_FORCE_PURE
#define _NN_NO_SIMD
#endif

namespace simd {

#ifdef _NN_USE_AVX512
using float_packed_type = __m512;
using integer_packed_type = __m512i;
#elif defined(_NN_USE_AVX2)
using float_packed_type = __m256;
using integer_packed_type = __m256i;
#elif defined(_NN_USE_SSE2)
using float_packed_type = __m128;
using integer_packed_type = __m128i;
#elif defined(_NN_USE_NEON)
using float_packed_type = float32x4_t;
using integer_packed_type = int32x4_t;
#endif

#ifndef _NN_NO_SIMD

template <typename T, typename = void>
struct PlatformPackedIntrinsics {};

static constexpr size_t getSimdRegisterSizeBits() {
#if defined(_NN_USE_AVX512)
    return 512;
#elif defined(_NN_USE_AVX2)
    return 256;
#elif defined(_NN_USE_SSE2)
    return 128;
#elif defined(_NN_USE_NEON)
    return 128;
#else
    return 0;
#endif
}

static constexpr size_t getSimdRegisterAlignment() {
#if defined(_NN_USE_AVX512)
    return 64;
#elif defined(_NN_USE_AVX2)
    return 32;
#elif defined(_NN_USE_SSE2)
    return 16;
#elif defined(_NN_USE_NEON)
    return 16;
#else
    return 0;
#endif
}

template <typename T, typename = std::enable_if_t<std::is_integral_v<T> and std::is_signed_v<T>>>
struct PlatformPackedDetails {
    static constexpr int Count = getSimdRegisterSizeBits() / (sizeof(T) * 8);
    using platf_packed_type = integer_packed_type;
    using value_type = T;
};

template <typename T>
struct PlatformPackedIntrinsics<T, std::enable_if_t<std::is_integral_v<T> and std::is_signed_v<T>>> {
    using platf_packed_type = PlatformPackedDetails<T>::platf_packed_type;

    static _FORCEINLINE platf_packed_type zero() {
#if defined(_NN_USE_AVX512)
        return _mm512_setzero_si512();
#elif defined(_NN_USE_AVX2)
        return _mm256_setzero_si256();
#elif defined(_NN_USE_SSE2)
        return _mm_setzero_si128();
#elif defined(_NN_USE_NEON)
        return vdupq_n_s32(0);
#else
        static_assert(false);
#endif
    }

    static _FORCEINLINE platf_packed_type fill(T value) {
#if defined(_NN_USE_AVX512)
        if constexpr (std::is_same_v<T, int8_t>)
            return _mm512_set1_epi8(value);
        else if constexpr (std::is_same_v<T, int16_t>)
            return _mm512_set1_epi16(value);
        else if constexpr (std::is_same_v<T, int32_t>)
            return _mm512_set1_epi32(value);
        else if constexpr (std::is_same_v<T, int64_t>)
            return _mm512_set1_epi64x(value);
#elif defined(_NN_USE_AVX2)
        if constexpr (std::is_same_v<T, int8_t>)
            return _mm256_set1_epi8(value);
        else if constexpr (std::is_same_v<T, int16_t>)
            return _mm256_set1_epi16(value);
        else if constexpr (std::is_same_v<T, int32_t>)
            return _mm256_set1_epi32(value);
        else if constexpr (std::is_same_v<T, int64_t>)
            return _mm256_set1_epi64x(value);
#elif defined(_NN_USE_SSE2)
        if constexpr (std::is_same_v<T, int8_t>)
            return _mm_set1_epi8(value);
        else if constexpr (std::is_same_v<T, int16_t>)
            return _mm_set1_epi16(value);
        else if constexpr (std::is_same_v<T, int32_t>)
            return _mm_set1_epi32(value);
        else if constexpr (std::is_same_v<T, int64_t>)
            return _mm_set1_epi64x(value);
#elif defined(_NN_USE_NEON)
        if constexpr (std::is_same_v<T, int8_t>)
            return vreinterpretq_s32_s8(vdupq_n_s8(value));
        else if constexpr (std::is_same_v<T, int16_t>)
            return vreinterpretq_s32_s16(vdupq_n_s16(value));
        else if constexpr (std::is_same_v<T, int32_t>)
            return vdupq_n_s32(static_cast<int32_t>(value));
        else if constexpr (std::is_same_v<T, int64_t>)
            return vreinterpretq_s32_s64(vdupq_n_s64(value));
#else
        static_assert(false);
#endif
        return platf_packed_type{};
    }

    static _FORCEINLINE platf_packed_type load(const platf_packed_type* mem) {
#if defined(_NN_USE_AVX512)
        return _mm512_load_si512(mem);
#elif defined(_NN_USE_AVX2)
        return _mm256_load_si256(mem);
#elif defined(_NN_USE_SSE2)
        return _mm_load_si128(mem);
#elif defined(_NN_USE_NEON)
        return vld1q_s32(reinterpret_cast<const int32_t*>(mem));
#else
        static_assert(false);
#endif
    }

    static _FORCEINLINE platf_packed_type load(const T* mem) {
        return load(reinterpret_cast<const platf_packed_type*>(mem));
    }

    static _FORCEINLINE void store(platf_packed_type* mem, platf_packed_type a) {
#if defined(_NN_USE_AVX512)
        _mm512_store_si512(mem, a);
#elif defined(_NN_USE_AVX2)
        _mm256_store_si256(mem, a);
#elif defined(_NN_USE_SSE2)
        _mm_store_si128(mem, a);
#elif defined(_NN_USE_NEON)
        vst1q_s32(reinterpret_cast<int32_t*>(mem), a);
#else
        static_assert(false);
#endif
    }

    static _FORCEINLINE void store(T* mem, platf_packed_type a) {
        store(reinterpret_cast<platf_packed_type*>(mem), a);
    }

    static _FORCEINLINE platf_packed_type add(platf_packed_type a, platf_packed_type b) {
#if defined(_NN_USE_AVX512)
        if constexpr (std::is_same_v<T, int8_t>)
            return _mm512_add_epi8(a, b);
        else if constexpr (std::is_same_v<T, int16_t>)
            return _mm512_add_epi16(a, b);
        else if constexpr (std::is_same_v<T, int32_t>)
            return _mm512_add_epi32(a, b);
        else if constexpr (std::is_same_v<T, int64_t>)
            return _mm512_add_epi64(a, b);
#elif defined(_NN_USE_AVX2)
        if constexpr (std::is_same_v<T, int8_t>)
            return _mm256_add_epi8(a, b);
        else if constexpr (std::is_same_v<T, int16_t>)
            return _mm256_add_epi16(a, b);
        else if constexpr (std::is_same_v<T, int32_t>)
            return _mm256_add_epi32(a, b);
        else if constexpr (std::is_same_v<T, int64_t>)
            return _mm256_add_epi64(a, b);
#elif defined(_NN_USE_SSE2)
        if constexpr (std::is_same_v<T, int8_t>)
            return _mm_add_epi8(a, b);
        else if constexpr (std::is_same_v<T, int16_t>)
            return _mm_add_epi16(a, b);
        else if constexpr (std::is_same_v<T, int32_t>)
            return _mm_add_epi32(a, b);
        else if constexpr (std::is_same_v<T, int64_t>)
            return _mm_add_epi64(a, b);
#elif defined(_NN_USE_NEON)
        if constexpr (std::is_same_v<T, int8_t>)
            return vreinterpretq_s32_s8(vaddq_s8(vreinterpretq_s8_s32(a), vreinterpretq_s8_s32(b)));
        else if constexpr (std::is_same_v<T, int16_t>)
            return vreinterpretq_s32_s16(vaddq_s16(vreinterpretq_s16_s32(a), vreinterpretq_s16_s32(b)));
        else if constexpr (std::is_same_v<T, int32_t>)
            return vaddq_s32(a, b);
        else if constexpr (std::is_same_v<T, int64_t>)
            return vreinterpretq_s32_s64(vaddq_s64(vreinterpretq_s64_s32(a), vreinterpretq_s64_s32(b)));
#else
        static_assert(false);
#endif
        return platf_packed_type{};
    }

    static _FORCEINLINE platf_packed_type sub(platf_packed_type a, platf_packed_type b) {
#if defined(_NN_USE_AVX512)
        if constexpr (std::is_same_v<T, int8_t>)
            return _mm512_sub_epi8(a, b);
        else if constexpr (std::is_same_v<T, int16_t>)
            return _mm512_sub_epi16(a, b);
        else if constexpr (std::is_same_v<T, int32_t>)
            return _mm512_sub_epi32(a, b);
        else if constexpr (std::is_same_v<T, int64_t>)
            return _mm512_sub_epi64(a, b);
#elif defined(_NN_USE_AVX2)
        if constexpr (std::is_same_v<T, int8_t>)
            return _mm256_sub_epi8(a, b);
        else if constexpr (std::is_same_v<T, int16_t>)
            return _mm256_sub_epi16(a, b);
        else if constexpr (std::is_same_v<T, int32_t>)
            return _mm256_sub_epi32(a, b);
        else if constexpr (std::is_same_v<T, int64_t>)
            return _mm256_sub_epi64(a, b);
#elif defined(_NN_USE_SSE2)
        if constexpr (std::is_same_v<T, int8_t>)
            return _mm_sub_epi8(a, b);
        else if constexpr (std::is_same_v<T, int16_t>)
            return _mm_sub_epi16(a, b);
        else if constexpr (std::is_same_v<T, int32_t>)
            return _mm_sub_epi32(a, b);
        else if constexpr (std::is_same_v<T, int64_t>)
            return _mm_sub_epi64(a, b);
#elif defined(_NN_USE_NEON)
        if constexpr (std::is_same_v<T, int8_t>)
            return vreinterpretq_s32_s8(vsubq_s8(vreinterpretq_s8_s32(a), vreinterpretq_s8_s32(b)));
        else if constexpr (std::is_same_v<T, int16_t>)
            return vreinterpretq_s32_s16(vsubq_s16(vreinterpretq_s16_s32(a), vreinterpretq_s16_s32(b)));
        else if constexpr (std::is_same_v<T, int32_t>)
            return vsubq_s32(a, b);
        else if constexpr (std::is_same_v<T, int64_t>)
            return vreinterpretq_s32_s64(vsubq_s64(vreinterpretq_s64_s32(a), vreinterpretq_s64_s32(b)));
#else
        static_assert(false);
#endif
        return platf_packed_type{};
    }

    static _FORCEINLINE platf_packed_type max(platf_packed_type a, platf_packed_type b) {
#if defined(_NN_USE_AVX512)
        if constexpr (std::is_same_v<T, int8_t>)
            return _mm512_max_epi8(a, b);
        else if constexpr (std::is_same_v<T, int16_t>)
            return _mm512_max_epi16(a, b);
        else if constexpr (std::is_same_v<T, int32_t>)
            return _mm512_max_epi32(a, b);
        else if constexpr (std::is_same_v<T, int64_t>)
            return _mm512_max_epi64(a, b);
#elif defined(_NN_USE_AVX2)
        if constexpr (std::is_same_v<T, int8_t>)
            return _mm256_max_epi8(a, b);
        else if constexpr (std::is_same_v<T, int16_t>)
            return _mm256_max_epi16(a, b);
        else if constexpr (std::is_same_v<T, int32_t>)
            return _mm256_max_epi32(a, b);
        else if constexpr (std::is_same_v<T, int64_t>)
            return _mm256_max_epi64(a, b);
#elif defined(_NN_USE_SSE2)
        if constexpr (std::is_same_v<T, int8_t>)
            return _mm_max_epi8(a, b);
        else if constexpr (std::is_same_v<T, int16_t>)
            return _mm_max_epi16(a, b);
        else if constexpr (std::is_same_v<T, int32_t>)
            return _mm_max_epi32(a, b);
        else if constexpr (std::is_same_v<T, int64_t>)
            return _mm_max_epi64(a, b);
#elif defined(_NN_USE_NEON)
        if constexpr (std::is_same_v<T, int8_t>)
            return vreinterpretq_s32_s8(vmaxq_s8(vreinterpretq_s8_s32(a), vreinterpretq_s8_s32(b)));
        else if constexpr (std::is_same_v<T, int16_t>)
            return vreinterpretq_s32_s16(vmaxq_s16(vreinterpretq_s16_s32(a), vreinterpretq_s16_s32(b)));
        else if constexpr (std::is_same_v<T, int32_t>)
            return vmaxq_s32(a, b);
        else if constexpr (std::is_same_v<T, int64_t>)
            return vreinterpretq_s32_s64(
                vbslq_s64(
                        vcgtq_s64(vreinterpretq_s64_s32(a), vreinterpretq_s64_s32(b)),
                        vreinterpretq_s64_s32(a),
                        vreinterpretq_s64_s32(b)
                    )
            );
#else
        static_assert(false);
#endif
        return platf_packed_type{};
    }

    static _FORCEINLINE platf_packed_type min(platf_packed_type a, platf_packed_type b) {
#if defined(_NN_USE_AVX512)
        if constexpr (std::is_same_v<T, int8_t>)
            return _mm512_min_epi8(a, b);
        else if constexpr (std::is_same_v<T, int16_t>)
            return _mm512_min_epi16(a, b);
        else if constexpr (std::is_same_v<T, int32_t>)
            return _mm512_min_epi32(a, b);
        else if constexpr (std::is_same_v<T, int64_t>)
            return _mm512_min_epi64(a, b);
#elif defined(_NN_USE_AVX2)
        if constexpr (std::is_same_v<T, int8_t>)
            return _mm256_min_epi8(a, b);
        else if constexpr (std::is_same_v<T, int16_t>)
            return _mm256_min_epi16(a, b);
        else if constexpr (std::is_same_v<T, int32_t>)
            return _mm256_min_epi32(a, b);
        else if constexpr (std::is_same_v<T, int64_t>)
            return _mm256_min_epi64(a, b);
#elif defined(_NN_USE_SSE2)
        if constexpr (std::is_same_v<T, int8_t>)
            return _mm_min_epi8(a, b);
        else if constexpr (std::is_same_v<T, int16_t>)
            return _mm_min_epi16(a, b);
        else if constexpr (std::is_same_v<T, int32_t>)
            return _mm_min_epi32(a, b);
        else if constexpr (std::is_same_v<T, int64_t>)
            return _mm_min_epi64(a, b);
#elif defined(_NN_USE_NEON)
        if constexpr (std::is_same_v<T, int8_t>)
            return vreinterpretq_s32_s8(vminq_s8(vreinterpretq_s8_s32(a), vreinterpretq_s8_s32(b)));
        else if constexpr (std::is_same_v<T, int16_t>)
            return vreinterpretq_s32_s16(vminq_s16(vreinterpretq_s16_s32(a), vreinterpretq_s16_s32(b)));
        else if constexpr (std::is_same_v<T, int32_t>)
            return vminq_s32(a, b);
        else if constexpr (std::is_same_v<T, int64_t>)
            return vreinterpretq_s32_s64(vminq_s64(vreinterpretq_s64_s32(a), vreinterpretq_s64_s32(b)));
#else
        static_assert(false);
#endif
        return platf_packed_type{};
    }

    static _FORCEINLINE platf_packed_type mul(platf_packed_type a, platf_packed_type b) {
#if defined(_NN_USE_AVX512)
        if constexpr (std::is_same_v<T, int16_t>)
            return _mm512_mullo_epi16(a, b);
        else if constexpr (std::is_same_v<T, int32_t>)
            return _mm512_mullo_epi32(a, b);
        else if constexpr (std::is_same_v<T, int64_t>)
            return _mm512_mullo_epi64(a, b);
#elif defined(_NN_USE_AVX2)
        if constexpr (std::is_same_v<T, int16_t>)
            return _mm256_mullo_epi16(a, b);
        else if constexpr (std::is_same_v<T, int32_t>)
            return _mm256_mullo_epi32(a, b);
#elif defined(_NN_USE_SSE2)
        if constexpr (std::is_same_v<T, int16_t>)
            return _mm_mullo_epi16(a, b);
        else if constexpr (std::is_same_v<T, int32_t>)
            return _mm_mullo_epi32(a, b);
#elif defined(_NN_USE_NEON)
        if constexpr (std::is_same_v<T, int16_t>)
            return vreinterpretq_s32_s16(vmulq_s16(vreinterpretq_s16_s32(a), vreinterpretq_s16_s32(b)));
        else if constexpr (std::is_same_v<T, int32_t>)
            return vmulq_s32(a, b);
#else
        static_assert(false);
#endif
        return platf_packed_type{};
    }

    static _FORCEINLINE platf_packed_type madd(platf_packed_type a, platf_packed_type b) {
        static_assert(std::is_same_v<T, int16_t>, "Multiply-add is only supported for 16-bit packed values");
#if defined(_NN_USE_AVX512)
        return _mm512_madd_epi16(a, b);
#elif defined(_NN_USE_AVX2)
        return _mm256_madd_epi16(a, b);
#elif defined(_NN_USE_SSE2)
        return _mm_madd_epi16(a, b);
#elif defined(_NN_USE_NEON)
        {
            int16x8_t a_16 = vreinterpretq_s16_s32(a);
            int16x8_t b_16 = vreinterpretq_s16_s32(b);
            int32x4_t mul_lo = vmull_s16(vget_low_s16(a_16), vget_low_s16(b_16));
            int32x4_t mul_hi = vmull_high_s16(a_16, b_16);
            return vpaddq_s32(mul_lo, mul_hi);
        }
#else
        static_assert(false);
#endif
    }

    static _FORCEINLINE int32_t accumulate(platf_packed_type a) {
#if defined(_NN_USE_AVX512)
        return _mm512_reduce_add_epi32(a);
#elif defined(_NN_USE_AVX2)
        const __m128i lo = _mm256_castsi256_si128(a);
        const __m128i hi = _mm256_extracti128_si256(a, 1);
        __m128i s = _mm_add_epi32(lo, hi);
        s = _mm_hadd_epi32(s, s);
        s = _mm_hadd_epi32(s, s);
        return _mm_cvtsi128_si32(s);
#elif defined(_NN_USE_SSE2)
        __m128i s = _mm_hadd_epi32(a, a);
        s = _mm_hadd_epi32(s, s);
        return _mm_cvtsi128_si32(s);
#elif defined(_NN_USE_NEON)
        return vaddvq_s32(a);
#else
        static_assert(false);
#endif
    }

    static _FORCEINLINE platf_packed_type aligned_load(const platf_packed_type* mem) { return load(mem); }
    static _FORCEINLINE platf_packed_type aligned_load(const void* mem) { return load(mem); }
    static _FORCEINLINE platf_packed_type aligned_load(const T* mem) { return load(mem); }

    static _FORCEINLINE void aligned_store(platf_packed_type* mem, platf_packed_type a) { store(mem, a); }
    static _FORCEINLINE void aligned_store(void* mem, platf_packed_type a) { store(mem, a); }
    static _FORCEINLINE void aligned_store(T* mem, platf_packed_type a) { store(mem, a); }
};

template <typename T, typename = std::enable_if_t<std::is_integral_v<T> and std::is_signed_v<T>>>
class Packed {
public:
    using Intr = PlatformPackedIntrinsics<T>;

    Packed() = default;
    explicit Packed(const T* aligned_mem) {
        _m = Intr::aligned_load(mem);
    }

    static Packed zero() {
        return Intr::zero();
    }

    void fill(T v) {
        _m = Intr::fill(v);
    }

    void aligned_load(const T* mem) {
        _m = Intr::aligned_load(mem);
    }

    void aligned_store(T* mem) const {
        Intr::aligned_store(mem, _m);
    }

    _INLINE int32_t accumulate() const {
        return Intr::accumulate(_m);
    }

    _INLINE Packed operator+(const Packed& r) const {
        Packed res;
        res._m = Intr::add(_m, r._m);
        return res;
    }

    _INLINE Packed operator-(const Packed& r) const {
        Packed res;
        res._m = Intr::sub(_m, r._m);
        return res;
    }

    _INLINE Packed operator*(const Packed& r) const {
        Packed res;
        res._m = Intr::mul(_m, r._m);
        return res;
    }

    _INLINE static constexpr size_t count() { 
        return PlatformPackedDetails<T>::Count; 
    }

private:
    using packed_register_type = PlatformPackedDetails<T>::packed_type;
    packed_register_type _m;
};

#else

template <typename T, typename = void>
struct PlatformPackedDetails {
    static constexpr int Count = 0;
    using type = void;
};

template <typename T, typename = void>
struct PlatformPackedIntrinsics {};

#endif

} // namespace simd
