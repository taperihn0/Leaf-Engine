#pragma once

#include "UtilsCommon.hpp"

#include <fstream>

namespace Utils {

class PackedPosition;

// Stockfish position compression.
class SfBinFormatPosition {
public:
    SfBinFormatPosition(); 
    explicit SfBinFormatPosition(const Position& pos);

    bool operator==(const SfBinFormatPosition& p) const;

    INLINE bool operator!=(const SfBinFormatPosition& p) const { return !(*this == p); }

    bool operator==(const PackedPosition& p) const;

    INLINE bool operator!=(const PackedPosition& p) const { return !(*this == p); }

    enum SpecialMasks : uint8_t {
        NO_SPECIAL = 0,
        EN_PASSANT_PAWN = 12,
        WHITE_ROOK_WITH_CASTLING = 13,
        BLACK_ROOK_WITH_CASTLING = 14,
        BLACK_KING_TO_MOVE = 15,
    };

    struct alignas(1) Nibble {
        INLINE Nibble() 
            : lo(0)
            , hi(0)
        {}

        INLINE Nibble(byte b)
            : lo(b & 0x0F)
            , hi((b & 0xF0) >> 4) 
        {}

        uint8_t lo : 4;
        uint8_t hi : 4;
    };

    static_assert(sizeof(Nibble) == 1);

    static SfBinFormatPosition SffromPacked(const PackedPosition& pack);

    static bool write(std::ostream& output, const SfBinFormatPosition& sfbin_pos);

    static bool read(std::istream& input, SfBinFormatPosition& sfbin_pos);

    static SfBinFormatPosition sfPacked(const Position& pos);

    static Position sfUnpacked(const SfBinFormatPosition& pos);

    static std::vector<SfBinFormatPosition> fullRead(std::istream& input);
protected:
    static constexpr int _MaxPiecesOnBoard = 32;
    static constexpr int _MaxNibbles       = _MaxPiecesOnBoard / 2;
    static constexpr int _SfBinBufferSize  = sizeof(BitBoard) + _MaxNibbles;

    BitBoard _occupancy_mask;
    Nibble   _pieces[_MaxNibbles];
    uint8_t  _piece_cnt;
};

// PackedPosition implements custom position compression.
// It extends Stockfish binpack format by halfmove and fullmove count.
class PackedPosition : public SfBinFormatPosition {
public:
    friend class SfBinFormatPosition;

    PackedPosition();
    explicit PackedPosition(const Position& pos);
    explicit PackedPosition(const SfBinFormatPosition& sfp);

    bool operator==(const PackedPosition& p) const;

    INLINE bool operator!=(const PackedPosition& p) const { return !(*this == p); }
    
    bool operator==(const SfBinFormatPosition& sfp) const;

    INLINE bool operator!=(const SfBinFormatPosition& sfp) const { return !(*this == sfp); }

    static PackedPosition fromFEN(const std::string& fen);

    static PackedPosition packed(const Position& pos);

    static Position unpacked(const PackedPosition& pack);

    static bool write(std::ostream& output, const PackedPosition& packed);

    static bool read(std::istream& input, PackedPosition& packed);

    static std::vector<PackedPosition> fullRead(std::istream& input);

    BitBoard getOccupancy() const;

    uint8_t getPieceCount() const;

    static Piece pieceFromMask(uint8_t mask, SpecialMasks& flags, Square sq);

    static PackedPosition fromSfPacked(const SfBinFormatPosition& sfp);
private:
    static uint8_t maskFromPiece(Piece piece, Square sq, const Position& pos);

    static void placeNextPieceFromNibble(Position& pos, BitBoard& occupied, uint8_t nibble_part);

    static constexpr int    _ClockBufferSize = 3;
    static constexpr size_t _PackedBufferSize = _SfBinBufferSize + _ClockBufferSize;

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
