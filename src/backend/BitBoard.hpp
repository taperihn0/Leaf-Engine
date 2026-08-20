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
#include "Square.hpp"
#include "Color.hpp"

class BitBoard {
public:
    _INLINE BitBoard() = default;
    _INLINE constexpr BitBoard(const BitBoard&) = default;
    _INLINE constexpr BitBoard(BitBoard&&) = default;

    _INLINE constexpr BitBoard& operator=(const BitBoard&) = default;
    _INLINE constexpr BitBoard& operator=(BitBoard&&) = default;

    _INLINE constexpr BitBoard(uint64_t b)
        : _board(b) {}

    _INLINE explicit constexpr BitBoard(Square sq)
        : _board(1_ui64 << sq) {}

    _INLINE explicit constexpr BitBoard(Square::enumSquare sq)
        : _board(1_ui64 << sq) {}

    _NODISCARD _INLINE constexpr operator uint64_t() const {
        return _board;
    }

    _INLINE constexpr BitBoard& operator|=(BitBoard bb) {
        _board |= bb._board;
        return *this;
    }

    _INLINE constexpr BitBoard& operator&=(BitBoard bb) {
        _board &= bb._board;
        return *this;
    }

    _INLINE constexpr BitBoard& operator^=(BitBoard bb) {
        _board ^= bb._board;
        return *this;
    }

    _INLINE constexpr BitBoard& operator>>=(int shift) {
        _board >>= shift;
        return *this;
    }

    _INLINE constexpr BitBoard& operator<<=(int shift) {
        _board <<= shift;
        return *this;
    }

    _NODISCARD _INLINE constexpr BitBoard operator|(BitBoard bb) const {
        return _board | bb._board;
    }

    _NODISCARD _INLINE constexpr BitBoard operator^(BitBoard bb) const {
        return _board ^ bb._board;
    }

    _NODISCARD _INLINE constexpr BitBoard operator^(uint64_t raw) const {
        return _board ^ raw;
    }

    _NODISCARD _INLINE constexpr BitBoard operator&(BitBoard bb) const {
        return _board & bb._board;
    }

    _NODISCARD _INLINE constexpr BitBoard operator&(uint64_t raw) const {
        return _board & raw;
    }

    _NODISCARD _INLINE constexpr BitBoard operator>>(int shift) const {
        return _board >> shift;
    }

    _NODISCARD _INLINE constexpr BitBoard operator<<(int shift) const {
        return _board << shift;
    }

    _NODISCARD _INLINE constexpr BitBoard operator*(BitBoard bb) const {
        return _board * bb._board;
    }

    _NODISCARD _INLINE constexpr BitBoard operator~() const {
        return ~_board;
    }

    _NODISCARD _INLINE constexpr BitBoard operator-() const {
        return static_cast<BitBoard>(-_board);
    }

    template <int Shift>
    _NODISCARD _INLINE BitBoard genShift() const {
        if constexpr (Shift < 0) return _board >> (-Shift);
        return _board << Shift;
    }

    _NODISCARD _INLINE BitBoard genShift(int shift) const {
        if (shift < 0) return _board >> (-shift);
        return _board << shift;
    }

    template <int Shift>
    _NODISCARD _INLINE BitBoard pawnsAttack() const {
        static_assert(Shift == 7 or Shift == -7 or Shift == 9 or Shift == -9);
        static constexpr BitBoard ExclFile = Shift == 7 or Shift == -9 ? NotHFile 
                                                                       : NotAFile;
        return genShift<Shift>() & ExclFile;
    }

    void print(std::ostream& os = std::cout) const;

    _NODISCARD int popCount() const;
    _NODISCARD int bitScanForward() const;
    _NODISCARD int bitScanReverse() const;

    _INLINE void set(uint64_t bb) {
        _board = bb;
    }

    // bit scan forward but with LS1B reset
    _INLINE int dropForward() {
        const int ls1b = bitScanForward();
        _board &= _board - 1;
        return ls1b;
    }

    _INLINE void popBit(Square sq) {
        assert(sq.isValid() and !sq.isNone());
        _board &= ~(1_ui64 << sq);
    }

    _INLINE void setBit(int shift) {
        assert(shift < 64);
        _board |= (1_ui64 << shift);
    }

    _INLINE bool getBit(int shift) const {
        assert(shift < 64);
        return _board & (1_ui64 << shift);
    }

    _NODISCARD _INLINE BitBoard swapBytes() const;

    _NODISCARD _INLINE bool isEmptySq(Square sq) const {
        return !getBit(sq);
    }

    _NODISCARD _INLINE bool isOccupiedSq(Square sq) const {
        return getBit(sq);
    }

    _INLINE void moveBit(Square origin, Square target) {
        assert(getBit(origin));
        popBit(origin);
        setBit(target);
    }

    _NODISCARD _INLINE BitBoard oneBit() const {
        return _board & -_board;
    }

    template <int Rank>
    _NODISCARD static _INLINE constexpr BitBoard rank() {
        static_assert(1 <= Rank and Rank <= 8);
        return BitBoard(0xff_ui64 << ((Rank - 1) * 8));
    }

    _NODISCARD static _INLINE constexpr BitBoard rank(int rank) {
        assert(1 <= rank and rank <= 8);
        return BitBoard(0xff_ui64 << ((rank - 1) * 8));
    }

    _NODISCARD static _INLINE constexpr BitBoard promorank(enumColor side) {
        return side == WHITE ? rank<8>() : rank<1>();
    }

    template <Square::enumFile File>
    _NODISCARD static _INLINE constexpr BitBoard file() {
        return BitBoard(AFile << static_cast<int>(File));
    }

    _NODISCARD static _INLINE constexpr BitBoard file(int file) {
        ASSERT(1 <= file and file <= 8, "Invalid file");
        return BitBoard(AFile << file);
    }

    _NODISCARD _INLINE constexpr bool isEmpty() const {
        return _board == BitBoard::Empty;
    }

    _NODISCARD _INLINE constexpr bool isSingleBit() const {
        return !isEmpty() and isExp2(_board);
    }

    static constexpr uint64_t Universe     = 0xffffffffffffffff_ui64;
    static constexpr uint64_t Empty        = 0x0000000000000000_ui64;
    static constexpr uint64_t AFile        = 0x0101010101010101_ui64;
    static constexpr uint64_t BFile        = 0x0202020202020202_ui64;
    static constexpr uint64_t GFile        = 0x4040404040404040_ui64;
    static constexpr uint64_t HFile        = 0x8080808080808080_ui64;
    static constexpr uint64_t WhiteSquares = 0x55aa55aa55aa55aa_ui64;
    static constexpr uint64_t BlackSquares = ~WhiteSquares;
    static constexpr uint64_t NotAFile     = ~AFile;
    static constexpr uint64_t NotBFile     = ~BFile;
    static constexpr uint64_t NotGFile     = ~GFile;
    static constexpr uint64_t NotHFile     = ~HFile;
    static constexpr uint64_t NotABFiles   = NotAFile & NotBFile;
    static constexpr uint64_t NotGHFiles   = NotGFile & NotHFile;
private:
    uint64_t _board;
};

static_assert(sizeof(BitBoard) == 8);

_INLINE int BitBoard::popCount() const {
#if defined(__INTEL_COMPILER) or defined(_MSC_VER)
    return static_cast<int>(_mm_popcnt_u64(_board));
#elif defined(__GNUC__)
    return __builtin_popcountll(_board);
#else
    uint64_t bb = _board;
    int c;
    for (c = 0; bb; bb &= bb - 1, c++);
    return c;
#endif
}

#if defined(_MSC_VER) or defined(__INTEL_COMPILER) or defined(__GNUC__)

_INLINE int BitBoard::bitScanForward() const {
    assert(_board != BitBoard::Empty);
#if defined(_MSC_VER) or defined(__INTEL_COMPILER)
    unsigned long s;
    _BitScanForward64(&s, _board);
    return static_cast<int>(s);
#else
    return __builtin_ctzll(_board);
#endif
}

_INLINE int BitBoard::bitScanReverse() const {
    assert(_board != BitBoard::Empty);
#if defined(_MSC_VER) or defined(__INTEL_COMPILER)
    unsigned long s;
    _BitScanReverse64(&s, _board);
    return static_cast<int>(s);
#else
    return __builtin_clzll(_board);
#endif
}

#else // different compiler

// credits to:
//  https://www.chessprogramming.org/BitScan

static constexpr int BitScanIndex64[64] = {
    0, 47,  1, 56, 48, 27,  2, 60,
   57, 49, 41, 37, 28, 16,  3, 61,
   54, 58, 35, 52, 50, 42, 21, 44,
   38, 32, 29, 23, 17, 11,  4, 62,
   46, 55, 26, 59, 40, 36, 15, 53,
   34, 51, 20, 43, 31, 22, 10, 45,
   25, 39, 14, 33, 19, 30,  9, 24,
   13, 18,  8, 12,  7,  6,  5, 63
};

_INLINE int BitBoard::bitScanForward() const {
    static constexpr uint64_t debruijn64 = 0x03f79d71b4cb0a89_ui64;
    assert(_board != 0);
    return BitScanIndex64[((_board ^ (_board - 1)) * debruijn64) >> 58];
}

_INLINE int BitBoard::bitScanReverse() const {
    static constexpr uint64_t debruijn64 = 0x03f79d71b4cb0a89_ui64;
    uint64_t bb = _board;
    assert(_board != BitBoard::Empty);
    bb |= bb >> 1;
    bb |= bb >> 2;
    bb |= bb >> 4;
    bb |= bb >> 8;
    bb |= bb >> 16;
    bb |= bb >> 32;
    return BitScanIndex64[(bb * debruijn64) >> 58];
}

#endif

_INLINE BitBoard BitBoard::swapBytes() const {
#if defined(__GNUC__)
    return __builtin_bswap64(_board);
#elif defined(_MSC_VER)
    return _byteswap_uint64(_board);
#else
    return ((_board & 0x00000000000000FF_ui64) << 56) |
           ((_board & 0x000000000000FF00_ui64) << 40) |
           ((_board & 0x0000000000FF0000_ui64) << 24) |
           ((_board & 0x00000000FF000000_ui64) << 8)  |
           ((_board & 0x000000FF00000000_ui64) >> 8)  |
           ((_board & 0x0000FF0000000000_ui64) >> 24) |
           ((_board & 0x00FF000000000000_ui64) >> 40) |
           ((_board & 0xFF00000000000000_ui64) >> 56);
#endif
}

/* Rectangular lookup for in-between routines
*/
class RectangularTable {
public:
    RectangularTable(const RectangularTable&) = delete;
    RectangularTable(RectangularTable&&) = delete;

    RectangularTable& operator=(const RectangularTable&) = delete;
    RectangularTable& operator=(RectangularTable&&) = delete;

    static RectangularTable& get();
    static BitBoard inBetweenOnFly(Square org, Square dst);

    MultiArray<BitBoard, 64, 64> t64;
private:
    RectangularTable();
};

/* General setwise operations on BitBoard wrapper class - 
*  one step only and shifting routines
*/

namespace {

_INLINE BitBoard nortOne(BitBoard bb) {
    return bb << 8;
}

_INLINE BitBoard soutOne(BitBoard bb) {
    return bb >> 8;
}

_INLINE BitBoard westOne(BitBoard bb) {
    return (bb >> 1) & BitBoard::NotHFile;
}

_INLINE BitBoard eastOne(BitBoard bb) {
    return (bb << 1) & BitBoard::NotAFile;
}

_INLINE BitBoard noEaOne(BitBoard bb) {
    return (bb << 9) & BitBoard::NotAFile;
}

_INLINE BitBoard soEaOne(BitBoard bb) {
    return (bb >> 7) & BitBoard::NotAFile;
}

_INLINE BitBoard soWeOne(BitBoard bb) {
    return (bb >> 9) & BitBoard::NotHFile;
}

_INLINE BitBoard noWeOne(BitBoard bb) {
    return (bb << 7) & BitBoard::NotHFile;
}

_INLINE BitBoard noNoEa(BitBoard bb) {
    return (bb << 17) & BitBoard::NotAFile;
}

_INLINE BitBoard noEaEa(BitBoard bb) {
    return (bb << 10) & BitBoard::NotABFiles;
}

_INLINE BitBoard soEaEa(BitBoard bb) {
    return (bb >> 6) & BitBoard::NotABFiles;
}

_INLINE BitBoard soSoEa(BitBoard bb) {
    return (bb >> 15) & BitBoard::NotAFile;
}

_INLINE BitBoard soSoWe(BitBoard bb) {
    return (bb >> 17) & BitBoard::NotHFile;
}

_INLINE BitBoard soWeWe(BitBoard bb) {
    return (bb >> 10) & BitBoard::NotGHFiles;
}

_INLINE BitBoard noWeWe(BitBoard bb) {
    return (bb << 6) & BitBoard::NotGHFiles;
}

_INLINE BitBoard noNoWe(BitBoard bb) {
    return (bb << 15) & BitBoard::NotHFile;
}

BitBoard nortRay(Square sq) {
    return 0x0101010101010100_ui64 << sq;
}

BitBoard soutRay(Square sq) {
    return 0x0080808080808080_ui64 >> (sq ^ 63);
}

BitBoard westRay(Square sq) {
    return (1_ui64 << sq) - (1_ui64 << (sq & 56));
}

BitBoard eastRay(Square sq) {
    return 2 * ((1_ui64 << (sq | 7)) - (1_ui64 << sq));
}

BitBoard noEaRay(Square sq) {
    static constexpr BitBoard NotAFile = BitBoard::NotAFile,
                              NotABFile = NotAFile & (NotAFile << 9),
                              NotABCDFile = NotABFile & (NotABFile << 18);

    BitBoard bb(sq);
    bb |= (bb << 9) & NotAFile;
    bb |= (bb << 18) & NotABFile;
    bb |= (bb << 36) & NotABCDFile;
    return bb & ~BitBoard(sq);
}

BitBoard soEaRay(Square sq) {
    static constexpr BitBoard NotAFile = BitBoard::NotAFile,
                              NotABFile = NotAFile & (NotAFile >> 7),
                              NotABCDFile = NotABFile & (NotABFile >> 14);

    BitBoard bb(sq);
    bb |= (bb >> 7) & NotAFile;
    bb |= (bb >> 14) & NotABFile;
    bb |= (bb >> 28) & NotABCDFile;
    return bb ^ BitBoard(sq);
}

BitBoard soWeRay(Square sq) {
    static constexpr BitBoard NotHFile = BitBoard::NotHFile,
                              NotGHFile = NotHFile & (NotHFile >> 9),
                              NotEFGHFile = NotGHFile & (NotGHFile >> 18);

    BitBoard bb(sq);
    bb |= (bb >> 9) & NotHFile;
    bb |= (bb >> 18) & NotGHFile;
    bb |= (bb >> 36) & NotEFGHFile;
    return bb ^ BitBoard(sq);
}

BitBoard noWeRay(Square sq) {
    static constexpr BitBoard NotHFile = BitBoard::NotHFile,
                              NotGHFile = NotHFile & (NotHFile << 7),
                              NotEFGHFile = NotGHFile & (NotGHFile << 14);

    BitBoard bb(sq);
    bb |= (bb << 7) & NotHFile;
    bb |= (bb << 14) & NotGHFile;
    bb |= (bb << 28) & NotEFGHFile;
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

_FORCEINLINE BitBoard inBetween(Square org, Square dst) {
    assert(org.isValid() and dst.isValid());
    return RectangularTable::get().t64[org][dst];
}

// InBetween but without 'org' and 'dst' squares.
_FORCEINLINE BitBoard onlyBetween(Square org, Square dst) {
    assert(org.isValid() and dst.isValid());
    return RectangularTable::get().t64[org][dst] ^ BitBoard(org) ^ BitBoard(dst);
}

} // namespace
