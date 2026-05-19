#pragma once

#include "Common.hpp"
#include "BitBoard.hpp"
#include "Magic.hpp"

/* Static and pre-computed attack masks for double-sided pawns, knights as also kings.
*/
struct StaticAttackTables {
	StaticAttackTables() 
		: pawns_attacks_mask{}
		, knights_attacks_mask{}
		, kings_attacks_mask{} { 

		for (enumColor col : { WHITE, BLACK }) {
			for (int sq = 0; sq < 64; sq++) {
				pawns_attacks_mask[col][sq] = col == WHITE 
					? whitePawnAttacksOnFly(sq)
					: blackPawnAttacksOnFly(sq);
			}
		}

		for (int sq = 0; sq < 64; sq++) {
			knights_attacks_mask[sq] = knightAttacksOnFly(sq);
			kings_attacks_mask[sq] = kingAttacksOnFly(sq);
		}
	}

	BitBoard whitePawnAttacksOnFly(Square sq) {
		const BitBoard bit(sq);
		return noEaOne(bit) | noWeOne(bit);
	}

	BitBoard blackPawnAttacksOnFly(Square sq) {
		const BitBoard bit(sq);
		return soEaOne(bit) | soWeOne(bit);
	}

	BitBoard knightAttacksOnFly(Square sq) {
		const BitBoard bit(sq);
		return noNoEa(bit) | noEaEa(bit) 
			 | soEaEa(bit) | soSoEa(bit) 
			 | soSoWe(bit) | soWeWe(bit)
			 | noWeWe(bit) | noNoWe(bit);
	}

	BitBoard kingAttacksOnFly(Square sq) {
		const BitBoard bit(sq);
		return nortOne(bit) | noEaOne(bit) 
			 | eastOne(bit) | soEaOne(bit) 
			 | soutOne(bit) | soWeOne(bit)
			 | westOne(bit) | noWeOne(bit);
	}

	array2d<BitBoard, 2, 64> pawns_attacks_mask;
	array1d<BitBoard, 64> knights_attacks_mask, 
							 kings_attacks_mask;
};

inline const StaticAttackTables StaticAttacks;

namespace {

_FORCEINLINE BitBoard pawnAttacks(Square sq, enumColor col_type) {
	assert(sq.isValid() and !sq.isNull());
	return StaticAttacks.pawns_attacks_mask[col_type][sq];
}

_FORCEINLINE BitBoard knightAttacks(Square sq) {
	assert(sq.isValid() and !sq.isNull());
	return StaticAttacks.knights_attacks_mask[sq];
}

_FORCEINLINE BitBoard kingAttacks(Square sq) {
	assert(sq.isValid() and !sq.isNull());
	return StaticAttacks.kings_attacks_mask[sq];
}

template <Piece::enumType Piece>
_INLINE BitBoard attacks(Square sq, BitBoard occ) {
	static_assert(Piece != Piece::PAWN and Piece != Piece::NONE, 
		"Unsupported piecetype in attacks func template");

	if constexpr (Piece == Piece::KNIGHT)
		return knightAttacks(sq);
	else if constexpr (Piece == Piece::BISHOP)
		return SlidersAttacks::bishopAttacks(sq, occ);
	else if constexpr (Piece == Piece::ROOK)
		return SlidersAttacks::rookAttacks(sq, occ);
	else if constexpr (Piece == Piece::QUEEN)
		return SlidersAttacks::queenAttacks(sq, occ);

	return kingAttacks(sq);
}

_INLINE BitBoard attacks(Piece::enumType piece, 
						 Square sq, 
						 BitBoard occ) 
{
	switch (piece) {
	case Piece::KNIGHT:
		return knightAttacks(sq);
	case Piece::BISHOP:
		return SlidersAttacks::bishopAttacks(sq, occ);
	case Piece::ROOK:
		return SlidersAttacks::rookAttacks(sq, occ);
	case Piece::QUEEN:
		return SlidersAttacks::queenAttacks(sq, occ);
	default: break;
	}

	return kingAttacks(sq);
}

_INLINE BitBoard attacksIncludePawns(Piece::enumType piece, 
									 Square sq, 
									 BitBoard occ, 
									 enumColor col) 
{
	switch (piece) {
	case Piece::PAWN:
		return pawnAttacks(sq, col);
	case Piece::KNIGHT:
		return knightAttacks(sq);
	case Piece::BISHOP:
		return SlidersAttacks::bishopAttacks(sq, occ);
	case Piece::ROOK:
		return SlidersAttacks::rookAttacks(sq, occ);
	case Piece::QUEEN:
		return SlidersAttacks::queenAttacks(sq, occ);
	default: break;
	}

	return kingAttacks(sq);
}

} // namespace
