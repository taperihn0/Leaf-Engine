#include "PackedPosition.hpp"

namespace Utils
{

template <typename PositionFormat>
std::vector<PositionFormat> fullReadOf(std::istream& input) {
    static_assert(_IS_SAME_TYPE(PositionFormat, SfBinFormatPosition) or
                  _IS_SAME_TYPE(PositionFormat, PackedPosition));

    std::vector<PositionFormat> res;

    ASSERT(input, "Invalid input stream");

    PositionFormat pack;

    // TODO: Optimize that 
    while (PositionFormat::read(input, pack)) {
        res.push_back(pack);
    }

    return res;
}

SfBinFormatPosition::SfBinFormatPosition() {
    std::memset(reinterpret_cast<void*>(this), 0, sizeof(SfBinFormatPosition));
}

SfBinFormatPosition::SfBinFormatPosition(const Position& pos) {
    *this = sfPacked(pos);
}

bool SfBinFormatPosition::operator==(const SfBinFormatPosition& p) const {
    if (_occupancy_mask != p._occupancy_mask)
        return false;

    size_t cmp_bytes = static_cast<size_t>((_piece_cnt + 1) / 2);

    return !static_cast<bool>(std::memcmp(&_pieces, &p._pieces, cmp_bytes));
}

bool SfBinFormatPosition::operator==(const PackedPosition& p) const {
    return *this == SffromPacked(p);
}

SfBinFormatPosition SfBinFormatPosition::SffromPacked(const PackedPosition& pack) {
    SfBinFormatPosition sfpack;

    sfpack._occupancy_mask = pack._occupancy_mask;
    sfpack._piece_cnt = pack._piece_cnt;

    std::memcpy(reinterpret_cast<void*>(&sfpack._pieces), 
                reinterpret_cast<const void*>(&pack._pieces), 
                (sfpack._piece_cnt + 1) / 2);

    return sfpack;
}

bool SfBinFormatPosition::write(std::ostream& output, const SfBinFormatPosition& sfbin_pos) {
    assert(output);

    byte mem[_SfBinBufferSize];
    *reinterpret_cast<BitBoard*>(mem) = sfbin_pos._occupancy_mask;

    size_t piece_bytes = static_cast<size_t>((sfbin_pos._piece_cnt + 1) / 2);
    size_t j = 0;

    for (; j < piece_bytes; j++) {
        mem[j + sizeof(BitBoard)] = *reinterpret_cast<const byte*>(&sfbin_pos._pieces[j]);
    }

    output.write(reinterpret_cast<const char*>(mem), j + sizeof(BitBoard));
    return output.good();
}

bool SfBinFormatPosition::read(std::istream& input, SfBinFormatPosition& sfbin_pos) {
    assert(input);

    size_t bytes_left = streamBytesLeft(input);

    if (!bytes_left)
        return false;

    assert(bytes_left >= sizeof(BitBoard));

    uint64_t occupied;
    input.read(reinterpret_cast<char*>(&occupied), sizeof(BitBoard));

    sfbin_pos._occupancy_mask = occupied;

    size_t piece_cnt = sfbin_pos._occupancy_mask.popCount();
    sfbin_pos._piece_cnt = static_cast<uint8_t>(piece_cnt);

    size_t piece_bytes = static_cast<size_t>((piece_cnt + 1) / 2);

    assert(bytes_left - sizeof(BitBoard) >= piece_bytes);

    Nibble piece_mem[_MaxNibbles];

    input.read(reinterpret_cast<char*>(piece_mem), piece_bytes);

    for (size_t i = 0; i < piece_bytes; i++) {
        sfbin_pos._pieces[i] = piece_mem[i];
    }

    return true;
}

std::vector<SfBinFormatPosition> SfBinFormatPosition::fullRead(std::istream& input) {
    return fullReadOf<SfBinFormatPosition>(input);
}

SfBinFormatPosition SfBinFormatPosition::sfPacked(const Position& pos) {
    return SffromPacked(PackedPosition::packed(pos));
}

Position SfBinFormatPosition::sfUnpacked(const SfBinFormatPosition& sfp) {
    return PackedPosition::unpacked(PackedPosition::fromSfPacked(sfp));
}

PackedPosition::PackedPosition() 
    : SfBinFormatPosition() {

    _clock_data.fullmove = 0;
    _clock_data.halfmove = 0;
}

PackedPosition::PackedPosition(const Position& pos) {
    *this = packed(pos);
}

PackedPosition::PackedPosition(const SfBinFormatPosition& sfp) 
    : SfBinFormatPosition(sfp) {

    _clock_data.fullmove = 0;
    _clock_data.halfmove = 0;
}

bool PackedPosition::operator==(const PackedPosition& p) const {
    return SfBinFormatPosition::operator==(p)
       and p._clock_data.fullmove == p._clock_data.fullmove
       and p._clock_data.halfmove == p._clock_data.halfmove;
}

bool PackedPosition::operator==(const SfBinFormatPosition& sfp) const {
    return SfBinFormatPosition::SffromPacked(*this) == sfp;
}

PackedPosition PackedPosition::fromFEN(const std::string& fen) {
    return packed(Position(fen));
}

PackedPosition PackedPosition::packed(const Position& pos) {
    PackedPosition packed;

    BitBoard occupied = pos.getOccupied();
    packed._occupancy_mask = occupied;

    packed._piece_cnt = 0;

    for (size_t i = 0; occupied and i < _MaxNibbles; i++) {
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

    packed._clock_data.halfmove = pos.halfmoveClock();
    packed._clock_data.fullmove = pos.fullmoveClock();

    return packed;
}

#define ROOK_WITH_CASTLING(color) (WHITE_ROOK_WITH_CASTLING + color)

Piece PackedPosition::pieceFromMask(uint8_t mask, PackedPosition::SpecialMasks& flags, Square sq) {
    Piece piece;

    switch (mask) {
    case EN_PASSANT_PAWN:
        flags = EN_PASSANT_PAWN;
        piece.set(sq.getRank() == Square::r4 ? WHITE : BLACK, Piece::PAWN);
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

PackedPosition PackedPosition::fromSfPacked(const SfBinFormatPosition& sfp) {
    return static_cast<PackedPosition>(sfp);
}

uint8_t PackedPosition::maskFromPiece(Piece piece, Square sq, const Position& pos) {
    uint8_t mask = 0;
    
    enumColor color = piece.color();

    if (piece.type() == Piece::PAWN and (sq.getRank() == Square::r4 or sq.getRank() == Square::r5)
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

        if (rook_file == Square::h and pos.getCastlingByColor(color).isShortPossible())
            mask = ROOK_WITH_CASTLING(color);
        else if (rook_file == Square::a and pos.getCastlingByColor(color).isLongPossible())
            mask = ROOK_WITH_CASTLING(color);
        else mask = piece.value() + 6 * color;

        return mask;
    }
    
    mask = piece.value() + 6 * color;
    return mask;
}

void PackedPosition::placeNextPieceFromNibble(Position& pos, BitBoard& occupied, uint8_t nibble_part) {
		Square square = occupied.dropForward();
		
		PackedPosition::SpecialMasks piece_flags;
		Piece piece = PackedPosition::pieceFromMask(nibble_part, piece_flags, square);

		enumColor color = piece.color();
		Piece::enumType piece_type = piece.type();

		switch (piece_flags) {
		case PackedPosition::EN_PASSANT_PAWN: {
			Square ep_sq = square + (color ? 8 : -8);
			pos._ep_square = ep_sq;
			break;
		}
		case PackedPosition::WHITE_ROOK_WITH_CASTLING:
		case PackedPosition::BLACK_ROOK_WITH_CASTLING: {
			Square::enumFile file = square.getFile();

			if (file == Square::a) 
				pos._castling_rights[color].setQueenSide(true);
			else if (file == Square::h)
				pos._castling_rights[color].setKingSide(true);
			else
				assert(false);

			break;
		}
		case PackedPosition::BLACK_KING_TO_MOVE:
			pos._turn = BLACK;
			break;
		case PackedPosition::NO_SPECIAL: break;
	}

	pos._piece_bb[color][piece_type].setBit(square);
}

Position PackedPosition::unpacked(const PackedPosition& pack) {
	Position pos;
	BitBoard occupied = pack.getOccupancy();

	pos._ep_square = Square::None;
	pos._turn = WHITE;

	pos._castling_rights[WHITE].clear();
	pos._castling_rights[BLACK].clear();

	for (size_t i = 0; occupied and i < _MaxNibbles; i++) {
		PackedPosition::Nibble nibble = pack._pieces[i];

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

	pos._zhash = ZobristHash::generateOnFly(pos);

	return pos;
}

bool PackedPosition::write(std::ostream& output, const PackedPosition& pack) {
    assert(output);

    byte mem[_PackedBufferSize];
    *reinterpret_cast<BitBoard*>(mem) = pack._occupancy_mask;
    
    size_t piece_bytes = static_cast<size_t>((pack._piece_cnt + 1) / 2);
    size_t j = 0;

    for (; j < piece_bytes; j++) {
        mem[j + sizeof(BitBoard)] = *reinterpret_cast<const byte*>(&pack._pieces[j]);
    }

    mem[j + sizeof(BitBoard)] = pack._clock_data.halfmove;
    *reinterpret_cast<uint16_t*>(mem + j + sizeof(BitBoard) + 1) = pack._clock_data.fullmove;

    output.write(reinterpret_cast<const char*>(mem), j + sizeof(BitBoard) + 3);
    return output.good();
}

bool PackedPosition::read(std::istream& input, PackedPosition& packed) {
    assert(input);

    size_t bytes_left = streamBytesLeft(input);

    if (!bytes_left)
        return false;

    assert(bytes_left >= sizeof(BitBoard));
    
    uint64_t occupied;
    input.read(reinterpret_cast<char*>(&occupied), sizeof(BitBoard));

    packed._occupancy_mask = occupied;

    size_t piece_cnt = packed._occupancy_mask.popCount();
    packed._piece_cnt = static_cast<uint8_t>(piece_cnt);

    size_t piece_bytes = static_cast<size_t>((piece_cnt + 1) / 2);

    assert(bytes_left - sizeof(BitBoard) >= piece_bytes + _ClockBufferSize);

    byte details_mem[_MaxNibbles + _ClockBufferSize];
    input.read(reinterpret_cast<char*>(details_mem), piece_bytes + _ClockBufferSize);

    size_t i = 0;

    for (; i < piece_bytes; i++) {
        packed._pieces[i] = static_cast<Nibble>(details_mem[i]);
    }

    packed._clock_data.halfmove = details_mem[i];
    packed._clock_data.fullmove = *reinterpret_cast<uint16_t*>(details_mem + i + 1);

    return input.good();
}

std::vector<PackedPosition> PackedPosition::fullRead(std::istream& input) {
    return fullReadOf<PackedPosition>(input);
}

BitBoard PackedPosition::getOccupancy() const {
    return _occupancy_mask;
}

uint8_t PackedPosition::getPieceCount() const {
    return _piece_cnt;
}

} // namespace Utils

