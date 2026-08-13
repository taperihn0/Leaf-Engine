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

#include <iostream>    
#include <string>
#include <cassert>
#include <type_traits>
#include <array>
#include <string_view>
#include <cstdint>
#include <cstring>
#include <limits>
#include <cstdlib>
#include <random>
#include <algorithm>
#include <cmath>
#include <ctime>
#include <filesystem>
#include <optional>

#if defined(_WIN32)
#include <windows.h>
#endif

#if defined(__GNUC__)
#include <unistd.h>
#endif

#if defined(LEAF_ENABLE_BMI2) and defined(__GNUC__)
// PEXT instructions for GCC
#include <x86gprintrin.h>
#endif

#if defined(_WIN32)
#undef min
#undef max
#undef FAILED
#endif

#if defined(__GNUC__)
#define WARNING(x) message x
#else
#define WARNING(x) message(x)
#endif

#if __cplusplus >= 202002L
#define _CPP_STANDARD_20
#elif __cplusplus >= 201702L
#define _CPP_STANDARD_17
#endif

#ifdef DEBUG
#define _VERIFY_NN
#define _UCI_DEBUG_UTILS
#endif

#if defined(_MSC_VER)
// using __forceinline by default - that came out to be more efficient
#define _INLINE             __forceinline 
#define _FORCEINLINE        __forceinline
#define _LAMBDA_FORCEINLINE [[msvc::forceinline]]
#define _RESTRICT            __restrict
#define _INTERNAL            inline
#else
#define _INLINE             inline
#define _FORCEINLINE        __attribute__((always_inline)) inline // [[gnu::always_inline]] ?
#define _LAMBDA_FORCEINLINE __attribute__((always_inline))        // [[gnu::always_inline]] ?
#define _RESTRICT           __restrict__
#define _INTERNAL           inline
#endif

#define _NORETURN     [[noreturn]]
#define _UNUSED       [[maybe_unused]]
#define _MAYBE_UNUSED [[maybe_unused]]
#define _NODISCARD    [[nodiscard]]
#if defined(_CPP_STANDARD_20)
#define _LIKELY       [[likely]]
#define _UNLIKELY     [[unlikely]]
#else
#define _LIKELY
#define _UNLIKELY
#endif

#if defined(LEAF_BUILD_UTILS) or defined(DEBUG)
#define _ENABLE_TUNING
#endif

/*  _P_CONSTEXPR macro expands to constexpr when _ENABLE_TUNING macro is not defined.
*   On _ENABLE_TUNING defined tuning mode is turned on and _P_CONSTEXPR and _P_STATIC are empty.
*/

#if defined(_ENABLE_TUNING)
#define _P_CONSTEXPR
#define _P_STATIC
#else
#define _P_CONSTEXPR constexpr
#define _P_STATIC    static
#endif

#define _PARAM_ATTRIBS    inline _P_CONSTEXPR
#define _LC_PARAM_ATTRIBS _P_STATIC _P_CONSTEXPR

#if defined(__GNUC__) and defined(LEAF_ARCHITECTURE_X86)
#define _GNU_TARGET_BMI2_AVX2 [[gnu::target("bmi2", "avx2")]]
#else
#define _GNU_TARGET_BMI2_AVX2
#endif

static constexpr size_t CachelineSize = 64;

#if (defined(__GNUC__) and !defined(DEBUG)) or \
    defined(_MSC_VER)                          \
// Loading embedded net do not work for debug builds while compiling with GCC
#define _USE_EMBEDDED_NEURAL_NET
#endif

#define _USE_SYZYGY_TB 1
#define _USE_SYZYGY_TB_ROOT 1

template <typename T>
_INTERNAL void _declUnused(T&&) {}

static constexpr std::string_view EngineName = "Leaf Lite";
static constexpr std::string_view EngineAuthor = "Szymon Belz";

// move format, so far only pure notation supported
#define _PURE_NOTATION_DISPLAY 

#if defined(_MSC_VER)
// Warning: operator '<<' : shift count negative or too big, undefined behavior
#pragma warning(disable: 4293)
// Warning: function uses 'X' bytes of stack. Consider moving some data to heap
#pragma warning(disable: 6262)
// error C4146: unary minus operator applied to unsigned type, result still unsigned
#pragma warning(disable: 4146)
// error C28020: The expression '0<=_Param_(1)&&_Param_(1)<=256-1' is not true at this call.
#pragma warning(disable: 28020)
#endif

using uint = unsigned int;
using byte = uint8_t;
using ll   = long long;
using ull  = unsigned long long;

template <typename T1, typename T2>
constexpr bool is_same = std::is_same_v<T1, T2>;

template <typename T>
constexpr bool is_integral = std::is_integral_v<T>;

template <typename T>
constexpr bool is_real = std::is_floating_point_v<T>;

template <typename T>
constexpr bool is_numeric = (is_real<T> or is_integral<T>);

// keep this macro for compatibility with some blocks of code
#define _IS_SAME_TYPE(t1, t2) (_isSameType<t1, t2>())

inline constexpr uint8_t operator"" _ui8(ull a) noexcept {
    return static_cast<uint8_t>(a);
}

inline constexpr uint16_t operator"" _ui16(ull a) noexcept {
    return static_cast<uint16_t>(a);
}

inline constexpr uint32_t operator"" _ui32(ull a) noexcept {
    return static_cast<uint32_t>(a);
}

inline constexpr uint64_t operator"" _ui64(ull a) noexcept {
    return static_cast<uint64_t>(a);
}

inline constexpr size_t operator""_MB(ull mb_count) {
    return mb_count * 1024 * 1024;
}

#if defined(DEBUG)
#define WARN(msg)                static_cast<void>(::xxassertutil::logWarnMessage(__FILE__, msg, __LINE__))
#else
#define WARN(msg)                static_cast<void>(0);
#endif
#define ASSERT(s, msg)           static_cast<void>((s) or ::xxassertutil::releaseFailedAssertion(__FILE__, msg, __LINE__))
#define ASSERT_NOLOG(s)          ASSERT(s, "Anonymous assertion failed")
#define WARN_IFNOT(s, msg)       static_cast<void>((s) or ::xxassertutil::logWarnMessage(__FILE__, msg, __LINE__))
#define WARN_IFNOT_NOLOG(s, msg) WARN_IFNOT(s, "Anonymous warning point")
#define FAILED(msg)              ASSERT(false, msg)
#define FAILED_NOLOG()           ASSERT_NOLOG(false)

#if defined(_MSC_VER) or defined(__INTEL_COMPILER)
#define DEBUG_BREAK() __debugbreak()
#else
#define DEBUG_BREAK() __builtin_trap()
#endif

namespace xxassertutil {

_INTERNAL bool releaseFailedAssertion(std::string_view file, std::string_view text, int line) {
    std::cout << text << '\n' << file << ", line " << line << std::endl;
    exit(EXIT_FAILURE);
    return false;
}

_INTERNAL bool logWarnMessage(std::string_view file, std::string_view text, int line) {
    std::cout << text << '\n' << file << ", line " << line << std::endl;
    DEBUG_BREAK();
    return false;
}

} // namespace xxassertutil

static constexpr int MaxNodeMoves = 128;
static constexpr int MaxDepth     = 96;
static constexpr int MaxSelDepth  = 128;
static constexpr int MaxGameMoves = 1024;

template <typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
_FORCEINLINE constexpr T sq(T x) {
    return x * x;
}

template <typename T, typename = std::enable_if_t<is_numeric<T>>>
_FORCEINLINE constexpr T abs(T x) {
    return x < 0 ? -x : x;
}

template <typename T, typename = std::enable_if_t<
                        std::is_integral_v<T> and std::is_unsigned_v<T>
                      >
>
_INLINE constexpr bool isExp2(T x) {
    return (x & (x - 1)) == 0;
}

template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
_INLINE constexpr T round(T x) {
    if (x >= 0.l) return static_cast<T>(static_cast<ull>(x + 0.5f));
    return static_cast<T>(static_cast<ll>(x - 0.5f));
}

template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
_INLINE constexpr int roundi(T x) {
    return static_cast<int>(round<T>(x));
}

template <typename T, typename = std::enable_if_t<
                        std::is_integral_v<T> and std::is_unsigned_v<T>
                      >
>
_INLINE constexpr uint8_t getExp2(T x) {
    assert(x != 0);

#if defined(_MSC_VER) or defined(__INTEL_COMPILER)
    unsigned long s;
    _BitScanForward64(&s, x);
    return static_cast<int>(s);
#else
    return __builtin_ctzll(x);
#endif
}

template <typename T, typename = std::enable_if_t<is_numeric<T>>>
_INLINE constexpr T minof() {
    return std::numeric_limits<T>::min();
}

template <typename T, typename = std::enable_if_t<is_numeric<T>>>
_INLINE constexpr T maxof() {
    return std::numeric_limits<T>::max();
}

namespace rnd {

static int GlobFixedSeed = 1;
static std::mt19937 GlobMersenne(GlobFixedSeed);

_INLINE std::mt19937_64& getRandomEngine() {
#if !defined(LEAF_BUILD_UTILS)
    static thread_local uint RandomEngineSeed = GlobFixedSeed;
    static thread_local std::mt19937_64 engine(static_cast<uint64_t>(RandomEngineSeed));
#else
    static thread_local std::mt19937_64 engine([]() { return std::random_device{}(); }());
#endif
    return engine;
}

template <typename T = int, typename = std::enable_if_t<std::is_integral_v<T>>>
_INLINE T random(T l, T r) {
    const uint64_t rand = getRandomEngine()();     
    const uint64_t range = static_cast<uint64_t>(r - l + 1);
    return static_cast<T>(l + static_cast<T>(rand % range));
}

template <typename T = int, typename = std::enable_if_t<std::is_integral_v<T>>>
_INLINE T sparseRandom(T l, T r) {
    return random<T>(l, r) & random<T>(l, r);
}

} // namespace rnd

template <typename T, size_t N>
using array1d = std::array<T, N>;

template <typename T, size_t N, size_t M>
using array2d = array1d<
                    array1d<T, M>, 
                N>;

template <typename T, size_t N, size_t M, size_t S>
using array3d = array1d<
                    array2d<T, M, S>, 
                N>;

template <typename T, size_t N>
_FORCEINLINE T* dataOfArray1d(array1d<T, N>& arr) {
    return reinterpret_cast<T*>(arr.data());
}

template <typename T, size_t N, size_t M>
_FORCEINLINE T* dataOfArray2d(array2d<T, N, M>& arr) {
    return reinterpret_cast<T*>(arr.data());
}

template <typename T, size_t N, size_t M, size_t S>
_FORCEINLINE T* dataOfArray3d(array3d<T, N, M, S>& arr) {
    return reinterpret_cast<T*>(arr.data());
}

template <typename T, size_t N>
_FORCEINLINE const T* dataOfArray1d(const array1d<T, N>& arr) {
    return reinterpret_cast<const T*>(arr.data());
}

template <typename T, size_t N, size_t M>
_FORCEINLINE const T* dataOfArray2d(const array2d<T, N, M>& arr) {
    return reinterpret_cast<const T*>(arr.data());
}

template <typename T, size_t N, size_t M, size_t S>
_FORCEINLINE const T* dataOfArray3d(const array3d<T, N, M, S>& arr) {
    return reinterpret_cast<const T*>(arr.data());
}

template <typename T, size_t N>
_FORCEINLINE constexpr size_t countOfArray1d(const array1d<T, N>&) {
    return N;
}

template <typename T, size_t N, size_t M>
_FORCEINLINE constexpr size_t countOfArray2d(const array2d<T, N, M>&) {
    return N * M;
}

template <typename T, size_t N, size_t M, size_t S>
_FORCEINLINE constexpr size_t countOfArray3d(const array3d<T, N, M, S>&) {
    return N * M * S;
}
