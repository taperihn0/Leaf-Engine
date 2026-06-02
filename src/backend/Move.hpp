#pragma once

#include "Common.hpp"
#include "Piece.hpp"
#include "Square.hpp"
#include "Attacks.hpp"

class Position;

template <typename T>
class MoveData {
public:
	static_assert(is_same<T, uint16_t> or is_same<T, uint32_t>);

	enum class Castle;
	enum class Notation;

	MoveData() = default;

	_INLINE MoveData(T raw)
		: _rmove(raw) {}

	_NODISCARD _INLINE bool isNull() const {
		return _rmove == Null;
	}

	_INLINE constexpr MoveData operator=(T raw) {
		_rmove = raw;
		return *this;
	}

	// do not consider whether move would be legally moved, since it's not a thing to compare
	_NODISCARD _INLINE constexpr bool operator!=(MoveData b) const noexcept {
		return !(*this == b);
	}

	_NODISCARD _INLINE constexpr bool operator==(MoveData b) const noexcept {
		return (_rmove & (PROMO_PIECE | TARGET | ORIGIN)) == 
			   (b._rmove & (PROMO_PIECE | TARGET | ORIGIN));
	}

	// simplified make function. Leaves other data fields empty, initializing only
	// performer piece, capture flag, target and origin squares fields.
	_NODISCARD static MoveData makeSimple(Square origin, 
							   Square target, 
							   bool is_capture, 
							   Piece::enumType piece_t);

	_NODISCARD static MoveData makePackedSimple(Square origin, Square target);

	// performer and captured piece in en passant move are de facto known - these are pawns.
	_NODISCARD static MoveData makeEnPassant(Square origin, Square target);

	_NODISCARD static MoveData makePromotion(Square origin, 
								  			 Square target, 
								  			 bool is_capture, 
								  			 Piece::enumType promo_t);

	_NODISCARD static MoveData makePackedPromo(Square origin, 
											   Square target, 
											   Piece::enumType promo_t);

	template <Castle Type>
	_NODISCARD static MoveData makeCastling(Square origin, Square target);

	template <MoveData::Notation Notation>
	_NODISCARD static MoveData fromStr(const Position& pos, const std::string& str);

	_NODISCARD bool isPackedCapture(const Position& pos) const;

	_NODISCARD _INLINE Square getOrigin() const {
		return _rmove & ORIGIN;
	}

	_NODISCARD _INLINE Square getTarget() const {
		return (_rmove & TARGET) >> 6;
	}

	_NODISCARD _INLINE bool isCapture() const {
		static_assert(is_same<T, uint32_t>);
		return _rmove & CAPTURE;
	}

	_NODISCARD _INLINE bool isQuiet() const {
		static_assert(is_same<T, uint32_t>);
		return !isCapture();
	}

	_NODISCARD _INLINE bool isEnPassant() const {
		static_assert(is_same<T, uint32_t>);
		return _rmove & EP_CAPTURE;
	}

	_NODISCARD _INLINE bool isShortCastle() const {
		static_assert(is_same<T, uint32_t>);
		return _rmove & SHORT_CASTLE;
	}

	_NODISCARD _INLINE bool isLongCastle() const {
		static_assert(is_same<T, uint32_t>);
		return _rmove & LONG_CASTLE;
	}

	_NODISCARD _INLINE bool isPromotion() const {
		return _rmove & PROMO_PIECE;
	}

	_NODISCARD _INLINE bool isQueenPromotion() const {
		return getPromoPiece() == Piece::QUEEN;
	}

	_NODISCARD _INLINE bool isRookPromotion() const {
		return getPromoPiece() == Piece::ROOK;
	}

	_NODISCARD _INLINE bool isUnderPromotion() const {
		return isPromotion() and !isQueenPromotion();
	}

	// use this field only after making a move -
	// move legality is checked only when attempting to make it
	_NODISCARD _INLINE bool isLegalMoved() const {
		static_assert(is_same<T, uint32_t>);
		return _rmove & LEGALLY_MOVED;
	}

	_NODISCARD _INLINE bool isIrreversible() const {
		static_assert(is_same<T, uint32_t>);
		return isCapture() or 
			   getPiece() == Piece::PAWN or 
			   isShortCastle() or 
			   isLongCastle();
	}

	_NODISCARD _INLINE Piece::enumType getPiece() const {
		static_assert(is_same<T, uint32_t>);
		return static_cast<Piece::enumType>((_rmove & PERFORMER) >> 19);
	}

	_NODISCARD _INLINE bool isKnight() const {
		if constexpr (is_same<T, uint32_t>)
			return getPiece() == Piece::KNIGHT;
		
		// In 16 bit encoding we don't have explicit piece information, 
		// but we can still check if we've got a knight move

		const Square org = getOrigin();
		const Square dst = getTarget();

		return knightAttacks(org).isOccupiedSq(dst);
	}

	_NODISCARD enumColor getPieceColor(const Position& pos) const;

	_NODISCARD Piece::enumType getCaptured(const Position& pos) const;

	// use this field only after making a move -
	// captured piece is saved only in making a move
	_NODISCARD _INLINE Piece::enumType getCapturedMoved() const {
		static_assert(is_same<T, uint32_t>);
		return static_cast<Piece::enumType>((_rmove & CAPTURED) >> 22);
	}

	_NODISCARD _INLINE Piece::enumType getPromoPiece() const {
		return static_cast<Piece::enumType>((_rmove & PROMO_PIECE) >> 12);
	}

	_INLINE void setOrigin(Square origin) {
		_rmove &= ~ORIGIN, _rmove |= origin;
	}

	_INLINE void setTarget(Square target) {
		_rmove &= ~TARGET, _rmove |= static_cast<uint32_t>(target) << 6;
	}

	_INLINE void setPromoPiece(Piece::enumType piece) {
		_rmove &= ~PROMO_PIECE, _rmove |= static_cast<uint32_t>(piece) << 12;
	}

	_INLINE void setPiece(Piece::enumType piece) {
		static_assert(is_same<T, uint32_t>);
		_rmove &= ~PERFORMER, _rmove |= static_cast<uint32_t>(piece) << 19;
	}

	_INLINE void setCaptured(Piece::enumType captured) {
		static_assert(is_same<T, uint32_t>);
		_rmove &= ~CAPTURED, _rmove |= static_cast<uint32_t>(captured) << 22;
	}

	_INLINE void setLegalMoved(bool legal) {
		static_assert(is_same<T, uint32_t>);
		_rmove &= ~LEGALLY_MOVED, _rmove |= static_cast<uint32_t>(legal) << 25;
	}

	void print(std::ostream& os = std::cout) const;

	_NODISCARD bool isPseudoLegal(const Position& pos) const;

	template <bool onlyQuiets>
	_NODISCARD bool isPseudoLegal_fromList(const Position& pos) const;

	// That function explicitly use make/unmake on given a position.
	// That is generally speaking costly, so use it only when really needed.
	_NODISCARD bool isLegal(Position& pos);

	enum class Castle {
		SHORT, LONG
	};

	enum class Notation {
		REGULAR, ALGEBRAIC
	};

	static constexpr T Null = 0;
private:
	enum enumLayout : uint32_t {
		ORIGIN		  = 0x3f,
		TARGET		  = 0xfc0,
		PROMO_PIECE	  = 0x7000,
		CAPTURE		  = 0x8000,
		EP_CAPTURE	  = 0x10000,
		SHORT_CASTLE  = 0x20000,
		LONG_CASTLE	  = 0x40000,
		PERFORMER	  = 0x380000,
		CAPTURED	  = 0x1C00000,
		LEGALLY_MOVED = 0x2000000
	};

	T _rmove;
};

/*
*	Use standard 32 - bit wide move encoding.
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
_INLINE MoveData<T> MoveData<T>::makeSimple(Square origin, 
										    Square target, 
										    bool is_capture, 
										    Piece::enumType piece_t) 
{
	static_assert(is_same<T, uint32_t>);
	return MoveData(
		  (static_cast<uint32_t>(piece_t) << 19)
		| (static_cast<uint32_t>(is_capture) << 15)
		| (static_cast<uint32_t>(target) << 6)
		|  static_cast<uint32_t>(origin));
}

template <typename T>
_INLINE MoveData<T> MoveData<T>::makeEnPassant(Square origin, Square target) {
	static_assert(is_same<T, uint32_t>);
	return MoveData(
		  (static_cast<uint32_t>(Piece::PAWN) << 19)
		| EP_CAPTURE
		| CAPTURE
		| (static_cast<uint32_t>(target) << 6)
		|  static_cast<uint32_t>(origin));
}

template <typename T>
_INLINE MoveData<T> MoveData<T>::makePromotion(Square origin, 
											   Square target, 
											   bool is_capture, 
											   Piece::enumType promo_t) 
{
	static_assert(is_same<T, uint32_t>);
	return MoveData(
		  (static_cast<uint32_t>(Piece::PAWN) << 19)
		| (static_cast<uint32_t>(is_capture) << 15)
		| (static_cast<uint32_t>(promo_t) << 12)
		| (static_cast<uint32_t>(target) << 6)
		|  static_cast<uint32_t>(origin));
}

template <typename T>
template <typename MoveData<T>::Castle Type>
_INLINE MoveData<T> MoveData<T>::makeCastling(Square origin, Square target) {
	static_assert(is_same<T, uint32_t>);
	static constexpr uint32_t Field = Type == Castle::SHORT ? SHORT_CASTLE : LONG_CASTLE;

	return MoveData(
		  (static_cast<uint32_t>(Piece::KING) << 19)
		| Field
		| (static_cast<uint32_t>(target) << 6)
		|  static_cast<uint32_t>(origin));
}

template <typename T>
_INLINE MoveData<T> MoveData<T>::makePackedSimple(Square origin, Square target) {
	static_assert(is_same<T, uint16_t>);
	return MoveData(
		  (static_cast<uint16_t>(target) << 6)
		|  static_cast<uint16_t>(origin));
}

template <typename T>
_INLINE MoveData<T> MoveData<T>::makePackedPromo(Square origin, 
							    				 Square target, 
							    				 Piece::enumType promo_t) {
	static_assert(is_same<T, uint16_t>);
	return MoveData(
		  (static_cast<uint16_t>(promo_t) << 12)
		| (static_cast<uint16_t>(target) << 6)
		|  static_cast<uint16_t>(origin));
}

_INLINE Move16b packedMove(Move32b move) {
	return Move16b(
		  (static_cast<uint16_t>(move.getPromoPiece() << 12))
		| (static_cast<uint16_t>(move.getTarget()) << 6)
		|  static_cast<uint16_t>(move.getOrigin()));
}

// used only in testing templates.
// prefered way to print a move is to juse use print() method
_INLINE std::ostream& operator<<(std::ostream& out, Move32b b) {
	b.print(out);
	return out;
}

_NODISCARD Move32b unpackedMove(const Position& pos, Move16b move);
