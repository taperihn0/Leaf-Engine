#include "Move.hpp"
#include "MoveGen.hpp"

/* TODO: move validity restricted checking */

template <>
Move Move::fromStr<Move::Notation::PURE>(const Position& pos, const std::string& str) {
	ASSERT(str.size() == 4 or str.size() == 5, "Invalid move");

	Square				  origin = Square::fromChar(str[0], str[1]),
						  target = Square::fromChar(str[2], str[3]);
	const Piece::enumType piece = pos.pieceOn(origin, pos.getTurn());
	const bool			  capture = pos.getOppositePieces().isOccupiedSq(target),
						  ep_capture = piece == Piece::PAWN and target == pos.getEnPassantSq(),
						  promotion = str.size() == 5,
						  short_castle = piece == Piece::KING and origin - target == -2,
						  long_castle = piece == Piece::KING and origin - target == 2;
	const Piece::enumType promo_piece = promotion ? Piece::typeFromChar(str[4]) : Piece::NONE;

	ASSERT(piece != Piece::NONE, "Invalid move");
	ASSERT(pos.getOwnPieces().isEmptySq(target), "Invalid move");

	Move res = Move::null;

	if (promotion)
		res = Move::makePromotion(origin, target, capture, promo_piece);
	else if (ep_capture)
		res = Move::makeEnPassant(origin, target);
	else if (short_castle) {
		ASSERT(pos.getOwnCastling().isShortPossible(), "Invalid castling move");
		res = Move::makeCastling<Move::Castle::SHORT>(origin, target);
	}
	else if (long_castle) {
		ASSERT(pos.getOwnCastling().isLongPossible(), "Invalid castling move");
		res = Move::makeCastling<Move::Castle::LONG>(origin, target);
	}
	else
		res = Move::makeSimple(origin, target, capture, piece);

	assert(res.isPseudoLegal(pos));
	return res;
}

template <>
Move Move::fromStr<Move::Notation::ALGEBRAIC>(const Position& pos, const std::string& str) {
	Square	   origin = Move::null, 
			   target = Move::null;
	char	   chpromo = '\0';
	const bool capture = str.find('x') != std::string::npos or str.find('X') != std::string::npos,
			   promotion = str.find('=') != std::string::npos,
			   short_castle = str == "O-O" or str == "0-0",
			   long_castle = str == "O-O-O" or str == "0-0-0";

	const size_t last = str.back() == '+' ? str.size() - 2 : str.size() - 1;

	if (short_castle) {
		ASSERT(pos.getOwnCastling().isShortPossible(), "Invalid castling move");
		origin = pos.getTurn() == WHITE ? Square::e1 : Square::e8;
		target = pos.getTurn() == WHITE ? Square::g1 : Square::g8;
	}
	else if (long_castle) {
		ASSERT(pos.getOwnCastling().isLongPossible(), "Invalid castling move");
		origin = pos.getTurn() == WHITE ? Square::e1 : Square::e8;
		target = pos.getTurn() == WHITE ? Square::c1 : Square::c8;
	}
	else if (promotion) {
		ASSERT(last - 3 >= 0, "Invalid move format");
		target = Square::fromChar(str[last - 3], str[last - 2]);

		if (!capture)
			origin = pos.getTurn() == WHITE ? Square(static_cast<int>(target) - 8)
											: Square(static_cast<int>(target) + 8);
		else origin = Square::fromChar(str[0], pos.getTurn() == WHITE ? '7' : '1');

		chpromo = str[last];
	}
	else {
		target = Square::fromChar(str[last - 1], str[last]);

		if (str.size() == 2 and !capture) {
			if (pos.getTurn() == WHITE ? target.getRank() == 3 : target.getRank() == 4) {
				Piece::enumType ddp = pos.pieceOn(
					pos.getTurn() == WHITE ?
						Square(static_cast<int>(target) - 16)
						: Square(static_cast<int>(target) + 16), pos.getTurn());

				Piece::enumType dp = pos.pieceOn(
					pos.getTurn() == WHITE ?
						Square(static_cast<int>(target) - 8)
						: Square(static_cast<int>(target) + 8), pos.getTurn());

				if (ddp == Piece::PAWN and dp == Piece::NONE) {
					origin = pos.getTurn() == WHITE ? Square(static_cast<int>(target) - 16)
													: Square(static_cast<int>(target) + 16);
				}
			}

			if (origin.isNull())
				origin = pos.getTurn() == WHITE ? Square(static_cast<int>(target) - 8)
												: Square(static_cast<int>(target) + 8);
		}
		else if (std::islower(str[0]) and capture)
			origin = Square::fromChar(str[0], pos.getTurn() == WHITE ? str[3] - 1 : str[3] + 1);
		else {
			Piece::enumType piece = Piece::typeFromChar(str[0]);
			BitBoard bb = attacks(piece, target, pos.getOccupied()) & pos.get(piece, pos.getTurn());
			ASSERT(bb != BitBoard(0_ui64), "Invalid capture");

			if (bb.popCount() > 1) {
				char id = str[1];
				int idn = 0;

				if ((idn = (int)std::string_view("abcdefgh").find(id)) != std::string::npos) {
					BitBoard file = BitBoard::file(idn);
					bb &= file;
					
					if (bb.popCount() > 1) {
						id = str[2];
						BitBoard rank = BitBoard::rank(static_cast<int>(id - '1'));
						bb &= rank;
						origin = Square(bb.bitScanForward());
					}
					else origin = Square(bb.bitScanForward());
				}
				else {
					BitBoard rank = BitBoard::rank(static_cast<int>(id - '1'));
					bb &= rank;
					origin = Square(bb.bitScanForward());
				}
			}
			else origin = Square(bb.bitScanForward());
		}
	}

	std::string puremove = origin.toStr() + target.toStr();
	if (promotion) puremove.append(1, chpromo);

	return fromStr<Move::Notation::PURE>(pos, puremove);
}

void Move::print() const {
#if defined(PURE_NOTATION_DISPLAY)
	if (_rmove == null) {
		std::cout << _null_str;
	}
	else {
		getOrigin().print(), getTarget().print();
		if (isPromotion()) Piece(BLACK, getPromoPiece()).print();
	}
#else
	ASSERT(false, "Prining moves in algebraic notation not supported");
#endif
}

bool Move::isPseudoLegal(const Position& pos) const {
	if (*this == Move::null) return false;

	const Square org = getOrigin(), dst = getTarget();
	const Piece::enumType p = getPiece(), d = pos.pieceOn(dst, pos.getOppositeTurn());

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
		return pos.pieceOn(org, pos.getTurn()) == Piece::PAWN
			and pos.getEnPassantSq() == dst;
	}

	return p == pos.pieceOn(org, pos.getTurn())
		and (!isCapture() or d != Piece::NONE)
		and (!isQuiet() or (d == Piece::NONE and pos.pieceOn(dst, pos.getTurn()) == Piece::NONE))
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