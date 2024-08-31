#include "MoveGen.hpp"

template <MoveGen::enumMode GenType>
void MoveGen::generatePseudoLegalMoves(const Position pos, MoveList& move_list) {
	// TODO
}

template void MoveGen::generatePseudoLegalMoves<MoveGen::CAPTURES>(const Position, MoveList&);
template void MoveGen::generatePseudoLegalMoves<MoveGen::QUIETS>(const Position, MoveList&);