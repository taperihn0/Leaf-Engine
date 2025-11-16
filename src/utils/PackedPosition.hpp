#pragma once

#include "backend/Position.hpp"

#include <fstream>

namespace Utils {
    
class PackedPosition {
public:
    PackedPosition();

    bool operator==(const PackedPosition& p) const;

    INLINE bool operator!=(const PackedPosition& p) const { return !(*this == p); }
    
    static PackedPosition fromFEN(const std::string& fen);

    static PackedPosition packed(const Position& pos);

    static Position unpacked(const PackedPosition& pack);

    void write(std::ostream& output) const;

    static bool read(std::istream& input, PackedPosition& packed);

    static std::vector<PackedPosition> fullRead(std::istream& input);

    struct alignas(1) Nibble {
        uint8_t lo : 4;
        uint8_t hi : 4;
    };

    static_assert(sizeof(Nibble) == 1);

    struct DetailData {
        Nibble pieces[16];
        uint8_t halfmove_clock;
        uint16_t fullmove_clock;
    };

    enum SpecialMasks : uint8_t {
        NO_SPECIAL = 0,
        EN_PASSANT_PAWN = 12,
        WHITE_ROOK_WITH_CASTLING = 13,
        BLACK_ROOK_WITH_CASTLING = 14,
        BLACK_KING_TO_MOVE = 15,
    };

    BitBoard getOccupancy() const;

    DetailData getDetailData() const;

    static Piece pieceFromMask(uint8_t mask, SpecialMasks& flags, Square sq);
private:
    static uint8_t maskFromPiece(Piece piece, Square sq, const Position& pos);

    static void placeNextPieceFromNibble(Position& pos, BitBoard& occupied, uint8_t nibble_part);

    static constexpr size_t _PackedSize = 8 + 19;

    BitBoard _occupancy_mask;
    DetailData _details_mask;

    uint8_t _piece_cnt;
};

} // namespace Utils
