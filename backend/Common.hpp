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

using ull = unsigned long long;

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