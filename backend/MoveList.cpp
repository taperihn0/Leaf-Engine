#include "MoveList.hpp"

static constexpr std::array<int, 6> piece_value = {
	100, 300, 300, 500, 900, 10000
};

void MoveList::scoreCaptures(size_t first, const Position& pos) {
	for (size_t i = first; i < _idx; i++) {
		assert(_moves[i].move.isCapture() or (_moves[i].move.isPromotion()
			   and _moves[i].move.getPromoPieceT() == Piece::QUEEN));

		if (_moves[i].move.isEnPassant()) {
			_moves[i].score = piece_value[Piece::PAWN] - value(Piece::PAWN);
		}
		else if (_moves[i].move.isCapture()) {
			const Piece::enumType att = _moves[i].move.getPerformerT();
			const Piece::enumType vic = pos.pieceTypeOn(_moves[i].move.getTarget(), pos.getOppositeTurn());
			_moves[i].score = piece_value[vic] - value(att);
		}

		if (_moves[i].move.isPromotion()) {
			_moves[i].score += piece_value[Piece::QUEEN];
		}
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
