#include "PackedPosition.hpp"

namespace Utils
{

template <typename PositionFormat>
std::vector<PositionFormat> fullReadOf(std::istream& input) {
    static_assert(_IS_SAME_TYPE(PositionFormat, PackedPosition) or
                  _IS_SAME_TYPE(PositionFormat, ExtPackedPosition));

    std::vector<PositionFormat> res;

    ASSERT(input, "Invalid input stream");

    PositionFormat pack;

    // TODO: Optimize that 
    while (PositionFormat::read(input, pack)) {
        res.push_back(pack);
    }

    return res;
}

PackedPosition::PackedPosition() {
    std::memset(reinterpret_cast<void*>(this), 0, sizeof(PackedPosition));
}

PackedPosition::PackedPosition(const Position& pos) {
    *this = packed(pos);
}

bool PackedPosition::operator==(const PackedPosition& p) const {
    if (_occupancy_mask != p._occupancy_mask)
        return false;

    size_t cmp_bytes = static_cast<size_t>((_piece_cnt + 1) / 2);

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
    
    const size_t piece_bytes = static_cast<size_t>((_piece_cnt + 1) / 2);

    for (size_t j = 0; j < piece_bytes; j++) {
        os << (_pieces[j].lo & 0x0F) << ' ' 
           << (_pieces[j].hi & 0xF0) << '\n';
    }

    os << std::endl;
}

PackedPosition PackedPosition::fromExt(const ExtPackedPosition& ext_pack) {
    PackedPosition pack;

    pack._occupancy_mask = ext_pack._occupancy_mask;
    pack._piece_cnt = ext_pack._piece_cnt;

    std::memcpy(reinterpret_cast<void*>(&pack._pieces), 
                reinterpret_cast<const void*>(&ext_pack._pieces), 
                (pack._piece_cnt + 1) / 2);

    return pack;
}

bool PackedPosition::write(std::ostream& output, const PackedPosition& pos) {
    assert(output);

    byte mem[_PackedPosBufferSize];
    *reinterpret_cast<BitBoard*>(mem) = pos._occupancy_mask;

    size_t piece_bytes = static_cast<size_t>((pos._piece_cnt + 1) / 2);
    size_t j = 0;

    for (; j < piece_bytes; j++) {
        mem[j + sizeof(BitBoard)] = *reinterpret_cast<const byte*>(&pos._pieces[j]);
    }

    output.write(reinterpret_cast<const char*>(mem), j + sizeof(BitBoard));
    return output.good();
}

bool PackedPosition::writeStatic(std::ostream& output, const PackedPosition& pos) {
    assert(output);

    byte mem[_PackedPosBufferSize];
    *reinterpret_cast<BitBoard*>(mem) = pos._occupancy_mask;

    for (size_t j = 0; j < _MaxNibbles; j++) {
        mem[j + sizeof(BitBoard)] = *reinterpret_cast<const byte*>(&pos._pieces[j]);
    }

    output.write(reinterpret_cast<const char*>(mem), _PackedPosBufferSize);
    return output.good();
}

bool PackedPosition::read(std::istream& input, PackedPosition& pos) {
    assert(input);

    size_t bytes_left = streamBytesLeft(input);

    if (!bytes_left)
        return false;

    assert(bytes_left >= sizeof(BitBoard));

    uint64_t occupied;
    input.read(reinterpret_cast<char*>(&occupied), sizeof(BitBoard));

    pos._occupancy_mask = occupied;

    size_t piece_cnt = pos._occupancy_mask.popCount();
    pos._piece_cnt = static_cast<uint8_t>(piece_cnt);

    size_t piece_bytes = static_cast<size_t>((piece_cnt + 1) / 2);

    assert(bytes_left - sizeof(BitBoard) >= piece_bytes);

    Nibble piece_mem[_MaxNibbles];

    input.read(reinterpret_cast<char*>(piece_mem), piece_bytes);

    for (size_t i = 0; i < piece_bytes; i++) {
        pos._pieces[i] = piece_mem[i];
    }

    return true;
}

bool PackedPosition::readStatic(std::istream& input, PackedPosition& pos) {
    assert(input);

    size_t bytes_left = streamBytesLeft(input);

    if (!bytes_left)
        return false;

    assert(bytes_left >= sizeof(BitBoard));

    uint64_t occupied;
    input.read(reinterpret_cast<char*>(&occupied), sizeof(BitBoard));

    pos._occupancy_mask = occupied;

    size_t piece_cnt = pos._occupancy_mask.popCount();
    pos._piece_cnt = static_cast<uint8_t>(piece_cnt);

    assert(bytes_left - sizeof(BitBoard) >= _MaxNibbles);

    input.read(reinterpret_cast<char*>(&pos._pieces), _MaxNibbles);

    return input.good();
}

std::vector<PackedPosition> PackedPosition::fullRead(std::istream& input) {
    return fullReadOf<PackedPosition>(input);
}

std::pair<Square, Square> PackedPosition::getKingsSquares() const {
    BitBoard occ = _occupancy_mask;

    Square ksq[2];
    SpecialMasks flags;

    enumColor side2move = WHITE; // may be BLACK, but we will see

    for (size_t j = 0; occ > 0; j++) {
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

			if (file == Square::a) 
				pos._castling_rights[color].setQueenSide(true);
			else if (file == Square::h)
				pos._castling_rights[color].setKingSide(true);
			else
				assert(false);

			break;
		}
		case ExtPackedPosition::BLACK_KING_TO_MOVE:
			pos._turn = BLACK;
			break;
		case ExtPackedPosition::NO_SPECIAL: break;
	}

	pos._piece_bb[color][piece_type].setBit(square);
}

Position ExtPackedPosition::unpacked(const ExtPackedPosition& pack) {
	Position pos;
	BitBoard occupied = pack.getOccupancy();

	pos._ep_square = Square::None;
	pos._turn = WHITE;

	pos._castling_rights[WHITE].clear();
	pos._castling_rights[BLACK].clear();

	for (size_t i = 0; occupied and i < _MaxNibbles; i++) {
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

	pos._zhash = ZobristHash::generateOnFly(pos);

	return pos;
}

bool ExtPackedPosition::write(std::ostream& output, const ExtPackedPosition& pack) {
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

bool ExtPackedPosition::read(std::istream& input, ExtPackedPosition& packed) {
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

std::vector<ExtPackedPosition> ExtPackedPosition::fullRead(std::istream& input) {
    return fullReadOf<ExtPackedPosition>(input);
}

} // namespace Utils

