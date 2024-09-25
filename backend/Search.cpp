#include "Search.hpp"
#include "Eval.hpp"
#include "MoveGen.hpp"
#include "Time.hpp"
#include "TranspositionTable.hpp"

#include <sstream>

INLINE bool SearchLimits::isTimeLeft() {
	timer.stop();
	return !search_time or timer.duration() < search_time;
}

INLINE void SearchResults::registerBestMove(Move move) {
	best_move = move;
}

INLINE void SearchResults::printBestMove() {
	ASSERT(!best_move.isNull(), "Null bestmove error");

	std::cout << "bestmove ";
	best_move.print();
	std::cout << '\n';
}

Search::Search(TranspositionTable&& tt)
	: _tt(tt) {}

INLINE void SearchResults::print(const Search* search, const Position& pos) {
	const auto duration_ms = timer.duration();
	const uint64_t nps = static_cast<uint64_t>((nodes_cnt * 1000.f) / (duration_ms ? duration_ms : 1));

	std::cout << "info depth " << depth
		<< " seldepth " << seldepth
		<< " score " << score_cp.toStr()
		<< " nodes " << nodes_cnt
		<< " time " << duration_ms 
		<< " nps " << nps 
		<< " hashfull " << static_cast<unsigned>(static_cast<float>(tt_hits) / tt_entries * 1000)
		<< " pv ";

	Position cpy = pos;

	while (depth--) {
		TTEntry tt_entry;
		const bool tt_hit = search->_tt.probe(tt_entry, cpy.getZobristKey(), -Score::infinity, +Score::infinity, depth, 0);

		Move pv_move = tt_entry.move;

		if (!tt_hit or pv_move.isNull()) 
			break;

		pv_move.print(), std::cout << ' ';

		Position::IrreversibleState tmp;
		cpy.make(pv_move, tmp);
	}

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
	search_results.tt_entries = _tt.getEntriesCount();

	for (unsigned d = 1; d <= limits.depth; d++) {
		search_results.nodes_cnt = 0;
		search_results.depth = d;

		_tree.clear();

		if (!search(pos, game, limits, search_results))
			break;

		search_results.registerBestMove(_tree.getNode(0).best_move);
	}

	search_results.printBestMove();
}

bool Search::search(Position& pos, const Game& game, SearchLimits& limits, SearchResults& results) {
	results.timer.go();

	const Score score 
		= -negaMax<true>(pos, limits, results, game, -Score::infinity, +Score::infinity, results.depth, 0);

	if (results.depth > 1 and !score.isValid())
		return false;

	results.timer.stop();
	results.print(this, pos);

	return true;
}

template <bool Root, Search::enumNode NodeType, bool NullMove>
Score Search::negaMax(Position& pos, SearchLimits& limits, SearchResults& results, const Game& game, 
	Score alpha, Score beta, unsigned depth, unsigned ply) {
	if constexpr (!Root) {
		if (pos.halfmoveClock() >= 100 or isRepetitionCycle(pos, game, ply)) {
			return Score::draw;
		}
		else if ((results.nodes_cnt & _check_node_count) == 0 and !limits.isTimeLeft()) {
			return -Score::undef;
		}
		else if (!depth) {
			return quiesce(pos, limits, results, alpha, beta, ply);
		}
	}

	TTEntry tt_entry;
	const bool tt_hit = _tt.probe(tt_entry, pos.getZobristKey(), alpha, beta, depth, ply);

	if (!Root and tt_hit) {
		return tt_entry.score;
	}

	results.nodes_cnt++;

	NodeInfo& node = _tree.getNode(ply);

	node.check = pos.isInCheck(pos.getTurn());

	if constexpr (NullMove) {
		static constexpr int R = 2;

		if (!node.check and depth >= R + 1) {
			pos.makeNull(node.state);
			const Score score =
				-negaMax<false, NON_PV_NODE, false>(pos, limits, results, game, -beta, -beta + 1, depth - R - 1, ply + 1);
			pos.unmakeNull(node.state);

			if (score >= beta) {
				const Score verify = 
					negaMax<false, NON_PV_NODE, false>(pos, limits, results, game, beta - 1, beta, depth - R - 1, ply);

				if (verify >= beta)
					return verify;
			}
		}
	}


	ASSERT(0 < depth and depth < max_depth, "Depth overflow");
	assert(alpha < beta);

	const Move tt_move = tt_entry.key == pos.getZobristKey() 
						 and tt_entry.move.isPseudoLegal(pos) ? tt_entry.move : Move::null;

	node.move_picker.clear();
	node.move_picker.setHashMove(tt_move);

	node.can_move = false;
	node.score = 0;
	node.best_move = Move::null;
	node.best_score = -Score::infinity;

	TTEntry::Bound bound_type = TTEntry::LOWERBOUND;

	while (node.move_picker.nextMove(pos, node.move)) {
		bool legal_move = false,
			 do_search = true;

		if (pos.make(node.move, node.state)) {
			legal_move = true;
			node.can_move = true;

			// Principle variation search
			if (!tt_move.isNull() and node.move != tt_move and NodeType == PV_NODE) {
				node.score = 
					-negaMax<false, PV_NODE, true>(pos, limits, results, game, -alpha - 1, -alpha, depth - 1, ply + 1);

				if (node.score <= alpha)
					do_search = false;
			} 

			if (do_search)
				node.score =
					-negaMax<false, NodeType, true>(pos, limits, results, game, -beta, -alpha, depth - 1, ply + 1);
		}

		pos.unmake(node.move, node.state);

		if (node.score.isValid() and legal_move and node.score > node.best_score) {
			node.best_move = node.move;
			node.best_score = node.score;

			if (node.score > alpha) {
				if (node.score >= beta) {
					bound_type = TTEntry::UPPERBOUND;
					if (node.move.isQuiet() and (!node.move.isPromotion() or node.move.getPromoPieceT() != Piece::QUEEN))
						node.move_picker.setKillerMove(node.move);
					break;
				}

				bound_type = TTEntry::EXACT;
				alpha = node.score;
			}
		}
		else if (!node.score.isValid()) {
			if (Root and node.best_move.isNull())
				// TODO: move at root assigned here might be illegal.
				node.best_move = node.move;

			return -Score::undef;
		}
	}
	
	// detect checkmate or stealmate
	if (!node.can_move) {
		bound_type = TTEntry::EXACT;
		node.best_score = node.check ? -Score::infinity + ply : Score::draw;
	}

	_tt.write(pos.getZobristKey(), depth, ply, bound_type, node.best_score, node.best_move, results);

	_tree.getNode(ply + 1).move_picker.setKillerMove(Move::null);

	if constexpr (Root) {
		results.score_cp = node.best_score;
	}

	return node.best_score;
}

template Score Search::negaMax<true>(Position& pos, SearchLimits& limits, SearchResults& results, const Game& game,
	Score alpha, Score beta, unsigned depth, unsigned ply);
template Score Search::negaMax<false>(Position& pos, SearchLimits& limits, SearchResults& results, const Game& game,
	Score alpha, Score beta, unsigned depth, unsigned ply);

Score Search::quiesce(Position& pos, SearchLimits& limits, SearchResults& results, Score alpha, Score beta, unsigned ply) {
	if ((results.nodes_cnt & _check_node_count) == 0 and !limits.isTimeLeft()) {
		return -Score::undef;
	}

	results.nodes_cnt++;
	results.seldepth = std::max(results.seldepth, ply + 1);

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

	while (moves.nextMove(pos, move)) {
		bool legal_move = false;
			
		if (pos.make(move, state)) {
			legal_move = true;
			score = -quiesce(pos, limits, results, -beta, -alpha, ply + 1);
		}

		pos.unmake(move, state);

		if (!score.isValid())
			return -Score::undef;
		else if (legal_move and score > alpha) {
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

	// iterate through only a subset of all game moves
	for (int i = 1; i <= 11; i++) {
		const int cnt = my_cnt - i;

		if (cnt < 0) 
			return false;

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