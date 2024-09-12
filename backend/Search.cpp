#include "Search.hpp"
#include "Eval.hpp"
#include "MoveGen.hpp"

std::string Score::toStr() const {
	if (_raw > Score::infinity - (int16_t)max_depth)
		return "mate " + std::to_string((Score::infinity - _raw + 1) / 2);
	else if (_raw < -Score::infinity + (int16_t)max_depth)
		return "mate -" + std::to_string((_raw + Score::infinity + 1) / 2);

	return "cp " + std::to_string(_raw);
}

INLINE void SearchResults::clearPV() {
	for (auto& ply_line : pv_line)
		ply_line.fill(Move::null);
}

INLINE void SearchResults::printBestMove() {
	ASSERT(!pv_line[0][0].isNull(), "Null bestmove error");

	std::cout << "bestmove ";
	pv_line[0][0].print();
	std::cout << '\n';
}

INLINE void SearchResults::print() {
	const auto duration_ms = timer.duration();
	const uint64_t nps = static_cast<uint64_t>((nodes_cnt * 1000.f) / (duration_ms ? duration_ms : 1));

	std::cout << "info depth " << depth
		<< " score " << score_cp.toStr()
		<< " nodes " << nodes_cnt
		<< " time " << duration_ms 
		<< " nps " << nps 
		<< " pv ";

	int it = 0;
	while (!pv_line[0][it].isNull())
		pv_line[0][it++].print(), std::cout << ' ';

	std::cout << '\n';
}

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
	results.timer.go();
	negaMax<true>(pos, results, -Score::infinity, Score::infinity, depth, 0);
	results.timer.stop();
	results.print();
}

template <bool Root>
Score Search::negaMax(Position& pos, SearchResults& results, Score alpha, Score beta, unsigned depth, unsigned ply) {
	results.nodes_cnt++;

	if (!depth) {
		return quiesce(pos, results, alpha, beta);
	} 

	const bool check = pos.isInCheck(pos.getTurn());

	assert(0 < depth and depth < max_depth);
	assert(alpha < beta);
	
	TreeNodeInfo& node = _tree.getNode(ply);
	node.moves.clear();
	node.legals_cnt = 0;

	while (node.moves.nextMove(pos, node.move)) {
		bool legal_move = false;

		if (pos.make(node.move, node.state)) {
			legal_move = true;
			node.legals_cnt++;
			node.score = -negaMax<false>(pos, results, -beta, -alpha, depth - 1, ply + 1);
		}

		pos.unmake(node.move, node.state);

		if (legal_move and node.score > alpha) {
			alpha = node.score;

			// fail hard
			if (node.score >= beta) break;
			
			results.pv_line[ply][0] = node.move;
			std::copy(results.pv_line[ply + 1].data(), results.pv_line[ply + 1].data() + depth - 1, 
				results.pv_line[ply].data() + 1);
		}
	}

	// detect checkmate or stealmate
	if (!node.legals_cnt) {
		return check ? -Score::infinity + ply : Score::draw;
	}
	// Fifty-move rule draw
	else if (pos.halfmoveClock() >= 100) {
		return Score::draw;
	}

	if constexpr (Root) {
		results.depth = depth;
		results.score_cp = alpha;
	}

	return alpha;
}

template Score Search::negaMax<true>(Position& pos, SearchResults& results, Score alpha, Score beta, unsigned depth, unsigned ply);
template Score Search::negaMax<false>(Position& pos, SearchResults& results, Score alpha, Score beta, unsigned depth, unsigned ply);

Score Search::quiesce(Position& pos, SearchResults& results, Score alpha, Score beta) {
	results.nodes_cnt++;

	assert(alpha < beta);

	const Score stand_pat = _eval.staticEval(pos);
	
	// standing pat cutoff
	if (stand_pat > alpha) {
		if (stand_pat >= beta) return beta;
		alpha = stand_pat;
	}

	MoveOrder<QUIESCENT> moves;
	Position::IrreversibleState state;
	Move move;
	Score score;

	moves.generateMoves(pos);
	while (moves.nextMove(pos, move)) {
		bool legal_move = false;
			
		if (pos.make(move, state)) {
			legal_move = true;
			score = -quiesce(pos, results, -beta, -alpha);
		}

		pos.unmake(move, state);

		if (legal_move and score > alpha) {
			// fail hard
			if (score >= beta) return beta;
			alpha = score;
		}
	}

	return alpha;
}