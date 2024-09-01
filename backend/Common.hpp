#pragma once

#include <iostream>
#include <string>
#include <intrin.h>
#include <cassert>
#include <type_traits>
#include <array>
#include <string_view>

#define INLINE inline 
#define _FORCEINLINE __forceinline

#define ENGINE_NAME "Leaf Lite"
#define AUTHOR "Szymon Belz"

// move format, so far only long algebraic notation supported
#define LONG_ALGEBRAIC_NOTATION

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

static constexpr uint16_t max_node_moves = 256;

class MoveGenerator;
using MoveGen = MoveGenerator;

enum class File {
	A = 1, B, C, D, E, F, G, H
};