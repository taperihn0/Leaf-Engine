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

#include "UtilsCommon.hpp"

#include <fstream>

namespace Utils {

class ExtPackedPosition;

// Default position compression.
class PackedPosition {
public:
    PackedPosition(); 
    explicit PackedPosition(const Position& pos);

    bool operator==(const PackedPosition& p) const;

    _INLINE bool operator!=(const PackedPosition& p) const { return !(*this == p); }

    bool operator==(const ExtPackedPosition& p) const;

    _INLINE bool operator!=(const ExtPackedPosition& p) const { return !(*this == p); }

    enum SpecialMasks : uint8_t {
        NO_SPECIAL = 0,
        EN_PASSANT_PAWN = 12,
        WHITE_ROOK_WITH_CASTLING = 13,
        BLACK_ROOK_WITH_CASTLING = 14,
        BLACK_KING_TO_MOVE = 15,
    };

    struct alignas(1) Nibble {
        _INLINE Nibble() 
            : lo(0)
            , hi(0)
        {}

        _INLINE Nibble(uint8_t b)
            : lo(b & 0x0F)
            , hi((b & 0xF0) >> 4) 
        {}

        uint8_t lo : 4;
        uint8_t hi : 4;
    };

    static_assert(sizeof(Nibble) == 1);

    void print(std::ostream& os = std::cout) const;

    static PackedPosition fromExt(const ExtPackedPosition& ext_pack);

    /* 'write' method is default, optimized writing method.
    *  It writes only needed piece nibbles.
    */
    static bool write(std::ostream& output, const PackedPosition& pos);

    /* 'writeStatic' is unoptimized, but handy.
    *  It writes full occupancy mask and full nibble buffer (8 bytes + 16 bytes).
    */
    static bool writeStatic(std::ostream& output, const PackedPosition& pos);

    /* 'read' overwrites current position from optimized input (see 'write').
    */
    static bool read(std::istream& input, PackedPosition& pos);

    /* 'readStatic' overwrites current position from static input (see 'writeStatic')
    */
    static bool readStatic(std::istream& input, PackedPosition& pos);

    static PackedPosition packed(const Position& pos);

    static Position unpacked(const PackedPosition& pos);

    static std::vector<PackedPosition> fullRead(std::istream& input);

    // result is a pair: { king_sq, opp_king_sq }
    std::pair<Square, Square> getKingsSquares() const;
    Square getKingSquare() const;
    Square getOppKingSquare() const;

    BitBoard getOccupancy() const;

    uint8_t getPieceCount() const;

    static Piece pieceFromMask(uint8_t mask, SpecialMasks& flags, Square sq);
protected:
    static constexpr int _MaxPiecesOnBoard = 32;
    static constexpr int _MaxNibbles       = _MaxPiecesOnBoard / 2;
    static constexpr int _PackedPosBufferSize = sizeof(BitBoard) + _MaxNibbles;

    BitBoard                     _occupancy_mask;
    array1d<Nibble, _MaxNibbles> _pieces;
    uint8_t                      _piece_cnt;
};

// ExtPackedPosition implements custom position compression.
// It extends by halfmove and fullmove count.
class ExtPackedPosition : public PackedPosition {
public:
    friend class PackedPosition;

    ExtPackedPosition();
    explicit ExtPackedPosition(const Position& pos);
    explicit ExtPackedPosition(const PackedPosition& sfp);

    bool operator==(const ExtPackedPosition& p) const;
    _INLINE bool operator!=(const ExtPackedPosition& p) const { return !(*this == p); }
    bool operator==(const PackedPosition& sfp) const;
    _INLINE bool operator!=(const PackedPosition& sfp) const { return !(*this == sfp); }

    static ExtPackedPosition fromFEN(const std::string& fen);
    static ExtPackedPosition packed(const Position& pos);
    static Position unpacked(const ExtPackedPosition& pack);
    static bool write(std::ostream& output, const ExtPackedPosition& packed);
    static bool read(std::istream& input, ExtPackedPosition& packed);
    static std::vector<ExtPackedPosition> fullRead(std::istream& input);
    static ExtPackedPosition fromPacked(const PackedPosition& sfp);
private:
    static uint8_t maskFromPiece(Piece piece, Square sq, const Position& pos);
    static void placeNextPieceFromNibble(Position& pos, BitBoard& occupied, uint8_t nibble_part);

    static constexpr int    _ClockBufferSize = 3;
    static constexpr size_t _PackedBufferSize = _PackedPosBufferSize + _ClockBufferSize;

    // apart from 16-byte pieces buffer,
    // we also store fullmove count and fullmove count
    // as a so called clock data.
    struct ClockData {
        uint16_t fullmove;
        uint8_t  halfmove;
    };

    ClockData _clock_data;
};

} // namespace Utils
