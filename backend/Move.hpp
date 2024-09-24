#pragma once

#include "Common.hpp"
#include "Piece.hpp"
#include "Square.hpp"
#include "Position.hpp"

class Position;
class Move;

class Move {
public:
	enum class Castle;

	Move() = default;

	INLINE Move(uint32_t raw)
		: _rmove(raw) {}

	INLINE bool isNull() const {
		return _rmove == null;
	}

	INLINE constexpr Move operator=(uint32_t raw) {
		_rmove = raw;
		return *this;
	}

	INLINE constexpr bool operator!=(Move b) const noexcept {
		return _rmove != b._rmove;
	}

	INLINE constexpr bool operator==(Move b) const noexcept {
		return _rmove == b._rmove;
	}
	
	// simplified make function. Leaves other data fields empty, initializing only
	// performer piece, capture flag, target and origin squares fields.
	static Move makeSimple(Square origin, Square target, bool is_capture, Piece::enumType piece_t);

	// performer and captured piece in en passant move are de facto known - these are pawns.
	static Move makeEnPassant(Square origin, Square target);

	static Move makePromotion(Square origin, Square target, bool is_capture, Piece::enumType to_piece);

	template <Move::Castle Type>
	static Move makeCastling(Square origin, Square target);

	static Move fromStr(const Position& pos, const std::string& str);

	INLINE Square getOrigin() const {
		return _rmove & ORIGIN;
	}

	INLINE Square getTarget() const {
		return (_rmove & TARGET) >> 6;
	}

	INLINE bool isCapture() const {
		return _rmove & CAPTURE;
	}

	INLINE bool isQuiet() const {
		return !isCapture();
	}

	INLINE bool isEnPassant() const {
		return _rmove & EP_CAPTURE;
	}

	INLINE bool isShortCastle() const {
		return _rmove & SHORT_CASTLE;
	}

	INLINE bool isLongCastle() const {
		return _rmove & LONG_CASTLE;
	}

	INLINE bool isPromotion() const {
		return _rmove & PROMO_PIECE;
	}

	INLINE Piece::enumType getPerformerT() const {
		return static_cast<Piece::enumType>((_rmove & PERFORMER) >> 16);
	}

	INLINE Piece::enumType getCapturedT() const {
		return static_cast<Piece::enumType>((_rmove & CAPTURED) >> 19);
	}

	INLINE Piece::enumType getPromoPieceT() const {
		return static_cast<Piece::enumType>((_rmove & PROMO_PIECE) >> 22);
	}

	INLINE void setOrigin(Square origin) {
		_rmove &= ~ORIGIN, _rmove |= origin;
	}

	INLINE void setTarget(Square target) {
		_rmove &= ~TARGET, _rmove |= static_cast<uint32_t>(target) << 6;
	}

	INLINE void setPromoPiece(Piece piece) {
		_rmove &= ~PROMO_PIECE, _rmove |= static_cast<uint32_t>(piece) << 4;
	}

	INLINE void setPerformerT(Piece::enumType piece) {
		_rmove &= ~PERFORMER, _rmove |= static_cast<uint32_t>(piece) << 16;
	}

	INLINE void setCapturedT(Piece::enumType captured) {
		_rmove &= ~CAPTURED, _rmove |= static_cast<uint32_t>(captured) << 19;
	}

	void print() const;

	bool isPseudoLegal(const Position& pos) const;

	template <bool onlyQuiets>
	bool isPseudoLegal_fromList(const Position& pos) const;

	enum class Castle {
		SHORT, LONG
	};

	static constexpr uint32_t null = 0Ui32;
private:
	static constexpr std::string_view _null_str = "0000";

	/*
		Raw number data consists of:
		 <----------------------------------------------------------------------------------->
		 |							25 bits	layout											|
		 <----------------------------------------------------------------------------------->
		 [promo][captured][performer][q-castle][k-castle][ep-capture][capture][target][origin]
		 3 bits   3 bits     3 bits     1 bit     1 bit     1 bit      1 bit   6 bits  6 bits
		  MS1B							     -->									    LS1B
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

INLINE Move Move::makeSimple(Square origin, Square target, bool is_capture, Piece::enumType piece_t) {
	return Move(
		(static_cast<uint32_t>(piece_t) << 16)
		| (static_cast<uint32_t>(is_capture) << 12)
		| (static_cast<uint32_t>(target) << 6)
		| origin);
}

INLINE Move Move::makeEnPassant(Square origin, Square target) {
	return Move(EP_CAPTURE | CAPTURE | (static_cast<uint32_t>(target) << 6) | origin);
}

INLINE Move Move::makePromotion(Square origin, Square target, bool is_capture, Piece::enumType promo_piece_t) {
	return Move(
		(static_cast<uint32_t>(promo_piece_t) << 22)
		| (static_cast<uint32_t>(is_capture) << 12)
		| (static_cast<uint32_t>(target) << 6)
		| origin);
}

template <Move::Castle Type>
INLINE Move Move::makeCastling(Square origin, Square target) {
	static constexpr uint32_t Field = Type == Castle::SHORT ? SHORT_CASTLE : LONG_CASTLE;
	return Move(
		(static_cast<uint32_t>(Piece::KING) << 16)
		| Field
		| (static_cast<uint32_t>(target) << 6)
		| origin);
}

INLINE bool Move::isPseudoLegal(const Position& pos) const {
	const Square org = getOrigin(), dst = getTarget();
	const Piece::enumType p = getPerformerT(), d = pos.pieceTypeOn(dst, pos.getOppositeTurn());

	if (p == Piece::KING) {
		if (kingAttacks(pos.getKingSquare(pos.getOppositeTurn())) & BitBoard(dst))
			return false;
		else if (isShortCastle()) {
			const CastlingRights own_castling_state = pos.getCastlingByColor(pos.getTurn());
			return own_castling_state.isShortPossible()
				and (own_castling_state.notThroughPieces_Short(pos.getOccupied(), pos.getTurn()))
				and !pos.isInCheck(pos.getTurn())
				and (own_castling_state.notThroughCheck_Short(pos, pos.getTurn()));
		}
		else if (isLongCastle()) {
			const CastlingRights own_castling_state = pos.getCastlingByColor(pos.getTurn());
			return own_castling_state.isLongPossible()
				and (own_castling_state.notThroughPieces_Long(pos.getOccupied(), pos.getTurn()))
				and !pos.isInCheck(pos.getTurn())
				and (own_castling_state.notThroughCheck_Long(pos, pos.getTurn()));
		}
	}

	// TODO: en passant validity
	return p == pos.pieceTypeOn(org, pos.getTurn())
		and (!isCapture() or d != Piece::NONE)
		and (!isQuiet() or (d == Piece::NONE and pos.pieceTypeOn(dst, pos.getTurn()) == Piece::NONE))
		and (p == Piece::KNIGHT or !(inBetween(org, dst) & pos.getOccupied() & ~BitBoard(org) & ~BitBoard(dst)));
}