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
#include "Color.hpp"

class Square {
public:
	using uint_t = uint8_t;

	// little endian squares mapping
	enum enumSquare : uint_t {
		SQ_A1, SQ_B1, SQ_C1, SQ_D1, SQ_E1, SQ_F1, SQ_G1, SQ_H1,
		SQ_A2, SQ_B2, SQ_C2, SQ_D2, SQ_E2, SQ_F2, SQ_G2, SQ_H2,
		SQ_A3, SQ_B3, SQ_C3, SQ_D3, SQ_E3, SQ_F3, SQ_G3, SQ_H3,
		SQ_A4, SQ_B4, SQ_C4, SQ_D4, SQ_E4, SQ_F4, SQ_G4, SQ_H4,
		SQ_A5, SQ_B5, SQ_C5, SQ_D5, SQ_E5, SQ_F5, SQ_G5, SQ_H5,
		SQ_A6, SQ_B6, SQ_C6, SQ_D6, SQ_E6, SQ_F6, SQ_G6, SQ_H6,
		SQ_A7, SQ_B7, SQ_C7, SQ_D7, SQ_E7, SQ_F7, SQ_G7, SQ_H7,
		SQ_A8, SQ_B8, SQ_C8, SQ_D8, SQ_E8, SQ_F8, SQ_G8, SQ_H8
	};

	enum enumFile : uint_t {
		FILE_A = 0, 
		FILE_B, 
		FILE_C, 
		FILE_D, 
		FILE_E, 
		FILE_F, 
		FILE_G, 
		FILE_H
	};
	
	enum enumRank : uint_t {
		RANK_1 = 0, 
		RANK_2, 
		RANK_3, 
		RANK_4, 
		RANK_5, 
		RANK_6, 
		RANK_7, 
		RANK_8
	};

	Square() = default;
	_INLINE constexpr Square(uint_t sq)
		: _sq(sq) { 
		assert(isValid()); 
	}
	
	_INLINE constexpr Square(enumSquare sq)
		: _sq(sq) { 
		assert(isValid()); 
	}

	_INLINE constexpr Square operator=(uint_t sq) {
		return _sq = sq;
	}

	_INLINE constexpr operator uint_t() const {
		return _sq;
	}

	_NODISCARD _INLINE enumFile getFile() const {
		return static_cast<enumFile>(_sq & 7);
	}

	_NODISCARD _INLINE enumRank getRank() const {
		return static_cast<enumRank>(_sq / 8);
	}

	_NODISCARD _INLINE bool isNull() const {
		return _sq == None;
	}

	_NODISCARD static Square fromChar(char file, char rank) {
		return (file - 'a') + (rank - '1') * 8;
	}

	_NODISCARD _INLINE std::string toStr() const {
		ASSERT(isValid(), "Invalid square");
		if (isNull()) return "-";
		return std::string{ "abcdefgh"[_sq & 7], static_cast<char>(_sq / 8 + '1') };
	}

	void print(std::ostream& os = std::cout) const {
		os << toStr();
	}

	_NODISCARD _INLINE constexpr bool isValid() const {
		return _sq < 64 or _sq == None;
	}

	static constexpr uint_t None = -1_ui8;
private:
	uint_t _sq;
};

// flipping square horizontally - a1 becomes a8 and vice versa.
_NODISCARD static _INLINE Square verticalFlip(Square sq) {
	return sq ^ 56;
}

/* preserving black perspective for whites:
*  when 'side' is BLACK, 'sq' is unchanged.
*  when 'side' is WHITE, 'sq' is flipped vertically.
*/
_NODISCARD static _INLINE Square blackPerspectiveFlip(Square sq, enumColor side) {
	static array1d<int8_t, 2> ConvertVal = { 56, 0 };
	return sq ^ ConvertVal[side];
}
