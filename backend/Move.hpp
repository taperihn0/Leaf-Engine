#pragma once

#include "Common.hpp"
#include "Piece.hpp"
#include "Square.hpp"

class Position;

template <typename T>
class MoveData {
public:
	static_assert(_IS_SAME_TYPE(T, uint16_t) or _IS_SAME_TYPE(T, uint32_t));

	enum class Castle;
	enum class Notation;

	MoveData() = default;

	INLINE MoveData(T raw)
		: _rmove(raw) {}

	INLINE bool isNull() const {
		return _rmove == Null;
	}

	INLINE constexpr MoveData operator=(T raw) {
		_rmove = raw;
		return *this;
	}

	// do not consider whether move would be legally moved, since it's not a thing to compare
	INLINE constexpr bool operator!=(MoveData b) const noexcept {
		static_assert(_IS_SAME_TYPE(T, uint32_t));
		return (_rmove & (PROMO_PIECE | TARGET | ORIGIN)) != (b._rmove & (PROMO_PIECE | TARGET | ORIGIN));
	}

	INLINE constexpr bool operator==(MoveData b) const noexcept {
		static_assert(_IS_SAME_TYPE(T, uint32_t));
		return (_rmove & (PROMO_PIECE | TARGET | ORIGIN)) == (b._rmove & (PROMO_PIECE | TARGET | ORIGIN));
	}

	// simplified make function. Leaves other data fields empty, initializing only
	// performer piece, capture flag, target and origin squares fields.
	static MoveData makeSimple(Square origin, Square target, bool is_capture, Piece::enumType piece_t);

	// performer and captured piece in en passant move are de facto known - these are pawns.
	static MoveData makeEnPassant(Square origin, Square target);

	static MoveData makePromotion(Square origin, Square target, bool is_capture, Piece::enumType to_piece);

	template <Castle Type>
	static MoveData makeCastling(Square origin, Square target);

	template <MoveData::Notation Notation>
	static MoveData fromStr(const Position& pos, const std::string& str);

	INLINE Square getOrigin() const {
		return _rmove & ORIGIN;
	}

	INLINE Square getTarget() const {
		return (_rmove & TARGET) >> 6;
	}

	INLINE bool isCapture() const {
		static_assert(_IS_SAME_TYPE(T, uint32_t));
		return _rmove & CAPTURE;
	}

	INLINE bool isQuiet() const {
		static_assert(_IS_SAME_TYPE(T, uint32_t));
		return !isCapture();
	}

	INLINE bool isEnPassant() const {
		static_assert(_IS_SAME_TYPE(T, uint32_t));
		return _rmove & EP_CAPTURE;
	}

	INLINE bool isShortCastle() const {
		static_assert(_IS_SAME_TYPE(T, uint32_t));
		return _rmove & SHORT_CASTLE;
	}

	INLINE bool isLongCastle() const {
		static_assert(_IS_SAME_TYPE(T, uint32_t));
		return _rmove & LONG_CASTLE;
	}

	INLINE bool isPromotion() const {
		return _rmove & PROMO_PIECE;
	}

	INLINE bool isQueenPromotion() const {
		return getPromoPiece() == Piece::QUEEN;
	}

	INLINE bool isUnderPromotion() const {
		return !isQueenPromotion();
	}

	// use this field only after making a move -
	// move legality is checked only when attempting to make it
	INLINE bool isLegalMoved() const {
		static_assert(_IS_SAME_TYPE(T, uint32_t));
		return _rmove & LEGALLY_MOVED;
	}

	INLINE bool isIrreversible() const {
		static_assert(_IS_SAME_TYPE(T, uint32_t));
		return isCapture() or getPiece() == Piece::PAWN or 
			isShortCastle() or isLongCastle();
	}

	INLINE Piece::enumType getPiece() const {
		static_assert(_IS_SAME_TYPE(T, uint32_t));
		return static_cast<Piece::enumType>((_rmove & PERFORMER) >> 19);
	}

	// use this field only after making a move -
	// captured piece is saved only in making a move
	INLINE Piece::enumType getCapturedMoved() const {
		static_assert(_IS_SAME_TYPE(T, uint32_t));
		return static_cast<Piece::enumType>((_rmove & CAPTURED) >> 22);
	}

	INLINE Piece::enumType getPromoPiece() const {
		return static_cast<Piece::enumType>((_rmove & PROMO_PIECE) >> 12);
	}

	INLINE void setOrigin(Square origin) {
		_rmove &= ~ORIGIN, _rmove |= origin;
	}

	INLINE void setTarget(Square target) {
		_rmove &= ~TARGET, _rmove |= static_cast<uint32_t>(target) << 6;
	}

	INLINE void setPromoPiece(Piece::enumType piece) {
		_rmove &= ~PROMO_PIECE, _rmove |= static_cast<uint32_t>(piece) << 12;
	}

	INLINE void setPiece(Piece::enumType piece) {
		static_assert(_IS_SAME_TYPE(T, uint32_t));
		_rmove &= ~PERFORMER, _rmove |= static_cast<uint32_t>(piece) << 19;
	}

	INLINE void setCaptured(Piece::enumType captured) {
		static_assert(_IS_SAME_TYPE(T, uint32_t));
		_rmove &= ~CAPTURED, _rmove |= static_cast<uint32_t>(captured) << 22;
	}

	INLINE void setLegalMoved(bool legal) {
		static_assert(_IS_SAME_TYPE(T, uint32_t));
		_rmove &= ~LEGALLY_MOVED, _rmove |= static_cast<uint32_t>(legal) << 25;
	}

	void print() const;

	bool isPseudoLegal(const Position& pos) const;

	template <bool onlyQuiets>
	bool isPseudoLegal_fromList(const Position& pos) const;

	enum class Castle {
		SHORT, LONG
	};

	enum class Notation {
		PURE, ALGEBRAIC
	};

	static constexpr T Null = 0;
private:
	static constexpr std::string_view _NullStr = "0000";

	enum enumLayout : uint32_t {
		ORIGIN = 0x3f,
		TARGET = 0xfc0,
		PROMO_PIECE = 0x7000,
		CAPTURE = 0x8000,
		EP_CAPTURE = 0x10000,
		SHORT_CASTLE = 0x20000,
		LONG_CASTLE = 0x40000,
		PERFORMER = 0x380000,
		CAPTURED = 0x1C00000,
		LEGALLY_MOVED = 0x2000000
	};

	T _rmove;
};

/*
*	Use standart 32 - bit wide move encoding.
*	Raw number data consists of:
*	 <------------------------------------------------------------------------------------------------>
*	 |								26 bits	layout													  |
*	 <------------------------------------------------------------------------------------------------>
*	 [legal-moved][captured][performer][q-castle][k-castle][ep-capture][capture][promo][target][origin]
*	     1 bit      3 bits    3 bits      1 bit     1 bit     1 bit      1 bit   3 bits 6 bits  6 bits
*		  MS1B									-->												LS1B
*/
using Move32b = MoveData<uint32_t>;

/*
*	Packed move is an optimized class storing only key information about the move.
*	It's only 16 bits wide, as opposed to 32 bits width of MoveData object.
*
*	Raw number data consists of:
*	 <--------------------->
*	 |	 16 bits layout    |
*	 <--------------------->
*	 [promo][target][origin]
*	  4 bits 6 bits  6 bits
*	   MS1B    -->    LS1B
*/
using Move16b = MoveData<uint16_t>;

template <typename T>
INLINE MoveData<T> MoveData<T>::makeSimple(Square origin, Square target, bool is_capture, Piece::enumType piece_t) {
	static_assert(_IS_SAME_TYPE(T, uint32_t));
	return MoveData(
		  (static_cast<uint32_t>(piece_t) << 19)
		| (static_cast<uint32_t>(is_capture) << 15)
		| (static_cast<uint32_t>(target) << 6)
		|  static_cast<uint32_t>(origin));
}

template <typename T>
INLINE MoveData<T> MoveData<T>::makeEnPassant(Square origin, Square target) {
	static_assert(_IS_SAME_TYPE(T, uint32_t));
	return MoveData(
		  (static_cast<uint32_t>(Piece::PAWN) << 19)
		| EP_CAPTURE
		| CAPTURE
		| (static_cast<uint32_t>(target) << 6)
		|  static_cast<uint32_t>(origin));
}

template <typename T>
INLINE MoveData<T> MoveData<T>::makePromotion(Square origin, Square target, bool is_capture, Piece::enumType promo_piece_t) {
	static_assert(_IS_SAME_TYPE(T, uint32_t));
	return MoveData(
		  (static_cast<uint32_t>(Piece::PAWN) << 19)
		| (static_cast<uint32_t>(is_capture) << 15)
		| (static_cast<uint32_t>(promo_piece_t) << 12)
		| (static_cast<uint32_t>(target) << 6)
		|  static_cast<uint32_t>(origin));
}

template <typename T>
template <typename MoveData<T>::Castle Type>
INLINE MoveData<T> MoveData<T>::makeCastling(Square origin, Square target) {
	static_assert(_IS_SAME_TYPE(T, uint32_t));
	static constexpr uint32_t Field = Type == Castle::SHORT ? SHORT_CASTLE : LONG_CASTLE;
	return MoveData(
		  (static_cast<uint32_t>(Piece::KING) << 19)
		| Field
		| (static_cast<uint32_t>(target) << 6)
		|  static_cast<uint32_t>(origin));
}

INLINE Move16b packed(Move32b move) {
	return Move16b(
		  (static_cast<uint16_t>(move.getPromoPiece() << 12))
		| (static_cast<uint16_t>(move.getTarget()) << 6)
		|  static_cast<uint16_t>(move.getOrigin()));
}

// used only in testing templates.
// prefered way to print a move is to juse use print() method
INLINE std::ostream& operator<<(std::ostream& out, Move32b b) {
	b.print();
	return out;
}

Move32b unpacked(const Position& pos, Move16b move);
