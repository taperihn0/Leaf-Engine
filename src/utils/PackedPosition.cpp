#include "PackedPosition.hpp"

namespace Utils
{

PackedPosition PackedPosition::fromFEN(const std::string& fen) {
    return packed(Position(fen));
}

PackedPosition PackedPosition::packed(const Position& pos) {
    PackedPosition packed;

    BitBoard occupied = pos.getOccupied();
    packed._occupancy_mask = occupied;

    for (size_t i = 0; occupied and i < 16; i++) {
        Nibble nibble;
        nibble.lo = 0;
        nibble.hi = 0;

        uint8_t sq = occupied.dropForward();
        Piece piece = pos.fullPieceOn(sq);
        uint8_t mask = maskFromPiece(piece, sq, pos);

        nibble.lo = mask;
        
        if (occupied) {
            sq = occupied.dropForward();
            piece = pos.fullPieceOn(sq);
            mask = maskFromPiece(piece, sq, pos);

            nibble.hi = mask;
        }

        packed._details_mask.pieces[i] = nibble;
    }

    packed._details_mask.halfmove_clock = pos.halfmoveClock();
    packed._details_mask.fullmove_clock = pos.fullmoveClock();
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
				ASSERT(false, "Invalid castling rights");

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
	BitBoard occupied = pack.getOccupancy();
	PackedPosition::DetailData details = pack.getDetailData();

	Position pos;

	pos._ep_square = Square::None;
	pos._turn = WHITE;

	pos._castling_rights[WHITE].clear();
	pos._castling_rights[BLACK].clear();

	for (size_t i = 0; i < 16 and occupied; i++) {
		PackedPosition::Nibble nibble = details.pieces[i];

		placeNextPieceFromNibble(pos, occupied, nibble.lo);

		if (occupied) 
			placeNextPieceFromNibble(pos, occupied, nibble.hi);
	}

	pos._halfmove_count = details.halfmove_clock;
	pos._fullmove_count = details.fullmove_clock;

	pos._occupied[WHITE] = pos.getBySideOnFly(WHITE);
	pos._occupied[BLACK] = pos.getBySideOnFly(BLACK);

	pos._king_sq[WHITE] = pos.getKingBySide(WHITE).bitScanForward();
	pos._king_sq[BLACK] = pos.getKingBySide(BLACK).bitScanForward();

	pos._hashing.set(pos._hashing.generateOnFly(pos));

	return pos;
}

BitBoard PackedPosition::getOccupancy() const {
    return _occupancy_mask;
}

PackedPosition::DetailData PackedPosition::getDetailData() const {
    return _details_mask;
}

} // namespace Utils

