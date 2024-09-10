#include "Search.hpp"
#include "Eval.hpp"
#include "MoveOrder.hpp"
#include "MoveGen.hpp"

void Search::bestMove(Position& pos, unsigned depth) {
	iterativeDeepening(pos, depth);

	std::cout << "bestmove ";
	_pv_line[0][0].print();
	std::cout << '\n';
}

void Search::iterativeDeepening(Position& pos, unsigned depth) {
	ASSERT(1 <= depth and depth < max_depth, "Invalid depth");
	clearPV();

	for (unsigned d = 1; d <= depth; d++) {
		search(pos, d);
	}
}

void Search::search(Position& pos, unsigned depth) {
	const Score score_cp = negaMax(pos, -Score::infinity, Score::infinity, depth, 0);
	assert(!_pv_line[0][0].isNull());

	std::cout << "info score cp " << score_cp.toInt() << " pv ";

	int it = 0;
	while (!_pv_line[0][it].isNull())
		_pv_line[0][it++].print(), std::cout << ' ';

	std::cout << '\n';
}

Score Search::negaMax(Position& pos, Score alpha, Score beta, unsigned depth, unsigned ply) {
	if (!depth) {
		return staticEval(pos);
	}

	assert(0 < depth and depth < max_depth);
	assert(alpha < beta);

	MoveOrder<PLAIN> moves;
	Position::IrreversibleState state;
	Move move;

	while (moves.nextMove(pos, move)) {
		Score score;
		bool legal_move = false;

		if (pos.make(move, state)) {
			legal_move = true;
			score = -negaMax(pos, -beta, -alpha, depth - 1, ply + 1);
		}

		pos.unmake(move, state);

		if (legal_move and score > alpha) {
			if (score >= beta) return beta;
			
			_pv_line[ply][0] = move;
			std::copy(_pv_line[ply + 1].data(), _pv_line[ply + 1].data() + depth - 1, _pv_line[ply].data() + 1);
			alpha = score;
		}
	}

	return alpha;
}