#pragma once

#include <iostream>
#include <string>
#include <intrin.h>
#include <cassert>
#include <type_traits>
#include <array>
#include <string_view>

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

#define ENGINE_NAME "Leaf Lite"
#define AUTHOR "Szymon Belz"

// move format, so far only pure notation supported
#define PURE_NOTATION

#if defined(_MSC_VER)
// Warning: operator '<<' : shift count negative or too big, undefined behavior
#pragma warning(disable: 4293)
// Warning: function uses 'X' bytes of stack. Consider moving some data to heap
#pragma warning(disable: 6262)
#endif

#define ASSERT(s, msg) (void)((s) or releaseFailedAssertion(__FILE__, msg, __LINE__))

inline bool releaseFailedAssertion(std::string_view file, std::string_view text, int line) {
	std::cout << text << '\n' << file << ", line " << line << '\n';
	abort();
	return true;
}

enum enumColor : bool {
	WHITE, BLACK
};

INLINE constexpr enumColor operator!(enumColor opp) {
	return static_cast<enumColor>(!static_cast<bool>(opp));
}

static constexpr int max_node_moves = 256;
static constexpr unsigned max_depth = 256,
						  max_game_moves = 512;

class MoveGenerator;
using MoveGen = MoveGenerator;

enum class File : uint8_t {
	A = 1, B, C, D, E, F, G, H
};

INLINE bool isValidNumber(const std::string& str) {
	return str.find_first_not_of("1234567890", 0) == std::string::npos;
}

INLINE bool isSigned(const std::string& str) {
	return !str.empty() and str[0] == '-';
}