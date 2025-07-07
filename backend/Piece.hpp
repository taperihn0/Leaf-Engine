#pragma once

#include "Common.hpp"
#include "Color.hpp"

#include <vector>

class Piece {
public:
	using uint_t = uint8_t;

	enum enumType : uint_t {
		PAWN,
		KNIGHT,
		BISHOP,
		ROOK,
		QUEEN,
		KING,
		BISHOP_LIKE = BISHOP | QUEEN,
		ROOK_LIKE   =   ROOK | QUEEN,
		NONE,
	};

	Piece() = default;
	inline Piece(enumType piece_t) { set(WHITE, piece_t); }
	inline Piece(enumColor col_t, enumType piece_t) { set(col_t, piece_t); }

	static inline Piece fromChar(enumColor col_t, char c) {
		auto id = col_t == WHITE ? _whites_str.find_first_of(c)
			: _blacks_str.find_first_of(c);
		return Piece(col_t, enumType(id));
	}

	static inline enumType typeFromChar(char c) {
		c = tolower(c);
		auto id = _blacks_str.find_first_of(c);
		return enumType(id);
	}

	inline void set(enumColor col_t, enumType piece_t) { 
		_col = col_t, _type = piece_t;
	}

	void print() const {
		if (_type == NONE) std::cout << ' ';
		else if (_col == WHITE) std::cout << _whites_str[_type];
		else std::cout << _blacks_str[_type];
	}

	inline Piece::uint_t value() const {
		return static_cast<Piece::uint_t>(_type);
	}

	inline enumType type() const {
		return _type;
	}

	inline enumColor color() const {
		return _col;
	}

	static constexpr std::array<enumType, 6> piece_list = { 
		Piece::PAWN, 
		Piece::KNIGHT, 
		Piece::BISHOP, 
		Piece::ROOK, 
		Piece::QUEEN, 
		Piece::KING
	};

private:
	static constexpr std::string_view _whites_str = "PNBRQK", 
									  _blacks_str = "pnbrqk";
	enumType _type;
	enumColor _col;
};

INLINE constexpr Piece::enumType operator|(Piece::enumType t1, Piece::enumType t2) {
	return static_cast<Piece::enumType>(t1 | t2);
}

INLINE constexpr Piece::uint_t value(Piece::enumType p) {
	return static_cast<Piece::uint_t>(p);
}