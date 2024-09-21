#pragma once

#include "Common.hpp"
#include "BitBoard.hpp"
#include "Magic.hpp"

// static and pre-computed attack masks for double-sided pawns, knights as also kings.
struct StaticAttackTables {
	StaticAttackTables() { init(); }

	inline BitBoard whitePawnAttacksOnFly(Square sq) {
		BitBoard bit(sq);
		return noEaOne(bit) | noWeOne(bit);
	}

	inline BitBoard blackPawnAttacksOnFly(Square sq) {
		BitBoard bit(sq);
		return soEaOne(bit) | soWeOne(bit);
	}

	inline BitBoard knightAttacksOnFly(Square sq) {
		BitBoard bit(sq);
		return noNoEa(bit) | noEaEa(bit) | soEaEa(bit)
			| soSoEa(bit) | soSoWe(bit) | soWeWe(bit)
			| noWeWe(bit) | noNoWe(bit);
	}

	inline BitBoard kingAttacksOnFly(Square sq) {
		BitBoard bit(sq);
		return nortOne(bit) | noEaOne(bit) | eastOne(bit)
			| soEaOne(bit) | soutOne(bit) | soWeOne(bit)
			| westOne(bit) | noWeOne(bit);
	}

	// generate attack masks for each square
	void init() {
		for (enumColor col : { WHITE, BLACK })
			for (int sq = 0; sq < 64; sq++)
				for_pawns[col][sq] = col == WHITE ? whitePawnAttacksOnFly(sq)
				: blackPawnAttacksOnFly(sq);

		for (int sq = 0; sq < 64; sq++) {
			for_knights[sq] = knightAttacksOnFly(sq);
			for_kings[sq] = kingAttacksOnFly(sq);
		}
	}

	std::array<std::array<BitBoard, 64>, 2> for_pawns;
	std::array<BitBoard, 64> for_knights, for_kings;
};

inline const StaticAttackTables attack_tables;

namespace {

	INLINE BitBoard pawnAttacks(Square sq, enumColor col_type) {
		assert(sq.isValid() and sq.isNotNull());
		return attack_tables.for_pawns[col_type][sq];
	}

	INLINE BitBoard knightAttacks(Square sq) {
		assert(sq.isValid() and sq.isNotNull());
		return attack_tables.for_knights[sq];
	}

	INLINE BitBoard kingAttacks(Square sq) {
		assert(sq.isValid() and sq.isNotNull());
		return attack_tables.for_kings[sq];
	}

	BitBoard nortRay(Square sq) {
		return 0x0101010101010100Ui64 << sq;
	}

	BitBoard soutRay(Square sq) {
		return 0x0080808080808080Ui64 >> (sq ^ 63);
	}

	BitBoard westRay(Square sq) {
		return (1Ui64 << sq) - (1Ui64 << (sq & 56));
	}

	BitBoard eastRay(Square sq) {
		return 2 * ((1Ui64 << (sq | 7)) - (1Ui64 << sq));
	}

	BitBoard noEaRay(Square sq) {
		static constexpr BitBoard excl_a = BitBoard::not_a_file,
			excl_ab = excl_a & (excl_a << 9),
			excl_abcd = excl_ab & (excl_ab << 18);

		BitBoard bb(sq);
		bb |= (bb << 9) & excl_a;
		bb |= (bb << 18) & excl_ab;
		bb |= (bb << 36) & excl_abcd;
		return bb & ~BitBoard(sq);
	}

	BitBoard soEaRay(Square sq) {
		static constexpr BitBoard excl_a = BitBoard::not_a_file,
			excl_ab = excl_a & (excl_a >> 7),
			excl_abcd = excl_ab & (excl_ab >> 14);

		BitBoard bb(sq);
		bb |= (bb >> 7) & excl_a;
		bb |= (bb >> 14) & excl_ab;
		bb |= (bb >> 28) & excl_abcd;
		return bb ^ BitBoard(sq);
	}

	BitBoard soWeRay(Square sq) {
		static constexpr BitBoard excl_h = BitBoard::not_h_file,
			excl_gh = excl_h & (excl_h >> 9),
			excl_efgh = excl_gh & (excl_gh >> 18);

		BitBoard bb(sq);
		bb |= (bb >> 9) & excl_h;
		bb |= (bb >> 18) & excl_gh;
		bb |= (bb >> 36) & excl_efgh;
		return bb ^ BitBoard(sq);
	}

	BitBoard noWeRay(Square sq) {
		static constexpr BitBoard excl_h = BitBoard::not_h_file,
			excl_gh = excl_h & (excl_h << 7),
			excl_efgh = excl_gh & (excl_gh << 14);

		BitBoard bb(sq);
		bb |= (bb << 7) & excl_h;
		bb |= (bb << 14) & excl_gh;
		bb |= (bb << 28) & excl_efgh;
		return bb ^ BitBoard(sq);
	}

	BitBoard rayAttacksBishop(Square sq) {
		return noEaRay(sq) | soEaRay(sq) | soWeRay(sq) | noWeRay(sq);
	}

	BitBoard rayAttacksRook(Square sq) {
		return nortRay(sq) | soutRay(sq) | westRay(sq) | eastRay(sq);
	}

	BitBoard rayAttacksQueen(Square sq) {
		return rayAttacksBishop(sq) | rayAttacksRook(sq);
	}

	// generalized template
	template <Piece::enumType Piece>
	INLINE BitBoard attacks(Square sq, BitBoard occ) {
		static_assert(Piece != Piece::PAWN and Piece != Piece::NONE, "Unsupported piecetype in attacks func template");

		if constexpr (Piece == Piece::KNIGHT)
			return knightAttacks(sq);
		else if constexpr (Piece == Piece::BISHOP)
			return SlidersMagics::bishopAttacks(sq, occ);
		else if constexpr (Piece == Piece::ROOK)
			return SlidersMagics::rookAttacks(sq, occ);
		else if constexpr (Piece == Piece::QUEEN)
			return SlidersMagics::queenAttacks(sq, occ);

		return kingAttacks(sq);
	}

} // namespace