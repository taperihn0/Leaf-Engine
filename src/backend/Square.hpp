#pragma once

#include "Common.hpp"
#include "Color.hpp"

enum class File : uint8_t {
	A = 0, B, C, D, E, F, G, H
};

class Square {
public:
	using uint_t = uint8_t;

	// little endian rank-file mapping
	enum enumSquare : uint_t {
		a1, b1, c1, d1, e1, f1, g1, h1,
		a2, b2, c2, d2, e2, f2, g2, h2,
		a3, b3, c3, d3, e3, f3, g3, h3,
		a4, b4, c4, d4, e4, f4, g4, h4,
		a5, b5, c5, d5, e5, f5, g5, h5,
		a6, b6, c6, d6, e6, f6, g6, h6,
		a7, b7, c7, d7, e7, f7, g7, h7,
		a8, b8, c8, d8, e8, f8, g8, h8
	};

	enum enumFile : uint_t {
		a = 0, b, c, d, e, f, g, h
	};
	
	enum enumRank : uint_t {
		r1 = 0, r2, r3, r4, r5, r6, r7, r8s
	};

	Square() = default;
	INLINE constexpr Square(uint_t cpy)
		: _sq(cpy) { assert(isValid()); }
	INLINE constexpr Square(enumSquare sq)
		: _sq(sq)  { assert(isValid()); }

	INLINE constexpr Square operator=(uint_t sq) {
		return _sq = sq;
	}

	INLINE constexpr operator uint_t() const {
		return _sq;
	}

	INLINE enumFile getFile() const {
		return static_cast<enumFile>(_sq & 7);
	}

	INLINE enumRank getRank() const {
		return static_cast<enumRank>(_sq / 8);
	}

	INLINE bool isNotNull() const {
		return _sq != None;
	}

	INLINE bool isNull() const {
		return _sq == None;
	}

	static Square fromChar(char file, char rank) {
		return Square((file - 'a') + (rank - '1') * 8);
	}

	INLINE std::string toStr() const {
		ASSERT(isValid(), "Invalid square");
		if (isNull()) return "-";
		return std::string{ "abcdefgh"[_sq & 7], static_cast<char>(_sq / 8 + '1') };
	}

	void print(std::string end = "") const {
		ASSERT(isValid(), "Trying to call print on invalid square");
		std::cout << toStr() << end;
	}

	INLINE constexpr bool isValid() const {
		return _sq < 64 or _sq == None;
	}

	static constexpr uint_t None = -1_ui8;
private:
	uint_t _sq;
};

// flipping square horizontally - a1 becomes a8 and vice versa.
static INLINE Square verticalFlip(Square sq) {
	return sq ^ 56;
}

/* preserving black perspective for whites:
*  when 'side' is BLACK, 'sq' is unchanged.
*  when 'side' is WHITE, 'sq' is flipped vertically.
*/
static INLINE Square blackPerspectiveFlip(Square sq, enumColor side) {
	static std::array<int8_t, 2> ConvertVal = { 56, 0 };
	return sq ^ ConvertVal[side];
}
