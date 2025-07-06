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

static constexpr std::array<int, 6> piece_value = {
	100, 300, 300, 500, 900, 10000
};

void MoveList::scoreCaptures(size_t first, const Position& pos) {
	for (size_t i = first; i < _idx; i++) {
		assert(_moves[i].move.isCapture() or (_moves[i].move.isPromotion()
			and _moves[i].move.getPromoPieceT() == Piece::QUEEN));

		if (_moves[i].move.isCapture()) {
			const Piece::enumType att = _moves[i].move.getPerformerT();
			const Piece::enumType vic = _moves[i].move.isEnPassant() ? Piece::PAWN : 
										pos.pieceTypeOn(_moves[i].move.getTarget(), pos.getOppositeTurn());

			if (piece_value[att] <= piece_value[vic])
				_moves[i].score = mvv_lva[att][vic];
			else
				_moves[i].score = pos.StaticExchangeEval(_moves[i].move.getOrigin(), _moves[i].move.getTarget(), vic, att);

		}
		else _moves[i].score = 2000;
	}
}

void MoveList::scoreQuiets(size_t first, const Position& pos, int16_t (*history)[6][64]) {
	for (size_t i = first; i < _idx; i++) {
		assert(_moves[i].move.isQuiet());
		_moves[i].score = history[pos.getTurn()][_moves[i].move.getPerformerT()][_moves[i].move.getTarget()];
	}
}

void MoveList::selectSort(size_t first) {
	assert(first < _idx);
	int16_t best = _moves[first].score;
	size_t ind = first;

	for (size_t i = first + 1; i < _idx; i++) {
		if (_moves[i].score > best) {
			best = _moves[i].score;
			ind = i;
		}
	}

	std::swap(_moves[ind], _moves[first]);
}