#include "Move.hpp"
#include "Position.hpp"

// TODO: move validity restricted checking
// there were some issues with castling moves when parsing 'position <> moves <...castling_move>
Move Move::fromStr(const Position& pos, const std::string str) {
#if defined(PURE_NOTATION)
	ASSERT(str.size() == 4 or str.size() == 5, "Invalid move");

	Square				  origin = Square::fromChar(str[0], str[1]),
						  target = Square::fromChar(str[2], str[3]);
	const Piece::enumType piece = pos.pieceTypeOn(origin, pos.getTurn());
	const bool			  is_capture = pos.getOppositePieces().isOccupiedSq(target),
						  is_ep_capture = target == pos.getEnPassantSq(),
						  promotion = str.size() == 5,
						  short_castle = piece == Piece::KING and origin - target == 2,
						  long_castle = piece == Piece::KING and origin - target == -2;

	ASSERT(piece != Piece::NONE, "Invalid move");
	ASSERT(pos.getOwnPieces().isEmptySq(target), "Invalid move");

	if (promotion)
		return Move::makePromotion(origin, target, is_capture, Piece::typeFromChar(str[4]));
	else if (is_ep_capture)
		return Move::makeEnPassant(origin, target);
	else if (short_castle) {
		ASSERT(pos.getOwnCastling().isShortPossible(), "Castling unavaible in current position");
		return Move::makeCastling<Move::Castle::SHORT>(origin, target);
	}
	else if (long_castle) {
		ASSERT(pos.getOwnCastling().isLongPossible(), "Castling unavaible in current position");
		return Move::makeCastling<Move::Castle::LONG>(origin, target);
	}

	return Move::makeSimple(origin, target, is_capture, piece);
#endif
}

void Move::print() {
#if defined(PURE_NOTATION) 
	if (_rmove == null) {
		std::cout << _null_str;
	}
	else {
		getOrigin().print(), getTarget().print();
		if (isPromotion()) Piece(BLACK, getPromoPieceT()).print();
	}
#endif
}