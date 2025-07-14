#include "Search.hpp"
#include "Eval.hpp"
#include "MoveGen.hpp"
#include "Time.hpp"
#include "TranspositionTable.hpp"

#include <sstream>

INLINE bool SearchLimits::isTimeLeft() {
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

Search::Search()
	: _tt() {}

INLINE void SearchResults::print(const Search* search, const Position& pos) {
	const uint64_t nps = static_cast<uint64_t>((nodes_cnt * 1000.f) / (duration ? duration : 1));

	std::cout << "info depth " << depth
		<< " seldepth " << seldepth
		<< " score " << score_cp.toStr()
		<< " nodes " << nodes_cnt
		<< " time " << duration
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

		cpy.make(pv_move);
	}

	std::cout << '\n';
}

INLINE void SearchResults::printShort() {
	std::cout << "Total nodes: " << nodes_cnt << '\n';
	printBestMove();
}

void Search::registerNewGame() {
	_tt.clear();
}

template <bool PrintFullInfo>
Move Search::bestMove(Position& pos, const Game& game, SearchLimits limits) {
	ASSERT(1 <= limits.depth and limits.depth < max_depth, "Invalid depth");

	limits.timer.go();
	limits.search_time = TimeMan::searchTime(pos, limits);

	const Move bm = iterativeDeepening<PrintFullInfo>(pos, game, limits);
	return bm;
}

Move Search::_bestMove_unittest(Search& search, Position& pos, const Game& game, SearchLimits limits) {
	return search.bestMove<false>(pos, game, limits);
}

template <bool PrintFullInfo>
Move Search::iterativeDeepening(Position& pos, const Game& game, SearchLimits& limits) {
	SearchResults search_results;
	search_results.tt_entries = _tt.getEntriesCount();

	NodeInfo* root = _tree_stack.getRootNode();

	for (unsigned d = 1; d <= limits.depth; d++) {
		search_results.depth = d;

		_tree_stack.clear();

		if (!search<PrintFullInfo>(pos, game, limits, search_results))
			break;

		search_results.registerBestMove(root->best_move);
	}

	if constexpr (PrintFullInfo)
		search_results.printBestMove();
	else
		search_results.printShort();

	return search_results.best_move;
}

template <bool PrintFullInfo>
bool Search::search(Position& pos, const Game& game, SearchLimits& limits, SearchResults& results) {
	const Score score
		= -negaMax<true>(pos, limits, results, game, _tree_stack.getRootNode(), -Score::mate, +Score::mate, results.depth, 0);

	if (results.depth > 1 and !limits.isTimeLeft())
		return false;

	results.duration = limits.timer.duration();

	if constexpr (PrintFullInfo)
		results.print(this, pos);

	return true;
}

template <bool Root, Search::enumNode NodeType, bool NullMove>
Score Search::negaMax(Position& pos, SearchLimits& limits, SearchResults& results, const Game& game, NodeInfo* node,
	Score alpha, Score beta, unsigned depth, unsigned ply) {

	assert(0 <= depth and depth < max_depth);
	assert(alpha < beta);

	if constexpr (!Root) {
		if (pos.halfmoveClock() >= 100 or isRepetitionCycle(pos, game, node - 1, ply)) {
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
	
	if constexpr (!Root and NodeType == NON_PV_NODE) {
		if (tt_hit) return tt_entry.score;
	}
	else if constexpr (!Root) {
		if (tt_hit and tt_entry.bound == TTEntry::EXACT) 
			return tt_entry.score;
	}
	
	results.nodes_cnt++;

	if constexpr (Root)
		node->check = pos.isInCheck(pos.getTurn());

	if constexpr (NullMove) {
		static constexpr int R = 2;

		if (!node->check and depth >= R + 1) {
			pos.makeNull(node->state);
			const Score score =
				-negaMax<false, NON_PV_NODE, false>(pos, limits, results, game, node + 1, -beta, -beta + 1, depth - R - 1, ply + 1);
			pos.unmakeNull(node->state);

			/* Unless Null Move Pruning is not handled properly in endgame, 
			*  verification search is just needed 
			*/
			if (score >= beta) {
				const Score verify = 
					negaMax<false, NON_PV_NODE, false>(pos, limits, results, game, node, beta - 1, beta, depth - R - 1, ply);

				if (verify >= beta)
					return verify;
			}
		}
	}

	const Move tt_move = tt_entry.move.isPseudoLegal(pos) ? tt_entry.move : Move::null;

	node->move_picker.clear();
	node->move_picker.setHashMove(tt_move);
	
	node->can_move = false;
	node->score = 0;
	node->ply = ply;
	node->best_move = Move::null;
	node->best_score = -Score::infinity;
	node->moves_searched = 0;
	node->state = pos.getIrreversibleState();

	TTEntry::Bound bound_type = TTEntry::LOWERBOUND;

	while (node->move_picker.nextMove(_tree_stack, pos, node->move)) {
		bool do_full_search = true;

		if (pos.make(node->move)) {
			node->can_move = true;

			int extend = 0;

			/* Principle Variation Search -
			*  So far it was avoided in NON-PV nodes.
			*  Now, always searching first move with full window, no matter what.
			*  After that search, every other node is expected CUT node and 
			*  is being search with null window.
			*/
			if (node->moves_searched > 0) {

				(node + 1)->check = pos.isInCheck(pos.getTurn());
				extend = static_cast<int>((node + 1)->check);

				/* Late Move Reduction -
				*  Try to reduce late moves, since they are statistically less interesting.
				*  Prove they fail low using null window search with some reduction.
				*  If somehow they fail high, then re-search without reduction.
				*/
				if (node->moves_searched >= 3 and depth >= 2 and !extend) {
					node->score =
						-negaMax<false, NON_PV_NODE, true>(pos, limits, results, game, node + 1, -alpha - 1, -alpha, depth - 2, ply + 1);
				}
				else node->score = alpha + 1;
				
				if (node->score > alpha) {
					node->score =
						-negaMax<false, NON_PV_NODE, true>(pos, limits, results, game, node + 1, -alpha - 1, -alpha, depth - 1 + extend, ply + 1);

					if constexpr (NodeType == NON_PV_NODE)
						do_full_search = false;
				}

				if (node->score <= alpha)
					do_full_search = false;
			} 

			if (do_full_search) {
				node->score =
					-negaMax<false, NodeType, true>(pos, limits, results, game, node + 1, -beta, -alpha, depth - 1 + extend, ply + 1);
			}

			node->moves_searched++;
		}

		pos.unmake(node->move, node->state);

		if (limits.isTimeLeft() and node->move.isLegalMoved() and node->score > node->best_score) {
			node->best_move = node->move;
			node->best_score = node->score;

			if (node->score > alpha) {
				if (node->score >= beta) {
					bound_type = TTEntry::UPPERBOUND;
					if (node->move.isQuiet() and (!node->move.isPromotion() or node->move.getPromoPiece() != Piece::QUEEN)) {
						node->move_picker.setKillerMove(node->move);
						node->move_picker.updateHistory(node->move, pos.getTurn(), depth);
					}
					break;
				}

				bound_type = TTEntry::EXACT;
				alpha = node->score;
			}
		}
		else if (!limits.isTimeLeft()) {
			if (Root and node->best_move.isNull())
				// TODO: move at root assigned here might be illegal.
				node->best_move = node->move;

			return -Score::undef;
		}
	}
	
	// detect checkmate or stealmate
	if (!node->can_move) {
		bound_type = TTEntry::EXACT;
		node->best_score = node->check ? -Score::mate + ply : Score::draw;
	}
	if (node->best_score > -Score::mate_bound and node->best_score < Score::mate_bound)
		_tt.write(pos.getZobristKey(), depth, ply, bound_type, node->best_score, node->best_move, results);

	(node + 1)->move_picker.setKillerMove(Move::null);

	if constexpr (Root) {
		results.score_cp = node->best_score;
	}

	return node->best_score;
}

template Score Search::negaMax<true>(Position& pos, SearchLimits& limits, SearchResults& results, const Game& game, NodeInfo* node,
	Score alpha, Score beta, unsigned depth, unsigned ply);
template Score Search::negaMax<false>(Position& pos, SearchLimits& limits, SearchResults& results, const Game& game, NodeInfo* node,
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
	Move move;
	Score score = 0;
	Position::IrreversibleState state = pos.getIrreversibleState();

	while (moves.nextMove(_tree_stack, pos, move)) {
		if (!move.isEnPassant() and !move.isPromotion() and
			pos.StaticExchangeEval<false>(move.getOrigin(), move.getTarget(),
				pos.pieceOn(move.getTarget(), pos.getOppositeTurn()), move.getPiece()) < 0) {
			continue;
		}

		if (pos.make(move)) {
			score = -quiesce(pos, limits, results, -beta, -alpha, ply + 1);
		}

		pos.unmake(move, state);

		if (!score.isValid())
			return -Score::undef;
		else if (move.isLegalMoved() and score > alpha) {
			if (score >= beta) return beta;
			alpha = score;
		}
	}

	return alpha;
}

bool Search::isRepetitionCycle(const Position& pos, const Game& game, NodeInfo* node, int ply) {
	const int my_ply = ply;
	const uint64_t my_hashkey = pos.getZobristKey();

	static constexpr int search_rep_depth = 11;
	static_assert(search_rep_depth & 1);

	for (ply = ply - 1; ply >= 0; ply--, node--) {
		const Move move = node->move;

		if (move.isIrreversible())
			return false;
		else if (((my_ply - ply) & 1) == 1)
			continue;
		else if (my_hashkey == node->state.hash_key /* previous hashkey */)
			return true;
	}

	const int my_cnt = static_cast<int>(game.currentHalfCount());

	// iterate through only a subset of all game moves
	for (int i = 1; i <= search_rep_depth; i++) {
		const int cnt = my_cnt - i;

		if (cnt < 0) 
			return false;

		const Move move = game.getPrevMove(cnt);

		if (move.isIrreversible())
			return false;
		else if ((i & 1) == 0)
			continue;
		else if (my_hashkey == game.getPrevKey(cnt))
			return true;
	}

	return false;
}

template Move Search::bestMove<true>(Position&, const Game&, SearchLimits);
template Move Search::bestMove<false>(Position&, const Game&, SearchLimits);
