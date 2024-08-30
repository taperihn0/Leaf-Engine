#pragma once

#include "Common.hpp"

class Piece {
public:
	enum enumType;

	Piece() = default;
	Piece(enumType piece_t) { set(WHITE, piece_t); }
	Piece(enumColor col_t, enumType piece_t) { set(col_t, piece_t); }

	Piece fromChar(enumColor col_t, char c) {
		auto id = col_t == WHITE ? _whites_str.find_first_of(c)
			: _blacks_str.find_first_of(c);
		_type = enumType(id);
		_col = col_t;
		return *this;
	}

	void set(enumColor col_t, enumType piece_t) { 
		_col = col_t, _type = piece_t;
	}

	void print() {
		if (_type == NONE) std::cout << ' ';
		else if (_col == WHITE) std::cout << _whites_str[_type];
		else std::cout << _blacks_str[_type];
	}

	int toIndex() {
		return static_cast<int>(_type);
	}

	enum enumType : uint8_t {
		PAWN,
		KNIGHT,
		BISHOP,
		ROOK,
		QUEEN,
		KING,
		NONE
	};

private:
	static constexpr std::string_view _whites_str = "PNBRQK", _blacks_str = "pnbrqk";
	enumType _type;
	enumColor _col;
};