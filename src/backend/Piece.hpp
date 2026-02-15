#pragma once

#include "Common.hpp"
#include "Color.hpp"

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
		NONE,
	};

	Piece() = default;
	explicit Piece(enumType piece_type) 		{ set(WHITE, piece_type); }
	explicit Piece(enumColor col_t, enumType piece_type) { set(col_t, piece_type); }

	static Piece fromChar(enumColor col_t, char c) {
		auto id = col_t == WHITE ? _WhitesStr.find_first_of(c)
							     : _BlacksStr.find_first_of(c);
		return Piece(col_t, enumType(id));
	}

	static enumType typeFromChar(char c) {
		c = tolower(c);
		auto id = _BlacksStr.find_first_of(c);
		return enumType(id);
	}

	void set(enumColor col_t, enumType piece_type) { 
		_col = col_t;
		_type = piece_type;
	}

	void setColor(enumColor col) {
		_col = col;
	}

	void setType(enumType piece_type) {
		_type = piece_type;
	}

	void print(std::ostream& os = std::cout) const {
		if (_type == NONE) os << ' ';
		else if (_col == WHITE) os << _WhitesStr[_type];
		else os << _BlacksStr[_type];
	}

	INLINE Piece::uint_t value() const {
		return static_cast<Piece::uint_t>(_type);
	}

	INLINE enumType type() const {
		return _type;
	}

	INLINE enumColor color() const {
		return _col;
	}

	static constexpr std::array<enumType, 6> PieceTypeList = { 
		Piece::PAWN, 
		Piece::KNIGHT, 
		Piece::BISHOP, 
		Piece::ROOK, 
		Piece::QUEEN, 
		Piece::KING
	};

private:
	static constexpr std::string_view _WhitesStr = "PNBRQK", 
									  _BlacksStr = "pnbrqk";
	enumType _type;
	enumColor _col;
};

INLINE std::ostream& operator<<(std::ostream& os, Piece p) {
	p.print(os);
	return os;
}

INLINE constexpr Piece::uint_t value(Piece::enumType p) {
	return static_cast<Piece::uint_t>(p);
}

INLINE constexpr bool isSlider(Piece::enumType p) {
	return p >= Piece::BISHOP and p <= Piece::QUEEN;
}
