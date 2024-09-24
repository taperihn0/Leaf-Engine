#include "Move.hpp"
#include "MoveGen.hpp"

// TODO: move validity restricted checking
Move Move::fromStr(const Position& pos, const std::string& str) {
#if defined(PURE_NOTATION)
	ASSERT(str.size() == 4 or str.size() == 5, "Invalid move");

	Square				  origin = Square::fromChar(str[0], str[1]),
						  target = Square::fromChar(str[2], str[3]);
	const Piece::enumType piece = pos.pieceTypeOn(origin, pos.getTurn());
	const bool			  is_capture = pos.getOppositePieces().isOccupiedSq(target),
						  is_ep_capture = piece == Piece::PAWN and target == pos.getEnPassantSq(),
						  promotion = str.size() == 5,
						  short_castle = piece == Piece::KING and origin - target == -2,
						  long_castle = piece == Piece::KING and origin - target == 2;

	ASSERT(piece != Piece::NONE, "Invalid move");
	ASSERT(pos.getOwnPieces().isEmptySq(target), "Invalid move");

	Move res;
	
	if (promotion)
		res = Move::makePromotion(origin, target, is_capture, Piece::typeFromChar(str[4]));
	else if (is_ep_capture)
		res =  Move::makeEnPassant(origin, target);
	else if (short_castle) {
		ASSERT(pos.getOwnCastling().isShortPossible(), "Invalid castling move: " + str);
		res = Move::makeCastling<Move::Castle::SHORT>(origin, target);
	}
	else if (long_castle) {
		ASSERT(pos.getOwnCastling().isLongPossible(), "Invalid castling: " + str);
		res = Move::makeCastling<Move::Castle::LONG>(origin, target);
	}
	else
		res = Move::makeSimple(origin, target, is_capture, piece);

	assert(res.isPseudoLegal(pos));
	return res;
#endif
}

void Move::print() const {
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

bool Move::isPseudoLegal(const Position& pos) const {
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
	else if (isEnPassant()) {
		return pos.pieceTypeOn(org, pos.getTurn()) == Piece::PAWN
			and pos.getEnPassantSq() == dst;
	}

	return p == pos.pieceTypeOn(org, pos.getTurn())
		and (!isCapture() or d != Piece::NONE)
		and (!isQuiet() or (d == Piece::NONE and pos.pieceTypeOn(dst, pos.getTurn()) == Piece::NONE))
		and (p == Piece::KNIGHT or !(inBetween(org, dst) & pos.getOccupied() & ~BitBoard(org) & ~BitBoard(dst)));
}

template <bool onlyQuiets>
bool Move::isPseudoLegal_fromList(const Position& pos) const {
	MoveList mlist;
	MoveGen::generatePseudoLegalMoves<onlyQuiets ? MoveGen::QUIETS : MoveGen::ALL>(pos, mlist);
	return mlist.contains(*this);
}

template bool Move::isPseudoLegal_fromList<true>(const Position& pos) const;
template bool Move::isPseudoLegal_fromList<false>(const Position& pos) const;