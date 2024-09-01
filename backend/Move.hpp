#pragma once

#include "Common.hpp"
#include "Piece.hpp"
#include "Square.hpp"

class Position;

class Move {
public:
	Move() = default;
	INLINE Move(uint32_t raw)
		: _rmove(raw) {}

	INLINE constexpr Move operator=(uint32_t raw) {
		_rmove = raw;
		return *this;
	}
	
	// simplified make function. Leaves other data fields empty, initializing only
	// capture flag, target and origin squares fields.
	static INLINE Move makeSimple(Square origin, Square target, bool isCapture) {
		return Move((isCapture << 12) | (target << 6) | origin);
	}

	static Move fromStr(const Position& pos, const std::string str);

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

	INLINE bool isPromotion() {
		return _rmove & PROMO_PIECE;
	}

	INLINE Piece::enumType getPerformerT() {
		return static_cast<Piece::enumType>((_rmove & PERFORMER) >> 16);
	}

	INLINE Piece::enumType getCapturedT() {
		return static_cast<Piece::enumType>((_rmove & CAPTURED) >> 19);
	}

	INLINE Piece::enumType getPromoPieceT() {
		return static_cast<Piece::enumType>((_rmove & PROMO_PIECE) >> 22);
	}

	INLINE void setOrigin(Square origin) {
		_rmove &= ~ORIGIN, _rmove |= origin;
	}

	INLINE void setTarget(Square target) {
		_rmove &= ~TARGET, _rmove |= (target << 6);
	}

	INLINE void setPromoPiece(Piece piece) {
		_rmove &= ~PROMO_PIECE, _rmove |= (piece << 4);
	}

	void print();

	static constexpr uint32_t null = 0Ui32;
private:
	static constexpr std::string_view _null_str = "0000";

	/*
		Raw number data consists of:
		<----------------------------------------------------------------------------------->
		|							25 bits	layout											|
		<----------------------------------------------------------------------------------->
		[promo] [captured][performer][q-castle][k-castle][ep-capture][capture][target][origin]
		3 bits   3 bits     3 bits     1 bit     1 bit     1 bit      1 bit   6 bits  6 bits
		 MS1B																		   LS1B
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