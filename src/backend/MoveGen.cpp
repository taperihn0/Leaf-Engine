/*
 * Leaf, a UCI Chess Engine
 * Copyright (C) 2026 taperihn0
 *
 * Leaf is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Leaf is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#include "MoveGen.hpp"
#include "Attacks.hpp"

enum enumLegality : uint8_t {
	LEGAL,
	PSEUDOLEGAL
};

struct CacheKingRelated {
	Square   ksq;
	BitBoard rook_att_from_ksq;
	BitBoard bishop_att_from_ksq;
	BitBoard diag_pinned_pcs;
	BitBoard hv_pinned_pcs;
	BitBoard pinned;
};

template <MoveGen::enumGenMoves Moves2Gen, bool Capture>
_INLINE void generatePromotions(Square origin, 
							    Square target, 
							    MoveList& move_list) 
{
	if constexpr (Moves2Gen == MoveGen::CAPTURES or 
				  Moves2Gen == MoveGen::TACTICALS) {
		move_list.push(Move32b::makePromotion(origin, target, Capture, Piece::QUEEN));
	}

	if constexpr (Capture or 
				  Moves2Gen == MoveGen::QUIETS or 
				  Moves2Gen == MoveGen::TACTICALS) {
		move_list.push(Move32b::makePromotion(origin, target, Capture, Piece::KNIGHT));
		move_list.push(Move32b::makePromotion(origin, target, Capture, Piece::BISHOP));
		move_list.push(Move32b::makePromotion(origin, target, Capture, Piece::ROOK));
	}
}

template <MoveGen::enumGenMoves Moves2Gen, enumLegality LMode, enumColor Side>
void generatePawnCaptures(const Position& pos, 
						  MoveList& move_list, 
						  BitBoard mask,
						  BitBoard enemies,
						  const CacheKingRelated& cache) 
{
	static constexpr int      Dir = Side == WHITE ? 8 : -8;
	static constexpr int      NortWest = 7, 
							  NortEast = 9, 
							  SoutWest = -9, 
							  SoutEast = -7;
	static constexpr int      WestDiag = Side == WHITE ? NortWest : SoutWest,
							  EastDiag = Side == WHITE ? NortEast : SoutEast;
	static constexpr bool     Captures = true;
	static constexpr BitBoard BackRank = Side == WHITE ? BitBoard::rank<8>() 
													   : BitBoard::rank<1>();
	
	const BitBoard pawns = pos.get<Piece::PAWN, Side>();
	const BitBoard av_mask = enemies & mask;

	BitBoard att;

	// Western captures only
	if constexpr (Side == WHITE)
		att = noWeOne(pawns) & av_mask;
	else 
		att = soWeOne(pawns) & av_mask;

	// exclude pawns that cannot move due to pin
	if constexpr (LMode == LEGAL) {
		att &= ~cache.hv_pinned_pcs.genShift<WestDiag>();
	}

	BitBoard promoted = att & BackRank;

	/* Handle pins for pawn promotions and east captures at the same time */
	if constexpr (LMode == LEGAL) {
		BitBoard pins = promoted & cache.diag_pinned_pcs.genShift<WestDiag>();

		promoted ^= pins;
		pins &= cache.bishop_att_from_ksq;

		while (pins) {
			const Square dst(pins.dropForward());
			generatePromotions<Moves2Gen, Captures>(dst - WestDiag, dst, move_list);
		}
	}

	att ^= promoted;

	while (promoted) {
		const Square dst(promoted.dropForward());
		generatePromotions<Moves2Gen, Captures>(dst - WestDiag, dst, move_list);
	}

	/* Handle pins for west pawn captures */
	if constexpr (LMode == LEGAL) {
		BitBoard pins = att & cache.diag_pinned_pcs.genShift<WestDiag>();

		att ^= pins;
		pins &= cache.bishop_att_from_ksq;

		while (pins) {
			const Square dst(pins.dropForward());
			move_list.push(Move32b::makeSimple(dst - WestDiag, dst, Captures, Piece::PAWN));
		}
	}

	while (att) {
		const Square dst(att.dropForward());
		move_list.push(Move32b::makeSimple(dst - WestDiag, dst, Captures, Piece::PAWN));
	}

	// Eastern captures left
	if constexpr (Side == WHITE)
		att = noEaOne(pawns) & av_mask;
	else
		att = soEaOne(pawns) & av_mask;

	// exclude pawns that cannot move due to pin
	if constexpr (LMode == LEGAL) {
		att &= ~cache.hv_pinned_pcs.genShift<EastDiag>();
	}

	promoted = att & BackRank;

	/* Handle pins for pawn promotions and east captures at the same time */
	if constexpr (LMode == LEGAL) {
		BitBoard pins = promoted & cache.diag_pinned_pcs.genShift<EastDiag>();

		promoted ^= pins;
		pins &= cache.bishop_att_from_ksq;

		while (pins) {
			const Square dst(pins.dropForward());
			generatePromotions<Moves2Gen, Captures>(dst - EastDiag, dst, move_list);
		}
	}

	att ^= promoted;

	while (promoted) {
		const Square dst(promoted.dropForward());
		generatePromotions<Moves2Gen, Captures>(dst - EastDiag, dst, move_list);
	}

	/* Handle pins for east pawn captures */
	if constexpr (LMode == LEGAL) {
		BitBoard pins = att & cache.diag_pinned_pcs.genShift<EastDiag>();

		att ^= pins;
		pins &= cache.bishop_att_from_ksq;

		while (pins) {
			const Square dst(pins.dropForward());
			move_list.push(Move32b::makeSimple(dst - EastDiag, dst, Captures, Piece::PAWN));
		}
	}

	while (att) {
		const Square dst(att.dropForward());
		move_list.push(Move32b::makeSimple(dst - EastDiag, dst, Captures, Piece::PAWN));
	}
	
	// En-passant validation
	const Square ep_sq = pos.getEnPassantSq();

	if (ep_sq.isNull()) return;

	const BitBoard ep_bb = BitBoard(ep_sq);

	/* Handle pinned pawns that can do en-passant capture */
	if constexpr (LMode == LEGAL) {
		array2d<BitBoard, 2, 6> pc_bbs = {
			pos.getPawnsBySide(WHITE),
			pos.getKnightsBySide(WHITE),
			pos.getBishopsBySide(WHITE),
			pos.getRooksBySide(WHITE),
			pos.getQueensBySide(WHITE),
			pos.getKingBySide(WHITE),
			pos.getPawnsBySide(BLACK),
			pos.getKnightsBySide(BLACK),
			pos.getBishopsBySide(BLACK),
			pos.getRooksBySide(BLACK),
			pos.getQueensBySide(BLACK),
			pos.getKingBySide(BLACK),
		};

		if (pawns.pawnsAttack<WestDiag>() & ep_bb) {
			const Square org = ep_sq - WestDiag;

			const BitBoard after_opp_pawns = pos.getPawnsBySide(!Side) & ~BitBoard(Square(ep_sq - Dir));
			const BitBoard after_own_pawns = pawns ^ BitBoard(org) ^ ep_bb;

			pc_bbs[Side][Piece::PAWN] = after_own_pawns;
			pc_bbs[!Side][Piece::PAWN] = after_opp_pawns;

			if (!Position::isAttackedSquareWithOccupancies(cache.ksq, Side, pc_bbs))
				move_list.push(Move32b::makeEnPassant(org, ep_sq));
		}

		if (pawns.pawnsAttack<EastDiag>() & ep_bb) {
			const Square org = ep_sq - EastDiag;

			const BitBoard after_opp_pawns = pos.getPawnsBySide(!Side) & ~BitBoard(Square(ep_sq - Dir));
			const BitBoard after_own_pawns = (pawns ^ BitBoard(org)) | ep_bb;

			pc_bbs[Side][Piece::PAWN] = after_own_pawns;
			pc_bbs[!Side][Piece::PAWN] = after_opp_pawns;

			if (!Position::isAttackedSquareWithOccupancies(cache.ksq, Side, pc_bbs))
				move_list.push(Move32b::makeEnPassant(org, ep_sq));
		}
	}
	else /* LMode == PSEUDOLEGAL */ { 
		if (pawns.pawnsAttack<WestDiag>() & ep_bb) {
			move_list.push(Move32b::makeEnPassant(ep_sq - WestDiag, ep_sq));
		}

		if (pawns.pawnsAttack<EastDiag>() & ep_bb) {
			move_list.push(Move32b::makeEnPassant(ep_sq - EastDiag, ep_sq));
		}
	}
}

template <MoveGen::enumGenMoves Moves2Gen, enumLegality LMode, enumColor Side>
void generatePawnPushes(const Position& pos, 
						MoveList& move_list, 
						BitBoard mask,
						BitBoard empties,
						const CacheKingRelated& cache) 
{
	static constexpr int      Dir = Side == WHITE ? 8 : -8;
	static constexpr bool     Captures = true,
							  nonCaptures = !Captures;
	static constexpr BitBoard BackRank = Side == WHITE ? BitBoard::rank<8>() : BitBoard::rank<1>(),
							  DoublePushable = Side == WHITE ? BitBoard::rank<3>() : BitBoard::rank<6>();

	BitBoard pushable = pos.get<Piece::PAWN, Side>();

	// exclude pinned pawns that cannot move anyway
	if constexpr (LMode == LEGAL)
		pushable &= ~cache.diag_pinned_pcs;

	pushable = pushable.genShift<Dir>();
	pushable &= empties;

	BitBoard promoted = pushable & BackRank;

	if constexpr (LMode == LEGAL)
		promoted &= mask;

	/* Handle pinned pawns that are ready to promote.
	*  These pawns cannot be pushed anyway.
	*/
	if constexpr (LMode == LEGAL)
		promoted &= ~cache.hv_pinned_pcs.genShift<Dir>();

	pushable ^= promoted;

	while (promoted) {
		const Square dst(promoted.dropForward());
		generatePromotions<Moves2Gen, nonCaptures>(dst - Dir, dst, move_list);
	}

	if constexpr (Moves2Gen == MoveGen::QUIETS) {
		BitBoard double_pushable = pushable & DoublePushable;
		double_pushable = double_pushable.genShift<Dir>();
		double_pushable &= empties & mask;

		/* We can push only those pawns that are h-vertically pinned and 
		*  can move up in a king pin line.
		*/

		/* Handle pins in double-pushable pawns */
		if constexpr (LMode == LEGAL) {
			BitBoard pins = double_pushable & cache.hv_pinned_pcs.genShift<2 * Dir>();

			double_pushable ^= pins;
			pins &= cache.rook_att_from_ksq;

			while (pins) {
				const Square dst(pins.dropForward());
				move_list.push(Move32b::makeSimple(dst -  2 * Dir, dst, nonCaptures, Piece::PAWN));
			}
		}

		while (double_pushable) {
			const Square dst(double_pushable.dropForward());
			move_list.push(Move32b::makeSimple(dst -  2 * Dir, dst, nonCaptures, Piece::PAWN));
		}

		pushable &= mask;

		/* Handle pins in pushable pawns */
		if constexpr (LMode == LEGAL) {
			BitBoard pins = pushable & cache.hv_pinned_pcs.genShift<Dir>();

			pushable ^= pins;
			pins &= cache.rook_att_from_ksq;

			while (pins) {
				const Square dst(pins.dropForward());
				move_list.push(Move32b::makeSimple(dst - Dir, dst, nonCaptures, Piece::PAWN));
			}
		}

		while (pushable) {
			const Square dst(pushable.dropForward());
			move_list.push(Move32b::makeSimple(dst - Dir, dst, nonCaptures, Piece::PAWN));
		}
	}
}

template <MoveGen::enumGenMoves Moves2Gen, enumLegality LMode, enumColor Side>
_FORCEINLINE void generatePawnMoves(const Position& pos, 
							  		MoveList& move_list, 
									BitBoard mask,
							  		BitBoard enemies, 
							  		BitBoard empties,
									const CacheKingRelated& cache) 
{
	if constexpr (Moves2Gen != MoveGen::QUIETS)
		generatePawnCaptures<Moves2Gen, LMode, Side>(pos, move_list, mask, enemies, cache);

	generatePawnPushes<Moves2Gen, LMode, Side>(pos, move_list, mask, empties, cache);
}

template <enumColor Side, enumLegality LMode, bool isCapture>
void generateKingMoves(const Position& pos, 
					   MoveList& move_list, 
					   BitBoard mask, 
					   BitBoard occupied, 
					   bool check,
					   const CacheKingRelated& cache) 
{
	const Square org = cache.ksq;
	// exclude opponent king's attacks from our king's attack mask - kings cannot touch
	BitBoard att = kingAttacks(org) & mask & ~kingAttacks(pos.getKingSquareBySide(!Side));
	BitBoard attacked_mask = BitBoard::Empty;

	// exclude already attacked squares
	if constexpr (LMode == LEGAL) {
		attacked_mask = pos.getAttackedMaskForLegalKingMoves(!Side);
		att &= ~attacked_mask;
	}

	while (att) {
		const Square dst = att.dropForward();
		move_list.push(Move32b::makeSimple(org, dst, isCapture, Piece::KING));
	}

	// handle castling 
	if constexpr (isCapture) return;
	if (check) return;

	static constexpr Square ShortCastleDst = Side == WHITE ? Square::SQ_G1 : Square::SQ_G8,
							LongCastleDst = Side == WHITE ? Square::SQ_C1 : Square::SQ_C8;

	const CastlingRights own_castling_state = pos.getCastlingByColor(Side);

	if (own_castling_state.isShortPossible() and
		own_castling_state.notThroughPieces_Short<Side>(occupied) and
		own_castling_state.notThroughCheck_Short<Side>(pos) and
	    (LMode != LEGAL or !attacked_mask.isOccupiedSq(ShortCastleDst))) {
		move_list.push(Move32b::makeCastling<Move32b::Castle::SHORT>(org, ShortCastleDst));
	}

	if (own_castling_state.isLongPossible() and
		own_castling_state.notThroughPieces_Long<Side>(occupied) and
		own_castling_state.notThroughCheck_Long<Side>(pos) and
		(LMode != LEGAL or !attacked_mask.isOccupiedSq(LongCastleDst))) {
		move_list.push(Move32b::makeCastling<Move32b::Castle::LONG>(org, LongCastleDst));
	}
}

template <Piece::enumType Pc, enumLegality LMode, enumColor Side, bool isCapture> 
_INLINE void generate(const Position& pos, 
					  MoveList& move_list, 
					  BitBoard mask, 
					  BitBoard occupied,
					  const CacheKingRelated& cache) 
{
	static_assert(Pc != Piece::PAWN and 
				  Pc != Piece::NONE and 
				  Pc != Piece::KING, 
				  "Unsupported piecetype in generate func template");

	BitBoard pieces = pos.get<Pc, Side>();

	/* We need to handle pins carefully */
	if constexpr (LMode == LEGAL and Pc != Piece::KNIGHT) {
		const BitBoard base_pin_pcs = Pc == Piece::BISHOP ? cache.diag_pinned_pcs :
									  Pc == Piece::ROOK   ? cache.hv_pinned_pcs   :
									  /* Piece::QUEEN */ 	cache.pinned;

		static auto get_pinned_queen_mask = [](Square sq, 
											   const Position& pos, 
											   const CacheKingRelated& cache)
		{
			const BitBoard opp_pieces = pos.getOppositePieces();
			const BitBoard king_qray = attacks<Piece::QUEEN>(cache.ksq, opp_pieces);
			const BitBoard blockers = BitBoard(sq) & king_qray;
			const BitBoard xray = king_qray ^ attacks<Piece::QUEEN>(cache.ksq, opp_pieces ^ blockers);
			return king_qray & xray | inBetween(cache.ksq, sq);
		};

		BitBoard pins = pieces & base_pin_pcs;
		// we need to exclude pinned pieces that are already processed
		pieces ^= pins;

		while (pins) {
			const Square org(pins.dropForward());
			const BitBoard pin_mask = Pc == Piece::BISHOP ? cache.bishop_att_from_ksq :
								  	  Pc == Piece::ROOK   ? cache.rook_att_from_ksq   :
								  	  /* Piece::QUEEN */    get_pinned_queen_mask(org, pos, cache);

			BitBoard att = attacks<Pc>(org, occupied) & mask & pin_mask;

			while (att) {
				const Square dst(att.dropForward());
				move_list.push(Move32b::makeSimple(org, dst, isCapture, Pc));
			}
		}

		// excluding pins that cannot even move
		if constexpr (Pc != Piece::QUEEN) {
			const BitBoard excl_pin_pcs = Pc == Piece::BISHOP ? cache.hv_pinned_pcs   :
										  /* Piece::ROOK */ 	cache.diag_pinned_pcs;
			pieces &= ~excl_pin_pcs;
		}
	}
	/* Pinned knights can be discarded - they can't move anyway */
	else if (LMode == LEGAL and Pc == Piece::KNIGHT) {
		pieces &= ~cache.pinned;
	}

	while (pieces) {
		const Square org(pieces.dropForward());
		BitBoard att = attacks<Pc>(org, occupied) & mask;

		while (att) {
			const Square dst(att.dropForward());
			move_list.push(Move32b::makeSimple(org, dst, isCapture, Pc));
		}
	}
}

_INLINE BitBoard getPinsMask(Square ksq, BitBoard pinners, const Position& pos) {
	const BitBoard own_pieces = pos.getOwnPieces();
	const BitBoard occ = pos.getOccupied();
	BitBoard pins = BitBoard::Empty;

	while (pinners) {
		const Square sq(pinners.dropForward());
		const BitBoard blockers = occ & onlyBetween(sq, ksq);

		if (blockers.isSingleBit() and blockers & own_pieces) 
			pins |= blockers;
	}

	return pins;
}

template <enumLegality LMode>
_FORCEINLINE CacheKingRelated getCache(const Position& pos, enumColor side2move) {
	CacheKingRelated cache = {
		/* cache.ksq = */ pos.getKingSquareBySide(side2move),
		BitBoard::Empty,
		BitBoard::Empty,
		BitBoard::Empty,
		BitBoard::Empty,
		BitBoard::Empty,
	};

	static auto get_diag_pins_mask = [](Square ksq, const Position& pos) _LAMBDA_FORCEINLINE {
		const enumColor side2move = pos.getTurn();
		const BitBoard pinners = pos.getBishopsQueensBySide(!side2move) & 
								 SlidersAttacks::xRayBishopAttacks(ksq);
		return getPinsMask(ksq, pinners, pos);
	};

	static auto get_horizontal_vertical_pins_mask = [](Square ksq, const Position& pos) _LAMBDA_FORCEINLINE {
		const enumColor side2move = pos.getTurn();
		const BitBoard pinners = pos.getRooksQueensBySide(!side2move) & 
								 SlidersAttacks::xRayRookAttacks(ksq);
		return getPinsMask(ksq, pinners, pos);
	};

	if constexpr (LMode == LEGAL) {
		cache.bishop_att_from_ksq = SlidersAttacks::xRayBishopAttacks(cache.ksq);
		cache.rook_att_from_ksq = SlidersAttacks::xRayRookAttacks(cache.ksq);
		cache.diag_pinned_pcs = get_diag_pins_mask(cache.ksq, pos);
		cache.hv_pinned_pcs = get_horizontal_vertical_pins_mask(cache.ksq, pos);
		cache.pinned = cache.diag_pinned_pcs | cache.hv_pinned_pcs;
	}

	return cache;
}

template <MoveGen::enumGenMoves Moves2Gen, enumLegality LMode, enumColor Side>
void generateByColor(const Position& pos, 
					 MoveList& move_list, 
					 const BitBoard occupied, 
					 const BitBoard enemy_pieces, 
					 const BitBoard checkers,
					 const CacheKingRelated& cache) 
{
	static constexpr bool areCaptures = Moves2Gen != MoveGen::QUIETS;
	const BitBoard		  empties = ~occupied,
						  base_gen_mask = Moves2Gen == MoveGen::QUIETS ? empties : enemy_pieces;
	const bool			  single_check = checkers.isSingleBit(),
						  multiple_check = checkers and !single_check,
						  check = single_check | multiple_check;
	const BitBoard 		  knight_checker = checkers & pos.getKnightsBySide(!Side);
	bool 				  only_king_moves = multiple_check;

	if constexpr (!areCaptures)
		only_king_moves |= knight_checker;

	if (!only_king_moves) {
		BitBoard base_pieces_mask = base_gen_mask;
		BitBoard pin_mask = BitBoard::Universe;

		if (check) {
			if (knight_checker) {
				base_pieces_mask &= knight_checker;
				pin_mask = knight_checker;
			}
			else {
				pin_mask = inBetween(pos.getKingSquareBySide(Side), checkers.bitScanForward());
				base_pieces_mask &= pin_mask;
			}
		}

		generatePawnMoves<Moves2Gen, LMode, Side>(pos, move_list, pin_mask, enemy_pieces, empties, cache);

		generate<Piece::KNIGHT, LMode, Side, areCaptures>(pos, move_list, base_pieces_mask, occupied, cache);
		generate<Piece::BISHOP, LMode, Side, areCaptures>(pos, move_list, base_pieces_mask, occupied, cache);
		generate<Piece::ROOK,   LMode, Side, areCaptures>(pos, move_list, base_pieces_mask, occupied, cache);
		generate<Piece::QUEEN,  LMode, Side, areCaptures>(pos, move_list, base_pieces_mask, occupied, cache);
	}

	generateKingMoves<Side, LMode, areCaptures>(pos, move_list, base_gen_mask, occupied, check, cache);
}

template <MoveGen::enumGenMoves Moves2Gen, enumLegality LMode>
void generateMovesInMode(const Position& pos, MoveList& move_list) {
	const enumColor side2move = pos.getTurn();
	const BitBoard enemy_pieces = pos.getOppositePieces(),
				   occupied = pos.getOccupied(),
				   checkers = LMode == PSEUDOLEGAL ? pos.getWeakestCheckers(side2move) 
				   								   : pos.getCheckers(side2move);

	const CacheKingRelated cache = getCache<LMode>(pos, side2move);

	if (side2move == WHITE) {
		if constexpr (Moves2Gen == MoveGen::ALL) {
			generateByColor<MoveGen::CAPTURES, LMode, WHITE>(pos, move_list, occupied, enemy_pieces, checkers, cache);
			generateByColor<MoveGen::QUIETS, LMode, WHITE>  (pos, move_list, occupied, enemy_pieces, checkers, cache);
		}
		else /* Moves2Gen != MoveGen::ALL */ {
			generateByColor<Moves2Gen, LMode, WHITE>(pos, move_list, occupied, enemy_pieces, checkers, cache);
		}
	} else /* side2move == BLACK */ {
		if constexpr (Moves2Gen == MoveGen::ALL) {
			generateByColor<MoveGen::CAPTURES, LMode, BLACK>(pos, move_list, occupied, enemy_pieces, checkers, cache);
			generateByColor<MoveGen::QUIETS, LMode, BLACK>  (pos, move_list, occupied, enemy_pieces, checkers, cache);
		}
		else /* Moves2Gen != MoveGen::ALL */ {
			generateByColor<Moves2Gen, LMode, BLACK>(pos, move_list, occupied, enemy_pieces, checkers, cache);
		}
	}
}

/*
*	MoveGen::generatePseudoLegalMoves<MoveGen::ALL> generates all pseudolegal moves.
*	Oriented towards perft generation, tested and debuged by perft function.
*
*	Its performance was measured and documented. Athough fully legal generators are much
*	more efficient in perft testing due to possibility of incorporating bulk counting in perft 
*	search, my pseudo-legal generator got some extra megaNodes per second.
*	Result above were generated in a position 
*	{ r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10 }
*	in depth 6,
*	also before implementing any other features like Zobrist Hashing.
*
*	added pre-calculated attack masks:
*	-> (217.774 seconds, 31790kN/sec.)
*
*	changed if-branches in make function:
*	-> (204.242 seconds, 33896kN/sec.)
*
*	delegated variable definitions in make and unmake:
*	-> (192.031 seconds, 36051kN/sec.)
*
*	changed if-branch for captures and r-value references in MoveList::push:
*	-> (190.25 seconds, 36389kN/sec.)
*
*	used __forceinline attribute:
*	-> (179.723 seconds, 38520kN/sec.)
*/

template <MoveGen::enumGenMoves Moves2Gen>
void MoveGen::generatePseudoLegalMoves(const Position& pos, MoveList& move_list) {
	generateMovesInMode<Moves2Gen, PSEUDOLEGAL>(pos, move_list);
}

template <MoveGen::enumGenMoves Moves2Gen>
void MoveGen::generateLegalMoves(Position& pos, MoveList& move_list) {
	generateMovesInMode<Moves2Gen, LEGAL>(pos, move_list);
}

template <MoveGen::enumGenMoves Moves2Gen>
Move32b MoveGen::getRandomLegalMove(Position& pos) {
	// TODO: Optimize that - generate just one move, not entire list.
    MoveList ml;
    generateLegalMoves<Moves2Gen>(pos, ml);
    return ml.getRandomMove();
}

bool MoveGen::isAnyCapture(Position& pos) {
	// TODO: Optimization.
	MoveList ml;
	generateLegalMoves<MoveGen::CAPTURES>(pos, ml);
	return ml.any([](MoveList::Entry en) {
		return en.move.isCapture();
	});
}

template void    MoveGen::generatePseudoLegalMoves<MoveGen::CAPTURES> (const Position&, MoveList&);
template void    MoveGen::generatePseudoLegalMoves<MoveGen::TACTICALS>(const Position&, MoveList&);
template void    MoveGen::generatePseudoLegalMoves<MoveGen::QUIETS>   (const Position&, MoveList&);
template void    MoveGen::generatePseudoLegalMoves<MoveGen::ALL>      (const Position&, MoveList&);

template void    MoveGen::generateLegalMoves<MoveGen::CAPTURES> (Position&, MoveList&);
template void    MoveGen::generateLegalMoves<MoveGen::TACTICALS>(Position&, MoveList&);
template void    MoveGen::generateLegalMoves<MoveGen::QUIETS>   (Position&, MoveList&);
template void    MoveGen::generateLegalMoves<MoveGen::ALL>      (Position&, MoveList&);

template Move32b MoveGen::getRandomLegalMove<MoveGen::CAPTURES> (Position&);
template Move32b MoveGen::getRandomLegalMove<MoveGen::TACTICALS>(Position&);
template Move32b MoveGen::getRandomLegalMove<MoveGen::QUIETS>   (Position&);
template Move32b MoveGen::getRandomLegalMove<MoveGen::ALL>      (Position&);
