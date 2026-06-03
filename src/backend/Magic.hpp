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
		return _mbishop_att[static_cast<int8_t>(sq)][sqidx];
	}

	_NODISCARD static _FORCEINLINE BitBoard rookAttacks(Square sq, BitBoard occ) {
#if defined(LEAF_ENABLE_BMI2)
		const size_t sqidx = pextIndexHash(_relv_occupancy_rook[sq], occ);
#else
		const size_t sqidx = mIndexHash(_magics_rook[sq], 
										_relv_occupancy_rook[sq] & occ, 
										_relv_bits_cnt_rook[sq]);
#endif
		return _mrook_att[static_cast<int8_t>(sq)][sqidx];
	}

	_NODISCARD static _FORCEINLINE BitBoard queenAttacks(Square sq, BitBoard occ) {
		return rookAttacks(sq, occ) | bishopAttacks(sq, occ);
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
