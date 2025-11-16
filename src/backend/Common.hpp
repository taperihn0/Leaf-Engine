#pragma once

#define _USE_SIMD
#include "Simd.hpp"

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

#if __cplusplus >= 202002L
#define _CPP_STANDARD_20
#elif __cplusplus >= 201702L
#define _CPP_STANDARD_17
#endif

#if defined(_MSC_VER)
// using __forceinline by default - that came out to be more efficient
#define INLINE				__forceinline 
#define _FORCEINLINE		__forceinline
#define _LAMBDA_FORCEINLINE [[msvc::forceinline]]
#else
#define INLINE				inline
#define _FORCEINLINE		__attribute__((always_inline))
#define _LAMBDA_FORCEINLINE __attribute__((always_inline))
#endif

#define _NORETURN [[noreturn]]
#define _UNUSED   [[maybe_unused]]

#if defined(_CPP_STANDARD_20)
#define _UNLIKELY [[unlikely]]
#else
#define _UNLIKELY
#endif

template <typename T>
static void _declUnused(T&&) {}

static constexpr std::string_view EngineName = "Leaf Lite";
static constexpr std::string_view Author     = "Szymon Belz";

// move format, so far only pure notation supported
#define PURE_NOTATION_DISPLAY 

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

using byte = unsigned char;
using ll   = long long;
using ull  = unsigned long long;

template <typename T1, typename T2>
inline constexpr bool _isSameType() {
	return std::is_same_v<T1, T2>;
};

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

#define ASSERT(s, msg) (void)((s) or releaseFailedAssertion(__FILE__, msg, __LINE__))

_NORETURN inline bool releaseFailedAssertion(std::string_view file, std::string_view text, int line) {
	std::cout << text << '\n' << file << ", line " << line << '\n';
	exit(EXIT_FAILURE);
}

static constexpr int	  MaxNodeMoves = 128;
static constexpr unsigned MaxDepth = 128,
						  MaxSelDepth = 128,
						  MaxGameMoves = 512;

template <typename T>
T sq(T x) {
	static_assert(std::is_integral_v<T>);
	return x * x;
}

INLINE bool isValidNumber(const std::string& str) {
	return str.find_first_not_of("1234567890", 0) == std::string::npos;
}

INLINE bool isSigned(const std::string& str) {
	return !str.empty() and str[0] == '-';
}

INLINE bool isValidUnsigned(const std::string& str) {
    return !isSigned(str) and isValidNumber(str);
}

// Target cacheline size is fixed
#define CACHELINE_SIZE 64

INLINE void* alignedMemset(void* dst, int ch, size_t cnt) {
	byte* d = reinterpret_cast<byte*>(dst);

#if defined (_AVX512)
	ASSERT(cnt % 64 == 0, "Size must be a multiple of 64");
	__m512i pack8i_ch = _mm512_set1_epi8(ch);

	for (size_t i = 0; i < cnt; i += 64) {
		_mm512_store_si512(reinterpret_cast<__m512*>(d + i), pack8i_ch);
	}
#elif defined (_AVX2)
	ASSERT(cnt % 32 == 0, "Size must be a multiple of 32");
	__m256i pack4i_ch = _mm256_set1_epi8(ch);

	for (size_t i = 0; i < cnt; i += 32) {
		_mm256_store_si256(reinterpret_cast<__m256i*>(d + i), pack4i_ch);
	}
#elif defined (_SSE)
	ASSERT(cnt % 16 == 0, "Size must be a multiple of 16");
	__m128i pack2i_ch = _mm_set1_epi8(ch);

	for (size_t i = 0; i < cnt; i += 16) {
		_mm_store_si128(reinterpret_cast<__m128i*>(d + i), pack2i_ch);
	}
#else
	_declUnused(d);
	std::memset(dst, ch, cnt);
#endif
	return dst;
}

INLINE void* alignedMalloc(size_t size, size_t alignment) {
#if defined (_MSC_VER)
	void* m = _aligned_malloc(size, alignment);
#else
	void* m = std::aligned_alloc(alignment, size);
#endif
	ASSERT(m, "Failed to allocate memory");
	return m;
}

INLINE void alignedFree(void* block) {
#if defined (_MSC_VER)
	_aligned_free(block);
#else
	std::free(block);
#endif
}

static constexpr int DefaultIntSeed = (1 << 13) + 5;

template <typename Integer = int>
INLINE Integer srandom(Integer l, Integer r, int seed = DefaultIntSeed)
{
    std::mt19937 mt(seed);
    std::uniform_int_distribution<Integer> dist(l, r);
    return dist(mt);
}

template <typename Integer = int>
INLINE Integer random(Integer l, Integer r)
{
    return srandom<Integer>(l, r, std::random_device{}());
}

