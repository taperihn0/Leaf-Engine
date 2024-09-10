#include "Search.hpp"
#include "Eval.hpp"
#include "MoveOrder.hpp"
#include "MoveGen.hpp"

void Search::bestMove(Position& pos, unsigned depth) {
	ASSERT(1 <= depth and depth < max_depth, "Invalid depth");
	iterativeDeepening(pos, depth);
}

void Search::iterativeDeepening(Position& pos, unsigned depth) {
	SearchResults search_results;

	for (unsigned d = 1; d <= depth; d++) {
		search_results.nodes_cnt = 0;
		search(pos, d, search_results);
	}

	search_results.printBestMove();
}

void Search::search(Position& pos, unsigned depth, SearchResults& results) {
	negaMax<true>(pos, results, -Score::infinity, Score::infinity, depth, 0);
	results.print();
}

template <bool Root>
Score Search::negaMax(Position& pos, SearchResults& results, Score alpha, Score beta, unsigned depth, unsigned ply) {
	results.nodes_cnt++;

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
			score = -negaMax<false>(pos, results, -beta, -alpha, depth - 1, ply + 1);
		}

		pos.unmake(move, state);

		if (legal_move and score > alpha) {
			if (score >= beta) return beta;
			
			results.pv_line[ply][0] = move;
			std::copy(results.pv_line[ply + 1].data(), results.pv_line[ply + 1].data() + depth - 1, 
				results.pv_line[ply].data() + 1);
			alpha = score;
		}
	}

	if constexpr (Root) 
		results.score_cp = alpha;

	return alpha;
}

template Score Search::negaMax<true>(Position& pos, SearchResults& results, Score alpha, Score beta, unsigned depth, unsigned ply);
template Score Search::negaMax<false>(Position& pos, SearchResults& results, Score alpha, Score beta, unsigned depth, unsigned ply);