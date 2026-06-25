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

#pragma once

#include "Common.hpp"
#include "BitBoard.hpp"
#include "Piece.hpp"
#include "Simd.hpp"

/* Class containing magic bitboards for bishops and rooks,
*  encapsulating hashing function for sliding pieces.
*  Resources need to be initializated by calling initAttackTables function.
*/
class SlidersAttacks {
public:
	SlidersAttacks() = delete;

	_NODISCARD static _FORCEINLINE constexpr uint16_t mIndexHash(BitBoard magic_bb,
													  			 BitBoard relv_occ, 
													  			 uint8_t relv_bits) noexcept
	{
		return static_cast<uint16_t>((relv_occ * magic_bb) >> (64 - relv_bits));
	}
	
	_GNU_TARGET_BMI2_AVX2 _NODISCARD static _INLINE uint16_t pextIndexHash(BitBoard relv_mask, 
																		   BitBoard occ)
	{
#if defined(LEAF_ENABLE_BMI2)
		return static_cast<uint16_t>(_pext_u64(static_cast<ull>(occ), 
											   static_cast<ull>(relv_mask)));
#else
		_declUnused(relv_mask & occ);
		ASSERT(false, "BMI2 is not enabled: 'pextIndexHash' is unimplemented.");
		return 0;
#endif
	}

	_NODISCARD static _FORCEINLINE BitBoard bishopAttacks(Square sq, BitBoard occ) {
#if defined(LEAF_ENABLE_BMI2)
		const uint16_t sqidx = pextIndexHash(_relv_occupancy_bishop[sq], occ);
#else
		const uint16_t sqidx = mIndexHash(_magics_bishop[sq], 
										  _relv_occupancy_bishop[sq] & occ, 
										  _relv_bits_cnt_bishop[sq]);
#endif
		return _mbishop_att[sq][sqidx];
	}

	_NODISCARD static _FORCEINLINE BitBoard rookAttacks(Square sq, BitBoard occ) {
#if defined(LEAF_ENABLE_BMI2)
		const size_t sqidx = pextIndexHash(_relv_occupancy_rook[sq], occ);
#else
		const size_t sqidx = mIndexHash(_magics_rook[sq], 
										_relv_occupancy_rook[sq] & occ, 
										_relv_bits_cnt_rook[sq]);
#endif
		return _mrook_att[sq][sqidx];
	}

	_NODISCARD static _FORCEINLINE BitBoard queenAttacks(Square sq, BitBoard occ) {
		return rookAttacks(sq, occ) | bishopAttacks(sq, occ);
	}

	_NODISCARD static _FORCEINLINE BitBoard xRayBishopAttacks(Square sq) {
		const BitBoard bb = _mbishop_att[sq][0];
		assert(bb.popCount() == _relv_bits_cnt_bishop[sq]);
		return bb;
	}

	_NODISCARD static _FORCEINLINE BitBoard xRayRookAttacks(Square sq) {
		const BitBoard bb = _mrook_att[sq][0];
		assert(bb.popCount() == _mrook_att[sq]);
		return bb;
	}

	_NODISCARD static _FORCEINLINE BitBoard xRayQueenAttacks(Square sq) {
		return xRayBishopAttacks(sq) | xRayRookAttacks(sq);
	}

	static void initTables();
private:
	// initialize look-up tables for bishop and rook
	template <Piece::enumType Piece>
	static void initAttackTables();

	static BitBoard indexToSubset(uint64_t i, BitBoard relv_occ, uint8_t relv_bits);
	static uint64_t generateBishopAttacks(Square sq, BitBoard relv_occ);
	static uint64_t generateRookAttacks(Square sq, BitBoard relv_occ);

#if !defined(LEAF_ENABLE_BMI2)
	// magic bitboards for bishop and rook
	static const array1d<uint64_t, 64> _magics_bishop, 
									   _magics_rook;
#endif // !LEAF_ENABLE_BMI2

	/* relevant occupancy bits count for bishop and rook - later used in 'mIndexHash' hash function
	*  while shifting product number
	*/
	static const array1d<uint8_t, 64> _relv_bits_cnt_bishop, 
								   	  _relv_bits_cnt_rook;

	/* look-up tables of rook and bishop attacks in Plain Magic Bitboards implementation 
	*  4096 = 2 ^ 12 - maximum number of occupancy subsets for rook (rook at [a1, h8])
	*  512 = 2 ^ 9 - maximum number of occupancy subsets for bishop (bishop at board center [d4, d5, e4, e5]) 
	*/
	static array2d<uint64_t, 64, 4096> _mrook_att;
	static array2d<uint64_t, 64, 512>  _mbishop_att;

	// relevant occupancy pre-computed masks
	static const array1d<uint64_t, 64> _relv_occupancy_bishop, 
									   _relv_occupancy_rook;
};
