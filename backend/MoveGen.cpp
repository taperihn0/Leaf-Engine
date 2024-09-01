#include "MoveGen.hpp"
#include "Attacks.hpp"

template <Piece::enumType Piece, enumColor Side, bool Capture> 
void generate(const Position& pos, MoveList& move_list, BitBoard mask) {
	static_assert(Piece != Piece::PAWN and Piece != Piece::NONE and Piece != Piece::KING, 
		"Unsupported piecetype in generate func template");

	BitBoard pieces = pos.get<Piece, Side>();

	while (pieces) {
		Square org = Square(pieces.dropForward());
		BitBoard att = attacks<Piece>(org, pos.getOccupied()) & mask;

		while (att) {
			Square dst = Square(att.dropForward());
			move_list.push(Move::makeSimple(org, dst, Capture));
		}
	}
}

template <MoveGen::enumMode GenType, enumColor Side>
void generateByColor(const Position& pos, MoveList& move_list) {
	static const BitBoard own_pieces = Side == WHITE ? pos.getWhites() : pos.getBlacks(),
						  enemy_pieces = pos.getOccupied() ^ own_pieces,
						  mask = GenType == MoveGen::QUIETS ? pos.getEmpties() : pos.getOccupied();
	static constexpr bool Capture = GenType != MoveGen::QUIETS;

	generate<Piece::KNIGHT, Side, Capture>(pos, move_list, mask);
	generate<Piece::BISHOP, Side, Capture>(pos, move_list, mask);
	generate<Piece::ROOK, Side, Capture> (pos, move_list, mask);
	generate<Piece::QUEEN, Side, Capture>(pos, move_list, mask);
}

template <MoveGen::enumMode GenType>
void MoveGen::generatePseudoLegalMoves(const Position& pos, MoveList& move_list) {
	static_cast<enumColor>(pos.getTurn()) == WHITE ? 
		generateByColor<GenType, WHITE>(pos, move_list) :
		generateByColor<GenType, BLACK>(pos, move_list);
}

template void MoveGen::generatePseudoLegalMoves<MoveGen::CAPTURES>(const Position&, MoveList&);
template void MoveGen::generatePseudoLegalMoves<MoveGen::TACTICALS>(const Position&, MoveList&);
template void MoveGen::generatePseudoLegalMoves<MoveGen::QUIETS>(const Position&, MoveList&);