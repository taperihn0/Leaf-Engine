#pragma once

#include "Common.hpp"
#include "Piece.hpp"

class Move {
public:
	INLINE constexpr Move operator=(uint32_t raw) {
		_rmove = raw;
		return *this;
	}

	INLINE Square getOrigin() {
		return _rmove & ORIGIN;
	}

	INLINE Square getTarget() {
		return (_rmove & TARGET) >> 6;
	}

	INLINE bool isCapture() {
		return _rmove & CAPTURE;
	}

	INLINE bool isEnPassant() {
		return _rmove & EP_CAPTURE;
	}

	INLINE bool isShortCastle() {
		return _rmove & SHORT_CASTLE;
	}

	INLINE bool isLongCastle() {
		return _rmove & LONG_CASTLE;
	}

	INLINE Piece::enumType getPerformer() {
		return static_cast<Piece::enumType>((_rmove & PERFORMER) >> 16);
	}

	INLINE Piece::enumType getCaptured() {
		return static_cast<Piece::enumType>((_rmove & CAPTURED) >> 19);
	}

	INLINE Piece::enumType getPromoPiece() {
		return static_cast<Piece::enumType>((_rmove & PROMO_PIECE) >> 22);
	}

private:
	/*
		Raw number data consists of layout above:
		[promo][captured][performer][q-castle][k-castle][ep-capture][capture][target][origin]
		3 bits   3 bits     3 bits     1 bit     1 bit     1 bit      1 bit   6 bits  6 bits
	*/

	enum enumLayout : uint32_t {
		ORIGIN = 0x3f,
		TARGET = 0xfc0,
		CAPTURE = 0x1000,
		EP_CAPTURE = 0x2000,
		SHORT_CASTLE = 0x4000,
		LONG_CASTLE = 0x8000,
		PERFORMER = 0x70000,
		CAPTURED = 0x380000,
		PROMO_PIECE = 0x1c00000
	};

	uint32_t _rmove;
};