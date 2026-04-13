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

#if defined(_MSC_VER)
#include <windows.h>
#else
#include <unistd.h>
#endif

#if defined(_MSC_VER)
#undef min
#undef max
#endif

#if defined(__GNUC__)
#	define WARNING(x) message x
#else
#	define WARNING(x) message(x)
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
#define _INLINE				__forceinline 
#define _FORCEINLINE		__forceinline
#define _LAMBDA_FORCEINLINE [[msvc::forceinline]]
#define _RESTRICT			__restrict
#define _INTERNAL			inline
#else
#define _INLINE				inline
#define _FORCEINLINE		__attribute__((always_inline)) inline 
#define _LAMBDA_FORCEINLINE __attribute__((always_inline)) 
#define _RESTRICT 			__restrict__
#define _INTERNAL 			inline
#endif

#define _NORETURN    [[noreturn]]
#define _UNUSED      [[maybe_unused]]

#if defined(BUILD_UTILS) or defined(DEBUG)
#define _ENABLE_TUNING
#endif

#if defined(_CPP_STANDARD_20)
#define _LIKELY   [[likely]]
#define _UNLIKELY [[unlikely]]
#else
#define _LIKELY
#define _UNLIKELY
#endif

#if defined(_ENABLE_TUNING)
#define _P_CONSTEXPR
#define _P_STATIC
#else
#define _P_CONSTEXPR constexpr
#define _P_STATIC    static
#endif

template <typename T>
_INTERNAL void _declUnused(T&&) {}

static constexpr std::string_view EngineName = "Leaf Lite";
static constexpr std::string_view Author     = "Szymon Belz";

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
using byte = unsigned char;
using ll   = long long;
using ull  = unsigned long long;

template <typename T1, typename T2>
inline constexpr bool _isSameType() {
	return std::is_same_v<T1, T2>;
};

template <typename T>
constexpr bool is_integral = std::is_integral_v<T>;

template <typename T>
constexpr bool is_real = std::is_floating_point_v<T>;

template <typename T>
constexpr bool is_numeric = (is_real<T> or is_integral<T>);

// keep this macro for compatibility with some blocks of code
#define _IS_SAME_TYPE(t1, t2) _isSameType<t1, t2>()

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

#define ASSERT(s, msg) (void)((s) or ::releaseFailedAssertion(__FILE__, msg, __LINE__))
#define ASSERTNOLOG(s) ASSERT(s, "Anonymous assertion failed")

_INTERNAL bool releaseFailedAssertion(std::string_view file, std::string_view text, int line) {
	std::cout << text << '\n' << file << ", line " << line << '\n';
	exit(EXIT_FAILURE);
	return false;
}

static constexpr int MaxNodeMoves = 128;
static constexpr int MaxDepth = 96,
					 MaxSelDepth = 128,
					 MaxGameMoves = 512;

template <typename T>
_FORCEINLINE constexpr T sq(T x) {
	static_assert(std::is_integral_v<T>);
	return x * x;
}

template <typename T>
_FORCEINLINE constexpr T abs(T x) {
	static_assert(is_numeric<T>);
	return x < 0 ? -x : x;
}

template <typename T>
_INLINE constexpr bool isPow2(T x) {
	static_assert(std::is_integral_v<T> and std::is_unsigned_v<T>);
	return (x & (x - 1)) == 0;
}

template <typename T>
_INLINE constexpr T round(T x) {
	static_assert(std::is_arithmetic_v<T>);
	
	if (x >= 0.l)
        return static_cast<T>(static_cast<int>(x + 0.5f));
    return static_cast<T>(static_cast<int>(x - 0.5f));
}

template <typename T>
_INLINE constexpr uint8_t get2pow(T x) {
	static_assert(std::is_integral_v<T> and std::is_unsigned_v<T>);
	assert(x != 0);

#if defined(_MSC_VER) or defined(__INTEL_COMPILER)
	unsigned long s;
	_BitScanForward64(&s, x);
	return static_cast<int>(s);
#else
	return __builtin_ctzll(x);
#endif
}

_INLINE bool isValidNumber(const std::string& str) {
	return str.find_first_not_of("1234567890", 0) == std::string::npos;
}

_INLINE bool isSigned(const std::string& str) {
	return !str.empty() and str[0] == '-';
}

_INLINE bool isValidUnsigned(const std::string& str) {
    return !isSigned(str) and isValidNumber(str);
}

// Target cacheline size is fixed
#define CACHELINE_SIZE 64

#if defined(BUILD_UTILS)
static int Seed = []() { return std::random_device{}(); }();
#else
static int Seed = 1;
#endif

static std::mt19937 GlobMersenne(Seed);

template <typename Integer = int>
_INLINE Integer random(Integer l, Integer r) {
	static thread_local std::mt19937 mersenne(Seed);
    std::uniform_int_distribution<Integer> dist(l, r);
    return dist(mersenne);
}

