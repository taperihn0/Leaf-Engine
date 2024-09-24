#include "MoveList.hpp"

// MVV-LVA table taken directly from Austerlitz:
// https://github.com/taperihn0/Austerlitz-Engine/blob/master/source/MoveOrder.h
static constexpr std::array<std::array<int, 5>, 6> mvv_lva = { {
	{ 1050, 2050, 3050, 4050, 5050 },
	{ 1040, 2040, 3040, 4040, 5040 },
	{ 1038, 2038, 3038, 4038, 5038 },
	{ 1015, 2015, 3015, 4015, 5015 },
	{ 1009, 2009, 3009, 4009, 5009 },
	{ 1000, 2000, 3000, 4000, 5000 }
} };

void MoveList::scoreCaptures(size_t first, const Position& pos) {
	for (size_t i = first; i < _idx; i++) {
		assert(_moves[i].move.isCapture() or (_moves[i].move.isPromotion() 
			and _moves[i].move.getPromoPieceT() == Piece::QUEEN));

		if (_moves[i].move.isCapture()) {
			const Piece::enumType att = _moves[i].move.getPerformerT(),
				vic = _moves[i].move.isEnPassant() ? Piece::PAWN : 
			pos.pieceTypeOn(_moves[i].move.getTarget(), pos.getOppositeTurn());

			_moves[i].score = mvv_lva[att][vic];
		}
		else _moves[i].score = 2000;
	}
}