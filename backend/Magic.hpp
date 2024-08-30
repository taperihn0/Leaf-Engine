#pragma once

#include "Common.hpp"
#include "BitBoard.hpp"
#include "Position.hpp"

// class containing magic bitboards for bishops and rooks,
// encapsulating hashing function for sliding pieces 
class SlidersMagics {
public:
    static INLINE constexpr int mIndexHash(BitBoard magic_bb, BitBoard relv_occ, int relv_bits) {
        return static_cast<int>((relv_occ * magic_bb) >> (64 - relv_bits));
    }

    static INLINE BitBoard bishopAttacks(Square sq, BitBoard occ) {
        return _mbishop_att[sq][mIndexHash(_magics_bishop[sq], _m_occupancy_bishop[sq] & occ, _m_bits_bishop[sq])];
    }

    static INLINE BitBoard rookAttacks(Square sq, BitBoard occ) {
        return _mrook_att[sq][mIndexHash(_magics_rook[sq], _m_occupancy_rook[sq] & occ, _m_bits_rook[sq])];
    }

    // initialize look-up tables for bishop and rook
    template <enumPiece Piece>
    static void initAttackTables();
private:
    static BitBoard indexToSubset(int i, BitBoard relv_occ, int relv_bits);

    static uint64_t generateBishopAttacks(Square sq, BitBoard relv_occ);
    static uint64_t generateRookAttacks(Square sq, BitBoard relv_occ);

    // magic bitboards for bishop and rook
	static std::array<uint64_t, 64> _magics_bishop, _magics_rook;

    // look-up tables of rook and bishop attacks in Plain Magic Bitboards implementation
    // 4096 = 2 ^ 12 - maximum number of occupancy subsets for rook (rook at [a1, h8])
    // 512 = 2 ^ 9 - maximum number of occupancy subsets for bishop (bishop at board center [d4, d5, e4, e5])
    static std::array<std::array<uint64_t, 4096>, 64> _mrook_att;
    static std::array<std::array<uint64_t, 512>, 64> _mbishop_att;

    // relevant occupancy pre-computed masks
    static std::array<uint64_t, 64> _m_occupancy_bishop, _m_occupancy_rook;

    // relevant occupancy bits count for bishop and rook - later used in hash function
    // while shifting product number
    static std::array<int, 64> _m_bits_bishop, _m_bits_rook;
};