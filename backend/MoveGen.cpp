#include "MoveGen.hpp"
#include "Attacks.hpp"

template <MoveGen::enumMode GenType, bool Capture>
void generatePromotions(Square origin, Square target, MoveList& move_list) {
	if constexpr (GenType == MoveGen::CAPTURES or GenType == MoveGen::TACTICALS)
		move_list.push(Move::makePromotion(origin, target, Capture, Piece::QUEEN));

	if constexpr (GenType == MoveGen::QUIETS or GenType == MoveGen::TACTICALS) {
		move_list.push(Move::makePromotion(origin, target, Capture, Piece::KNIGHT));
		move_list.push(Move::makePromotion(origin, target, Capture, Piece::BISHOP));
		move_list.push(Move::makePromotion(origin, target, Capture, Piece::ROOK));
	}
}

template <MoveGen::enumMode GenType, enumColor Side>
void generatePawnCaptures(const Position& pos, MoveList& move_list) {
	static constexpr int      NortWest = 7, NortEast = 9, SoutWest = -9, SoutEast = -7;
	static constexpr int      WestDiag = Side == WHITE ? NortWest : SoutWest,
							  EastDiag = Side == WHITE ? NortEast : SoutEast;
	static constexpr bool     Captures = true;
	static constexpr BitBoard BackRank = Side == WHITE ? BitBoard::rank<8>() : BitBoard::rank<1>();
	
	const BitBoard pawns = pos.get<Piece::PAWN, Side>();
	BitBoard att;

	// Western captures only
	if constexpr (Side == WHITE)
		att = noWeOne(pawns) & pos.getBlacks();
	else 
		att = soWeOne(pawns) & pos.getWhites();

	BitBoard promoted = att & BackRank;
	att ^= promoted;

	while (promoted) {
		const Square dst = Square(promoted.dropForward());
		generatePromotions<GenType, Captures>(dst - WestDiag, dst, move_list);
	}

	while (att) {
		const Square dst = Square(att.dropForward());
		move_list.push(Move::makeSimple(dst - WestDiag, dst, Captures, Piece::PAWN));
	}

	// Eastern captures left
	if constexpr (Side == WHITE)
		att = noEaOne(pawns) & pos.getBlacks();
	else
		att = soEaOne(pawns) & pos.getWhites();

	promoted = att & BackRank;
	att ^= promoted;

	while (promoted) {
		const Square dst = Square(promoted.dropForward());
		generatePromotions<GenType, Captures>(dst - EastDiag, dst, move_list);
	}

	while (att) {
		const Square dst = Square(att.dropForward());
		move_list.push(Move::makeSimple(dst - EastDiag, dst, Captures, Piece::PAWN));
	}
	
	// En-passant check
	const Square ep_sq = pos.getEnPassantSq();
	if (ep_sq.isNull()) return;

	const BitBoard ep_bb = BitBoard(ep_sq);

	if (pawns.genShift<WestDiag>() & ep_bb)
		move_list.push(Move::makeEnPassant(ep_sq - WestDiag, ep_sq));
	else if (pawns.genShift<EastDiag>() & ep_bb)
		move_list.push(Move::makeEnPassant(ep_sq - EastDiag, ep_sq));
}

template <MoveGen::enumMode GenType, enumColor Side>
void generatePawnPushes(const Position& pos, MoveList& move_list) {
	static constexpr int      Dir = Side == WHITE ? 8 : -8;
	static constexpr bool     Captures = true,
							  nonCaptures = !Captures;
	static constexpr BitBoard BackRank = Side == WHITE ? BitBoard::rank<8>() : BitBoard::rank<1>(),
							  NonPushed = Side == WHITE ? BitBoard::rank<2>() : BitBoard::rank<7>();

	BitBoard pawns = pos.get<Piece::PAWN, Side>();
	pawns = pawns.genShift<Dir>();
	pawns &= ~pos.getOccupied();

	BitBoard promoted = pawns & BackRank;
	pawns ^= promoted;

	while (promoted) {
		const Square dst = Square(promoted.dropForward());
		generatePromotions<GenType, nonCaptures>(dst - Dir, dst, move_list);
	}

	if constexpr (GenType == MoveGen::QUIETS) {
		BitBoard double_pushable = pawns & NonPushed;
		pawns ^= double_pushable;

		while (double_pushable) {
			const Square dst = Square(double_pushable.dropForward());
			move_list.push(Move::makeSimple(dst - 2 * Dir, dst, nonCaptures, Piece::PAWN));
		}

		while (pawns) {
			const Square dst = Square(pawns.dropForward());
			move_list.push(Move::makeSimple(dst - Dir, dst, nonCaptures, Piece::PAWN));
		}
	}
}

template <MoveGen::enumMode GenType, enumColor Side>
inline void generatePawnMoves(const Position& pos, MoveList& move_list) {
	if constexpr (GenType != MoveGen::QUIETS)
		generatePawnCaptures<GenType, Side>(pos, move_list);
	generatePawnPushes<GenType, Side>(pos, move_list);
}

template <enumColor Side, bool isCapture>
inline void generateKingMoves(const Position& pos, MoveList& move_list, BitBoard mask) {
	const Square org = pos.getKingBySide(Side).bitScanForward();
	BitBoard att = kingAttacks(org) & mask;

	while (att) {
		const Square dst = att.dropForward();
		move_list.push(Move::makeSimple(org, dst, isCapture, Piece::KING));
	}

	// handle castling 
	static constexpr Square ShortCastleDst = Side == WHITE ? Square::g1 : Square::g8,
							LongCastleDst = Side == WHITE ? Square::c1 : Square::c8;

	if (pos.isCheck()) return;

	const CastlingRights own_castling_state = pos.getCastlingByColor(Side);

	if (own_castling_state.isShortPossible() and
		own_castling_state.notThroughPieces_Short<Side>(pos) and
		own_castling_state.notThroughCheck_Short<Side>(pos))
		move_list.push(Move::makeCastling<Move::Castle::SHORT>(org, ShortCastleDst));

	if (own_castling_state.isLongPossible() and
		own_castling_state.notThroughPieces_Long<Side>(pos) and
		own_castling_state.notThroughCheck_Long<Side>(pos))
		move_list.push(Move::makeCastling<Move::Castle::LONG>(org, LongCastleDst));
}

template <Piece::enumType Piece, enumColor Side, bool isCapture> 
void generate(const Position& pos, MoveList& move_list, BitBoard mask) {
	static_assert(Piece != Piece::PAWN and Piece != Piece::NONE and Piece != Piece::KING, 
		"Unsupported piecetype in generate func template");

	BitBoard pieces = pos.get<Piece, Side>();

	while (pieces) {
		const Square org = Square(pieces.dropForward());
		BitBoard att = attacks<Piece>(org, pos.getOccupied()) & mask;

		while (att) {
			const Square dst = Square(att.dropForward());
			move_list.push(Move::makeSimple(org, dst, isCapture, Piece));
		}
	}
}

template <MoveGen::enumMode GenType, enumColor Side>
void generateByColor(const Position& pos, MoveList& move_list) {
	static const BitBoard own_pieces = Side == WHITE ? pos.getWhites() : pos.getBlacks(),
						  enemy_pieces = pos.getOccupied() ^ own_pieces,
						  mask = GenType == MoveGen::QUIETS ? pos.getEmpties() : pos.getOccupied();
	static constexpr bool areCaptures = GenType != MoveGen::QUIETS;

	generatePawnMoves<GenType, Side>(pos, move_list);

	generate<Piece::KNIGHT, Side, areCaptures>(pos, move_list, mask);
	generate<Piece::BISHOP, Side, areCaptures>(pos, move_list, mask);
	generate<Piece::ROOK, Side, areCaptures> (pos, move_list, mask);
	generate<Piece::QUEEN, Side, areCaptures>(pos, move_list, mask);

	generateKingMoves<Side, areCaptures> (pos, move_list, mask);
}

template <MoveGen::enumMode GenType>
void MoveGen::generatePseudoLegalMoves(const Position& pos, MoveList& move_list) {
	static_cast<enumColor>(pos.getTurn()) == WHITE ? 
		generateByColor<GenType, WHITE>(pos, move_list) :
		generateByColor<GenType, BLACK>(pos, move_list);
}

template void MoveGen::generatePseudoLegalMoves<MoveGen::CAPTURES>(const Position&, MoveList&);
template void MoveGen::generatePseudoLegalMoves<MoveGen::TACTICALS>(const Position&, MoveList&);
template void MoveGen::generatePseudoLegalMoves<MoveGen::QUIETS>(const Position&, MoveList&);