#include "Search.hpp"
#include "Eval.hpp"
#include "MoveGen.hpp"
#include "Time.hpp"
#include "TranspositionTable.hpp"

#include <sstream>

std::string Score::toStr() const {
	if (_raw > Score::infinity - (int16_t)max_depth)
		return "mate " + std::to_string((Score::infinity - _raw + 1) / 2);
	else if (_raw < -Score::infinity + (int16_t)max_depth)
		return "mate -" + std::to_string((_raw + Score::infinity + 1) / 2);

	return "cp " + std::to_string(_raw);
}

INLINE bool SearchLimits::isTimeLeft() {
	timer.stop();
	return !search_time or timer.duration() < search_time;
}

INLINE void SearchResults::registerBestMove() {
	bestmove = pv_line[0][0];
}

INLINE void SearchResults::clearPV() {
	for (auto& ply_line : pv_line)
		ply_line.fill(Move::null);
}

INLINE void SearchResults::printBestMove() {
	ASSERT(!bestmove.isNull(), "Null bestmove error");

	std::cout << "bestmove ";
	bestmove.print();
	std::cout << '\n';
}

Search::Search(TranspositionTable&& tt)
	: _tt(tt) {}

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

void Search::bestMove(Position& pos, const Game& game, SearchLimits limits) {
	ASSERT(1 <= limits.depth and limits.depth < max_depth, "Invalid depth");

	limits.timer.go();
	limits.search_time = TimeMan::searchTime(pos, limits);

	iterativeDeepening(pos, game, limits);
}

void Search::iterativeDeepening(Position& pos, const Game& game, SearchLimits& limits) {
	SearchResults search_results;

	for (unsigned d = 1; d <= limits.depth; d++) {
		search_results.nodes_cnt = 0;
		search_results.depth = d;

		if (!search(pos, game, limits, search_results))
			break;

		search_results.registerBestMove();
	}

	search_results.printBestMove();
}

bool Search::search(Position& pos, const Game& game, SearchLimits& limits, SearchResults& results) {
	results.timer.go();

	const Score score 
		= negaMax<true>(pos, limits, results, game, -Score::infinity, Score::infinity, results.depth, 0);

	if (results.depth > 1 and score == Score::undef)
		return false;

	results.timer.stop();
	results.print();

	return true;
}

template <bool Root>
Score Search::negaMax(Position& pos, SearchLimits& limits, SearchResults& results, const Game& game, 
	Score alpha, Score beta, unsigned depth, unsigned ply) {
	if constexpr (!Root) {

		// Fifty-move rule or repetition draw
		if (pos.halfmoveClock() >= 100 or isRepetitionCycle(pos, game, ply)) {
			return Score::draw;
		}
		else if ((results.nodes_cnt & _check_node_count) == 0 and !limits.isTimeLeft()) {
			return Score::undef;
		}

		const auto& [is_valid, tt_score] = _tt.probe(pos.getZobristKey(), alpha, beta, depth, ply);

		if (is_valid) {
			return tt_score;
		}

		if (!depth) {
			return quiesce(pos, limits, results, alpha, beta);
		}
	}

	results.nodes_cnt++;

	const bool check = pos.isInCheck(pos.getTurn());

	ASSERT(0 < depth and depth < max_depth, "Depth overflow");
	assert(alpha < beta);
	
	TreeNodeInfo& node = _tree.getNode(ply);

	node.moves.clear();
	node.legals_cnt = 0;
	node.score = 0;

	TTEntry::Bound bound_type = TTEntry::LOWERBOUND;

	while (node.moves.nextMove(pos, node.move)) {
		bool legal_move = false;

		if (pos.make(node.move, node.state)) {
			legal_move = true;
			node.legals_cnt++;
			node.score = -negaMax<false>(pos, limits, results, game, -beta, -alpha, depth - 1, ply + 1);
		}

		pos.unmake(node.move, node.state);

		if (node.score == -Score::undef)
			return Score::undef;
		else if (legal_move and node.score > alpha) {
			if (node.score >= beta) {
				bound_type = TTEntry::UPPERBOUND;
				alpha = beta;
				break;
			}

			bound_type = TTEntry::EXACT;
			alpha = node.score;

			results.pv_line[ply][0] = node.move;
			std::copy(results.pv_line[ply + 1].data(), results.pv_line[ply + 1].data() + depth - 1, 
				results.pv_line[ply].data() + 1);
		}
	}
	
	// detect checkmate or stealmate
	if (!node.legals_cnt) {
		bound_type = TTEntry::EXACT;
		alpha = check ? -Score::infinity + ply : Score::draw;
	}

	_tt.write(pos.getZobristKey(), depth, ply, bound_type, alpha);

	if constexpr (Root) {
		results.score_cp = alpha;
	}

	return alpha;
}

template Score Search::negaMax<true>(Position& pos, SearchLimits& limits, SearchResults& results, const Game& game,
	Score alpha, Score beta, unsigned depth, unsigned ply);
template Score Search::negaMax<false>(Position& pos, SearchLimits& limits, SearchResults& results, const Game& game,
	Score alpha, Score beta, unsigned depth, unsigned ply);

Score Search::quiesce(Position& pos, SearchLimits& limits, SearchResults& results, Score alpha, Score beta) {
	if ((results.nodes_cnt & _check_node_count) == 0 and !limits.isTimeLeft()) {
		return Score::undef;
	}

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
	Score score = 0;

	moves.generateMoves(pos);
	while (moves.nextMove(pos, move)) {
		bool legal_move = false;
			
		if (pos.make(move, state)) {
			legal_move = true;
			score = -quiesce(pos, limits, results, -beta, -alpha);
		}

		pos.unmake(move, state);

		if (score == -Score::undef)
			return Score::undef;
		else if (legal_move and score > alpha) {
			// fail hard
			if (score >= beta) return beta;
			alpha = score;
		}
	}

	return alpha;
}

bool Search::isRepetitionCycle(const Position& pos, const Game& game, int ply) {
	const int my_ply = ply;
	const uint64_t my_hashkey = pos.getZobristKey();

	for (ply = ply - 1; ply >= 0; ply--) {
		const Move move = _tree.getNode(ply).move;

		if (move.isCapture() or move.getPerformerT() == Piece::PAWN)
			return false;
		else if ((my_ply - ply) % 2 == 1)
			continue;

		const uint64_t prev_hashkey = _tree.getNode(ply).state.hash_key;
		if (my_hashkey == prev_hashkey)
			return true;
	}

	
	const int my_cnt = static_cast<int>(game.currentHalfCount());

	if (!my_cnt) return false;

	for (int cnt = my_cnt - 1; cnt >= my_cnt - 5 and cnt >= 0; cnt--) {
		const Move move = game.getPrevMove(cnt);

		if (move.isCapture() or move.getPerformerT() == Piece::PAWN)
			return false;
		else if ((my_cnt - cnt) % 2 == 0)
			continue;

		const uint64_t prev_hashkey = game.getPrevKey(cnt);
		if (my_hashkey == prev_hashkey)
			return true;
	}

	return false;
}