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

#include "PackedPosition.hpp"

namespace utils
{

template <typename PositionFormat>
std::vector<PositionFormat> fullReadOf(std::istream& input) {
    static_assert(is_same<PositionFormat, PackedPosition> or
                  is_same<PositionFormat, ExtPackedPosition>);

    std::vector<PositionFormat> res;

    ASSERT(input, "Invalid input stream");

    // TODO: Optimize that 
    for (PositionFormat pack; PositionFormat::read(input, pack); ) {
        res.push_back(pack);
    }

    return res;
}

PackedPosition::PackedPosition() {
    mem::memSet(reinterpret_cast<void*>(this), 0, sizeof(PackedPosition));
}

PackedPosition::PackedPosition(const Position& pos) {
    *this = packed(pos);
}

bool PackedPosition::operator==(const PackedPosition& p) const {
    if (_occupancy_mask != p._occupancy_mask)
        return false;

    std::size_t cmp_bytes = static_cast<std::size_t>((_piece_cnt + 1) / 2);

    return !static_cast<bool>(std::memcmp(&_pieces, &p._pieces, cmp_bytes));
}

bool PackedPosition::operator==(const ExtPackedPosition& p) const {
    return *this == fromExt(p);
}

void PackedPosition::print(std::ostream& os) const {
    os << "Piece count: " << static_cast<int>(_piece_cnt) << '\n';
    os << "Occupancy:\n";

    _occupancy_mask.print(os);
    
    os << "\nNibbles:\n";
    
    const std::size_t piece_bytes = static_cast<std::size_t>((_piece_cnt + 1) / 2);

    for (std::size_t j = 0; j < piece_bytes; j++) {
        os << (_pieces[j].lo & 0x0F) << ' ' 
           << (_pieces[j].hi & 0xF0) << '\n';
    }

    os << std::endl;
}

PackedPosition PackedPosition::fromExt(const ExtPackedPosition& ext_pack) {
    PackedPosition pack;

    pack._occupancy_mask = ext_pack._occupancy_mask;
    pack._piece_cnt = ext_pack._piece_cnt;

    mem::memCopy(reinterpret_cast<void*>(&pack._pieces), 
            reinterpret_cast<const void*>(&ext_pack._pieces), 
            (pack._piece_cnt + 1) / 2);

    return pack;
}

bool PackedPosition::write(std::ostream& output, const PackedPosition& pos) {
    assert(output);

    MultiArray<std::byte, _PackedPosBufferSize> mem;
    *reinterpret_cast<BitBoard*>(mem.data()) = pos._occupancy_mask;

    std::size_t piece_bytes = static_cast<std::size_t>((pos._piece_cnt + 1) / 2);
    std::size_t j = 0;

    for (; j < piece_bytes; j++) {
        mem[j + sizeof(BitBoard)] = *reinterpret_cast<const std::byte*>(&pos._pieces[j]);
    }

    output.write(reinterpret_cast<const char*>(mem.data()), j + sizeof(BitBoard));
    return output.good();
}

bool PackedPosition::writeStatic(std::ostream& output, const PackedPosition& pos) {
    assert(output);

    MultiArray<std::byte, _PackedPosBufferSize> mem;
    *reinterpret_cast<BitBoard*>(mem.data()) = pos._occupancy_mask;

    for (std::size_t j = 0; j < MaxNibbles; j++) {
        mem[j + sizeof(BitBoard)] = *reinterpret_cast<const std::byte*>(&pos._pieces[j]);
    }

    output.write(reinterpret_cast<const char*>(mem.data()), _PackedPosBufferSize);
    return output.good();
}

bool PackedPosition::read(std::istream& input, PackedPosition& pos) {
    assert(input);

    std::size_t bytes_left = getIStreamBytesLeft(input);

    if (!bytes_left)
        return false;

    assert(bytes_left >= sizeof(BitBoard));

    uint64_t occupied;
    input.read(reinterpret_cast<char*>(&occupied), sizeof(BitBoard));

    pos._occupancy_mask = occupied;

    std::size_t piece_cnt = pos._occupancy_mask.popCount();
    pos._piece_cnt = static_cast<uint8_t>(piece_cnt);

    std::size_t piece_bytes = static_cast<std::size_t>((piece_cnt + 1) / 2);

    assert(bytes_left - sizeof(BitBoard) >= piece_bytes);

    MultiArray<Nibble, MaxNibbles> piece_mem;

    input.read(reinterpret_cast<char*>(piece_mem.data()), piece_bytes);

    for (std::size_t i = 0; i < piece_bytes; i++) {
        pos._pieces[i] = piece_mem[i];
    }

    return true;
}

bool PackedPosition::readStatic(std::istream& input, PackedPosition& pos) {
    assert(input);

    std::size_t bytes_left = getIStreamBytesLeft(input);

    if (!bytes_left)
        return false;

    assert(bytes_left >= sizeof(BitBoard));

    uint64_t occupied;
    input.read(reinterpret_cast<char*>(&occupied), sizeof(BitBoard));

    pos._occupancy_mask = occupied;

    std::size_t piece_cnt = pos._occupancy_mask.popCount();
    pos._piece_cnt = static_cast<uint8_t>(piece_cnt);

    assert(bytes_left - sizeof(BitBoard) >= MaxNibbles);

    input.read(reinterpret_cast<char*>(&pos._pieces), MaxNibbles);

    return input.good();
}

std::vector<PackedPosition> PackedPosition::fullRead(std::istream& input) {
    return fullReadOf<PackedPosition>(input);
}

std::pair<Square, Square> PackedPosition::getKingsSquares() const {
    BitBoard occ = _occupancy_mask;

    MultiArray<Square, 2> ksq;
    SpecialMasks flags;

    enumColor side2move = WHITE; // may be BLACK, but we will see

    for (std::size_t j = 0; occ > 0; j++) {
        Square sq = static_cast<Square>(occ.dropForward());

        uint8_t mask = j % 2 ? _pieces[j / 2].hi : _pieces[j / 2].lo;

        Piece pc = pieceFromMask(mask, flags, sq);

        if (pc.type() == Piece::KING)
            ksq[pc.color()] = sq;

        if (flags == BLACK_KING_TO_MOVE)
            side2move = BLACK;
    }

    return std::make_pair(ksq[side2move], ksq[!side2move]);
}

Square PackedPosition::getKingSquare() const {
    return getKingsSquares().first;
}

Square PackedPosition::getOppKingSquare() const {
    return getKingsSquares().second;
}

BitBoard PackedPosition::getOccupancy() const {
    return _occupancy_mask;
}

uint8_t PackedPosition::getPieceCount() const {
    return _piece_cnt;
}

Turn PackedPosition::getTurn() const {
    bool white_to_move = true;
    const std::size_t piece_cnt = getPieceCount();

    for (std::size_t i = 0; i < piece_cnt; i++) {
        if (_pieces[i].lo == BLACK_KING_TO_MOVE) {
            white_to_move = false;
            break;
        }

        if (++i < piece_cnt) {
            if (_pieces[i].hi == BLACK_KING_TO_MOVE) {
                white_to_move = false;
                break;
            }   
        }
    }

    return Turn(static_cast<enumColor>(white_to_move));
}

MultiArray<PackedPosition::Nibble, PackedPosition::MaxNibbles> PackedPosition::getNibbles() const {
    return _pieces;
}

#define ROOK_WITH_CASTLING(color) (WHITE_ROOK_WITH_CASTLING + color)

Piece PackedPosition::pieceFromMask(uint8_t mask, PackedPosition::SpecialMasks& flags, Square sq) {
    Piece piece;

    switch (mask) {
    case EN_PASSANT_PAWN:
        flags = EN_PASSANT_PAWN;
        piece.set(sq.getRank() == Square::RANK_4 ? WHITE : BLACK, Piece::PAWN);
        break;
    case WHITE_ROOK_WITH_CASTLING:
        flags = WHITE_ROOK_WITH_CASTLING;
        piece.set(WHITE, Piece::ROOK);
        break;
    case BLACK_ROOK_WITH_CASTLING:
        flags = BLACK_ROOK_WITH_CASTLING;
        piece.set(BLACK, Piece::ROOK);
        break;
    case BLACK_KING_TO_MOVE:
        flags = BLACK_KING_TO_MOVE;
        piece.set(BLACK, Piece::KING);
        break;
    default: 
        flags = NO_SPECIAL;
        enumColor col = static_cast<enumColor>(mask / 6);
        Piece::enumType type = static_cast<Piece::enumType>(mask % 6);
        piece.set(col, type);
        break;
    }

    return piece;
}

PackedPosition PackedPosition::packed(const Position& pos) {
    return fromExt(ExtPackedPosition::packed(pos));
}

Position PackedPosition::unpacked(const PackedPosition& pos) {
    return ExtPackedPosition::unpacked(ExtPackedPosition::fromPacked(pos));
}

ExtPackedPosition::ExtPackedPosition() 
    : PackedPosition() {

    _clock_data.fullmove = 0;
    _clock_data.halfmove = 0;
}

ExtPackedPosition::ExtPackedPosition(const Position& pos) {
    *this = packed(pos);
}

ExtPackedPosition::ExtPackedPosition(const PackedPosition& pos) 
    : PackedPosition(pos) {

    _clock_data.fullmove = 0;
    _clock_data.halfmove = 0;
}

bool ExtPackedPosition::operator==(const ExtPackedPosition& p) const {
    return PackedPosition::operator==(p)
       and p._clock_data.fullmove == p._clock_data.fullmove
       and p._clock_data.halfmove == p._clock_data.halfmove;
}

bool ExtPackedPosition::operator==(const PackedPosition& pos) const {
    return PackedPosition::fromExt(*this) == pos;
}

ExtPackedPosition ExtPackedPosition::fromFEN(const std::string& fen) {
    return packed(Position(fen));
}

ExtPackedPosition ExtPackedPosition::packed(const Position& pos) {
    ExtPackedPosition packed;

    BitBoard occupied = pos.getOccupied();
    packed._occupancy_mask = occupied;

    packed._piece_cnt = 0;

    for (std::size_t i = 0; occupied and i < MaxNibbles; i++) {
        Nibble nibble;
        nibble.lo = 0;
        nibble.hi = 0;

        uint8_t sq = occupied.dropForward();
        Piece piece = pos.fullPieceOn(sq);
        uint8_t mask = maskFromPiece(piece, sq, pos);

        nibble.lo = mask;

        packed._piece_cnt++;
        
        if (occupied) {
            sq = occupied.dropForward();
            piece = pos.fullPieceOn(sq);
            mask = maskFromPiece(piece, sq, pos);

            nibble.hi = mask;

            packed._piece_cnt++;
        }

        packed._pieces[i] = nibble;
    }

    packed._clock_data.halfmove = pos.getHalfmoveClock();
    packed._clock_data.fullmove = pos.getFullmoveClock();

    return packed;
}

ExtPackedPosition ExtPackedPosition::fromPacked(const PackedPosition& pos) {
    return static_cast<ExtPackedPosition>(pos);
}

uint8_t ExtPackedPosition::maskFromPiece(Piece piece, Square sq, const Position& pos) {
    uint8_t mask = 0;
    
    enumColor color = piece.color();

    if (piece.type() == Piece::PAWN and 
        (sq.getRank() == Square::RANK_4 or sq.getRank() == Square::RANK_5)
        and Square(sq + (color ? 8 : -8)) == pos.getEnPassantSq()) {
        mask = EN_PASSANT_PAWN;
        return mask;
    }
    else if (piece.type() == Piece::KING and color == BLACK and pos.getTurn() == BLACK) {
        mask = BLACK_KING_TO_MOVE;
        return mask;
    }
    else if (piece.type() == Piece::ROOK) {
        Square::enumFile rook_file = sq.getFile();

        if (rook_file == Square::FILE_H and pos.getCastlingByColor(color).isShortPossible())
            mask = ROOK_WITH_CASTLING(color);
        else if (rook_file == Square::FILE_A and pos.getCastlingByColor(color).isLongPossible())
            mask = ROOK_WITH_CASTLING(color);
        else mask = piece.value() + 6 * color;

        return mask;
    }
    
    mask = piece.value() + 6 * color;
    return mask;
}

void ExtPackedPosition::placeNextPieceFromNibble(Position& pos, BitBoard& occupied, uint8_t nibble_part) {
    Square square = occupied.dropForward();
    
    ExtPackedPosition::SpecialMasks piece_flags;
    Piece piece = ExtPackedPosition::pieceFromMask(nibble_part, piece_flags, square);

    enumColor color = piece.color();
    Piece::enumType piece_type = piece.type();

    switch (piece_flags) {
    case ExtPackedPosition::EN_PASSANT_PAWN: {
        Square ep_sq = square + (color ? 8 : -8);
        pos._ep_square = ep_sq;
        break;
    }
    case ExtPackedPosition::WHITE_ROOK_WITH_CASTLING:
    case ExtPackedPosition::BLACK_ROOK_WITH_CASTLING: {
        Square::enumFile file = square.getFile();

        if (file == Square::FILE_A) 
            pos._castling_rights[color].setQueenSide(true);
        else if (file == Square::FILE_H)
            pos._castling_rights[color].setKingSide(true);
        else
            assert(false);

        break;
    }
    case ExtPackedPosition::BLACK_KING_TO_MOVE:
        pos._s2m = BLACK;
        break;
    case ExtPackedPosition::NO_SPECIAL: 
        break;
    }

    pos._piece_bb[color][piece_type].setBit(square);
}

Position ExtPackedPosition::unpacked(const ExtPackedPosition& pack) {
    Position pos;
    BitBoard occupied = pack.getOccupancy();

    pos._ep_square = Square::None;
    pos._s2m = WHITE;

    pos._castling_rights[WHITE].clear();
    pos._castling_rights[BLACK].clear();

    for (std::size_t i = 0; occupied and i < MaxNibbles; i++) {
        ExtPackedPosition::Nibble nibble = pack._pieces[i];

        placeNextPieceFromNibble(pos, occupied, nibble.lo);

        if (occupied) 
            placeNextPieceFromNibble(pos, occupied, nibble.hi);
    }

    pos._halfmove_count = pack._clock_data.halfmove;
    pos._fullmove_count = pack._clock_data.fullmove;

    pos._occupied[WHITE] = pos.getBySideOnFly(WHITE);
    pos._occupied[BLACK] = pos.getBySideOnFly(BLACK);

    pos._king_sq[WHITE] = pos.getKingBySide(WHITE).bitScanForward();
    pos._king_sq[BLACK] = pos.getKingBySide(BLACK).bitScanForward();

    pos._zhash = ZHash::generateOnFly(pos);

    return pos;
}

bool ExtPackedPosition::write(std::ostream& output, const ExtPackedPosition& pack) {
    assert(output);

    MultiArray<std::byte, _PackedBufferSize> mem;
    *reinterpret_cast<BitBoard*>(mem.data()) = pack._occupancy_mask;
    
    std::size_t piece_bytes = static_cast<std::size_t>((pack._piece_cnt + 1) / 2);
    std::size_t j = 0;

    for (; j < piece_bytes; j++) {
        mem[j + sizeof(BitBoard)] = *reinterpret_cast<const std::byte*>(&pack._pieces[j]);
    }

    mem[j + sizeof(BitBoard)] = std::byte(pack._clock_data.halfmove);
    *reinterpret_cast<uint16_t*>(mem.data() + j + sizeof(BitBoard) + 1) = pack._clock_data.fullmove;

    output.write(reinterpret_cast<const char*>(mem.data()), j + sizeof(BitBoard) + 3);
    return output.good();
}

bool ExtPackedPosition::read(std::istream& input, ExtPackedPosition& packed) {
    assert(input);

    std::size_t bytes_left = getIStreamBytesLeft(input);

    if (!bytes_left)
        return false;

    assert(bytes_left >= sizeof(BitBoard));
    
    uint64_t occupied;
    input.read(reinterpret_cast<char*>(&occupied), sizeof(BitBoard));

    packed._occupancy_mask = occupied;

    std::size_t piece_cnt = packed._occupancy_mask.popCount();
    packed._piece_cnt = static_cast<uint8_t>(piece_cnt);

    std::size_t piece_bytes = static_cast<std::size_t>((piece_cnt + 1) / 2);

    assert(bytes_left - sizeof(BitBoard) >= piece_bytes + _ClockBufferSize);

    MultiArray<std::byte, MaxNibbles + _ClockBufferSize> details_mem;
    input.read(reinterpret_cast<char*>(details_mem.data()), piece_bytes + _ClockBufferSize);

    std::size_t i = 0;

    for (; i < piece_bytes; i++) {
        packed._pieces[i] = static_cast<Nibble>(std::to_integer<uint8_t>(details_mem[i]));
    }

    packed._clock_data.halfmove = std::to_integer<uint8_t>(details_mem[i]);
    packed._clock_data.fullmove = *reinterpret_cast<uint16_t*>(details_mem.data() + i + 1);

    return input.good();
}

std::vector<ExtPackedPosition> ExtPackedPosition::fullRead(std::istream& input) {
    return fullReadOf<ExtPackedPosition>(input);
}

} // namespace utils

