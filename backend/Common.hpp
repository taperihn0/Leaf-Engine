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

#define ASSERT(s, error) (void)((s) or releaseFailedAssertion(__FILE__, error, __LINE__))

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

class Square {
public:
	Square() = default;
	constexpr Square(uint8_t cpy)
		: _sq(cpy) { assert(isValid()); }

	INLINE constexpr Square operator=(uint8_t sq) {
		return _sq = sq;
	}

	INLINE constexpr operator int() {
		return _sq;
	}

	static Square fromChar(char file, char rank) {
		return Square((file - 'a') + (rank - '1') * 8);
	}

	void print() const {
		std::cout << "abcdefgh"[_sq % 8] << (_sq / 8 + 1);
	}

	constexpr bool isValid() const {
		return 0 <= _sq and _sq < 64 or _sq == none;
	}

	// little endian rank-file mapping
	enum enumSquare {
		a1, b1, c1, d1, e1, f1, g1, h1,
		a2, b2, c2, d2, e2, f2, g2, h2,
		a3, b3, c3, d3, e3, f3, g3, h3,
		a4, b4, c4, d4, e4, f4, g4, h4,
		a5, b5, c5, d5, e5, f5, g5, h5,
		a6, b6, c6, d6, e6, f6, g6, h6,
		a7, b7, c7, d7, e7, f7, g7, h7,
		a8, b8, c8, d8, e8, f8, g8, h8
	};

	static constexpr uint8_t none = -1Ui8;
private:
	uint8_t _sq;
};
