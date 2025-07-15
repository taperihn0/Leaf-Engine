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

#if defined(_MSC_VER)
#include <intrin.h>
#endif

#include <immintrin.h>

#define _USE_SIMD

#if defined(_USE_SIMD)
//#define _AVX512
#define _AVX2
//#define _SSE
#endif

#if defined(_MSC_VER)
// using __forceinline by default
#define INLINE __forceinline 
#define _FORCEINLINE __forceinline
#define _LAMBDA_FORCEINLINE [[msvc::forceinline]] 
#else
#define INLINE inline
#define _FORCEINLINE inline
#define _LAMBDA_FORCEINLINE  
#endif

#define _NORETURN [[noreturn]]
#define _UNUSED   [[maybe_unused]]

#define ENGINE_NAME "Leaf Lite"
#define AUTHOR		"Szymon Belz"

// move format, so far only pure notation supported
#define PURE_NOTATION_DISPLAY 

#if defined(_MSC_VER)
// Warning: operator '<<' : shift count negative or too big, undefined behavior
#pragma warning(disable: 4293)
// Warning: function uses 'X' bytes of stack. Consider moving some data to heap
#pragma warning(disable: 6262)
// error C4146: unary minus operator applied to unsigned type, result still unsigned
#pragma warning(disable: 4146)
#endif

using byte = unsigned char;
using ll = long long;
using ull = unsigned long long;

#define _IS_SAME_TYPE(t1, t2) std::is_same_v<t1, t2>

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

#define ASSERT(s, msg) (void)((s) or releaseFailedAssertion(__FILE__, msg, __LINE__))

_NORETURN inline bool releaseFailedAssertion(std::string_view file, std::string_view text, int line) {
	std::cout << text << '\n' << file << ", line " << line << '\n';
	exit(EXIT_FAILURE);
}

static constexpr int max_node_moves = 256;
static constexpr unsigned max_depth = 256,
						  max_game_moves = 512;

class MoveGenerator;
using MoveGen = MoveGenerator;

enum class File : uint8_t {
	A = 0, B, C, D, E, F, G, H
};

INLINE bool isValidNumber(const std::string& str) {
	return str.find_first_not_of("1234567890", 0) == std::string::npos;
}

INLINE bool isSigned(const std::string& str) {
	return !str.empty() and str[0] == '-';
}

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
