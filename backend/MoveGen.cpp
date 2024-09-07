#include "MoveGen.hpp"
#include "Attacks.hpp"

template <MoveGen::enumMode GenType, bool Capture>
inline void generatePromotions(Square origin, Square target, MoveList& move_list) {
	if constexpr (GenType == MoveGen::CAPTURES or GenType == MoveGen::TACTICALS)
		move_list.push(Move::makePromotion(origin, target, Capture, Piece::QUEEN));

	if constexpr (Capture or GenType == MoveGen::QUIETS or GenType == MoveGen::TACTICALS) {
		move_list.push(Move::makePromotion(origin, target, Capture, Piece::KNIGHT));
		move_list.push(Move::makePromotion(origin, target, Capture, Piece::BISHOP));
		move_list.push(Move::makePromotion(origin, target, Capture, Piece::ROOK));
	}
}

template <MoveGen::enumMode GenType, enumColor Side>
void generatePawnCaptures(const Position& pos, MoveList& move_list, BitBoard enemies) {
	static constexpr int      NortWest = 7, NortEast = 9, SoutWest = -9, SoutEast = -7;
	static constexpr int      WestDiag = Side == WHITE ? NortWest : SoutWest,
							  EastDiag = Side == WHITE ? NortEast : SoutEast;
	static constexpr bool     Captures = true;
	static constexpr BitBoard BackRank = Side == WHITE ? BitBoard::rank<8>() : BitBoard::rank<1>();
	
	const BitBoard pawns = pos.get<Piece::PAWN, Side>();
	BitBoard att;

	// Western captures only
	if constexpr (Side == WHITE)
		att = noWeOne(pawns) & enemies;
	else 
		att = soWeOne(pawns) & enemies;

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
		att = noEaOne(pawns) & enemies;
	else
		att = soEaOne(pawns) & enemies;

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
	
	// En-passant validation
	const Square ep_sq = pos.getEnPassantSq();

	if (ep_sq.isNull()) return;

	const BitBoard ep_bb = BitBoard(ep_sq);

	if (pawns.pawnsAttack<WestDiag>() & ep_bb)
		move_list.push(Move::makeEnPassant(ep_sq - WestDiag, ep_sq));
	if (pawns.pawnsAttack<EastDiag>() & ep_bb)
		move_list.push(Move::makeEnPassant(ep_sq - EastDiag, ep_sq));
}

template <MoveGen::enumMode GenType, enumColor Side>
void generatePawnPushes(const Position& pos, MoveList& move_list, BitBoard empties) {
	static constexpr int      Dir = Side == WHITE ? 8 : -8;
	static constexpr bool     Captures = true,
							  nonCaptures = !Captures;
	static constexpr BitBoard BackRank = Side == WHITE ? BitBoard::rank<8>() : BitBoard::rank<1>(),
							  DoublePushable = Side == WHITE ? BitBoard::rank<3>() : BitBoard::rank<6>();

	BitBoard pushable = pos.get<Piece::PAWN, Side>();
	pushable = pushable.genShift<Dir>();
	pushable &= empties;

	BitBoard promoted = pushable & BackRank;
	pushable ^= promoted;

	while (promoted) {
		const Square dst = Square(promoted.dropForward());
		generatePromotions<GenType, nonCaptures>(dst - Dir, dst, move_list);
	}

	if constexpr (GenType == MoveGen::QUIETS) {
		BitBoard double_pushable = pushable & DoublePushable;
		double_pushable = double_pushable.genShift<Dir>();
		double_pushable &= empties;

		while (double_pushable) {
			const Square dst = Square(double_pushable.dropForward());
			move_list.push(Move::makeSimple(dst -  2 * Dir, dst, nonCaptures, Piece::PAWN));
		}

		while (pushable) {
			const Square dst = Square(pushable.dropForward());
			move_list.push(Move::makeSimple(dst - Dir, dst, nonCaptures, Piece::PAWN));
		}
	}
}

template <MoveGen::enumMode GenType, enumColor Side>
inline void generatePawnMoves(const Position& pos, MoveList& move_list, BitBoard enemies, BitBoard empties) {
	if constexpr (GenType != MoveGen::QUIETS)
		generatePawnCaptures<GenType, Side>(pos, move_list, enemies);
	generatePawnPushes<GenType, Side>(pos, move_list, empties);
}

template <enumColor Side, bool isCapture>
inline void generateKingMoves(const Position& pos, MoveList& move_list, BitBoard mask, BitBoard occupied, bool check) {
	const Square org = pos.getKingSquare(Side);
	// exclude opponent king's attacks from our king's attack mask - kings cannot touch
	BitBoard att = kingAttacks(org) & mask & ~kingAttacks(pos.getKingSquare(!Side));

	while (att) {
		const Square dst = att.dropForward();
		move_list.push(Move::makeSimple(org, dst, isCapture, Piece::KING));
	}

	// handle castling 
	if constexpr (isCapture) return;
	if (check) return;

	static constexpr Square ShortCastleDst = Side == WHITE ? Square::g1 : Square::g8,
							LongCastleDst = Side == WHITE ? Square::c1 : Square::c8;

	const CastlingRights own_castling_state = pos.getCastlingByColor(Side);

	if (own_castling_state.isShortPossible() and
		own_castling_state.notThroughPieces_Short<Side>(occupied) and
		own_castling_state.notThroughCheck_Short<Side>(pos))
		move_list.push(Move::makeCastling<Move::Castle::SHORT>(org, ShortCastleDst));

	if (own_castling_state.isLongPossible() and
		own_castling_state.notThroughPieces_Long<Side>(occupied) and
		own_castling_state.notThroughCheck_Long<Side>(pos))
		move_list.push(Move::makeCastling<Move::Castle::LONG>(org, LongCastleDst));
}

template <Piece::enumType Piece, enumColor Side, bool isCapture> 
void generate(const Position& pos, MoveList& move_list, BitBoard mask, BitBoard occupied) {
	static_assert(Piece != Piece::PAWN and Piece != Piece::NONE and Piece != Piece::KING, 
		"Unsupported piecetype in generate func template");

	BitBoard pieces = pos.get<Piece, Side>();

	while (pieces) {
		const Square org = Square(pieces.dropForward());
		BitBoard att = attacks<Piece>(org, occupied) & mask;

		while (att) {
			const Square dst = Square(att.dropForward());
			move_list.push(Move::makeSimple(org, dst, isCapture, Piece));
		}
	}
}

template <MoveGen::enumMode GenType, enumColor Side>
void generateByColor(const Position& pos, MoveList& move_list, const BitBoard occupied, 
	const BitBoard enemy_pieces, const BitBoard checkers) {
	static constexpr bool areCaptures = GenType != MoveGen::QUIETS;
	const BitBoard		  empties = ~occupied,
						  gen_mask = GenType == MoveGen::QUIETS ? empties : enemy_pieces;
	const bool			  check = static_cast<bool>(checkers);

	BitBoard pieces_mask = gen_mask;

	if (check)
		pieces_mask &= inBetween(pos.getKingSquare(Side), checkers.bitScanForward());

	generatePawnMoves<GenType, Side>(pos, move_list, enemy_pieces, empties);

	generate<Piece::KNIGHT, Side, areCaptures>(pos, move_list, pieces_mask, occupied);
	generate<Piece::BISHOP, Side, areCaptures>(pos, move_list, pieces_mask, occupied);
	generate<Piece::ROOK, Side, areCaptures>(pos, move_list, pieces_mask, occupied);
	generate<Piece::QUEEN, Side, areCaptures>(pos, move_list, pieces_mask, occupied);

	generateKingMoves<Side, areCaptures>(pos, move_list, gen_mask, occupied, check);
}

template <MoveGen::enumMode GenType>
void MoveGen::generatePseudoLegalMoves(const Position& pos, MoveList& move_list) {
	const BitBoard enemy_pieces = pos.getOppositePieces(),
				   occupied = enemy_pieces | pos.getOwnPieces(),
				   checkers = pos.getCheckers(pos.getTurn());

	static_cast<enumColor>(pos.getTurn()) == WHITE ? 
		generateByColor<GenType, WHITE>(pos, move_list, occupied, enemy_pieces, checkers) :
		generateByColor<GenType, BLACK>(pos, move_list, occupied, enemy_pieces, checkers);
}

template <>
void MoveGen::generatePseudoLegalMoves<MoveGen::ALL>(const Position& pos, MoveList& move_list) {
	const BitBoard enemy_pieces = pos.getOppositePieces(),
				   occupied = enemy_pieces | pos.getOwnPieces(),
				   checkers = pos.getCheckers(pos.getTurn());

	if (pos.getTurn() == WHITE) {
		generateByColor<CAPTURES, WHITE>(pos, move_list, occupied, enemy_pieces, checkers);
		generateByColor<QUIETS, WHITE>  (pos, move_list, occupied, enemy_pieces, checkers);
	}
	else {
		generateByColor<CAPTURES, BLACK>(pos, move_list, occupied, enemy_pieces, checkers);
		generateByColor<QUIETS, BLACK>  (pos, move_list, occupied, enemy_pieces, checkers);
	}
}

template void MoveGen::generatePseudoLegalMoves<MoveGen::CAPTURES>(const Position&, MoveList&);
template void MoveGen::generatePseudoLegalMoves<MoveGen::TACTICALS>(const Position&, MoveList&);
template void MoveGen::generatePseudoLegalMoves<MoveGen::QUIETS>(const Position&, MoveList&);