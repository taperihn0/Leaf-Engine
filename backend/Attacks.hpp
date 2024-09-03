#pragma once

#include "Common.hpp"
#include "BitBoard.hpp"
#include "Magic.hpp"

namespace {
	// static attack masks

	inline BitBoard whitePawnAttacks(Square sq) {
		BitBoard bit(sq);
		return noEaOne(bit) | noWeOne(bit);
	}

	inline BitBoard blackPawnAttacks(Square sq) {
		BitBoard bit(sq);
		return soEaOne(bit) | soWeOne(bit);
	}

	inline BitBoard pawnAttacks(Square sq, enumColor col_type) {
		return col_type == WHITE ? whitePawnAttacks(sq) : blackPawnAttacks(sq);
	}

	inline BitBoard knightAttacks(Square sq) {
		BitBoard bit(sq);
		return noNoEa(bit) | noEaEa(bit) | soEaEa(bit)
			| soSoEa(bit) | soSoWe(bit) | soWeWe(bit)
			| noWeWe(bit) | noNoWe(bit);
	}

	inline BitBoard kingAttacks(Square sq) {
		BitBoard bit(sq);
		return nortOne(bit) | noEaOne(bit) | eastOne(bit)
			| soEaOne(bit) | soutOne(bit) | soWeOne(bit)
			| westOne(bit) | noWeOne(bit);
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