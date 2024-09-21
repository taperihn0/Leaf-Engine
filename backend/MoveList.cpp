#include "MoveList.hpp"

// MVV-LVA table taken directly from Austerlitz:
// https://github.com/taperihn0/Austerlitz-Engine/blob/master/source/MagicBitBoards.h
static constexpr std::array<std::array<int, 5>, 6> mvv_lva = { {
	{ 1050, 2050, 3050, 4050, 5050 },
	{ 1040, 2040, 3040, 4040, 5040 },
	{ 1038, 2038, 3038, 4038, 5038 },
	{ 1015, 2015, 3015, 4015, 5015 },
	{ 1009, 2009, 3009, 4009, 5009 },
	{ 1000, 2000, 3000, 4000, 5000 }
} };

void MoveList::scoreCaptures(size_t first, const Position& pos) {
	for (; first < _idx; first++) {
		assert(_moves[first].move.isCapture() or (_moves[first].move.isPromotion() 
			and _moves[first].move.getPromoPieceT() == Piece::QUEEN));

		if (_moves[first].move.isCapture()) {
			const Piece::enumType att = _moves[first].move.getPerformerT(),
				vic = pos.pieceTypeOn(_moves[first].move.getTarget(), pos.getOppositeTurn());

			_moves[first].score = mvv_lva[att][vic];
		}
		else _moves[first].score = 2000;
	}
}