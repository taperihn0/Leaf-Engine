#include "Search.hpp"
#include "MoveGen.hpp"
#include "NetworkEval.hpp"

#include <sstream>
#include <iomanip>

#define _TT_PROBE_QSEARCH

INLINE bool SearchLimits::isTimeLeft() {
	return !search_time or timer.duration() < search_time;
}

INLINE bool SearchLimits::anyNodesLeft(ull nodes_so_far) {
    return !nodes or nodes_so_far < nodes;
}

INLINE bool SearchLimits::anyQuiesceNodesLeft(ull qnodes_so_far) {
    return !qnodes or qnodes_so_far < qnodes;
}

INLINE void SearchResults::registerBestMove(Move32b move) {
	best_move = move;
}

INLINE void SearchResults::printBestMove() {
	ASSERT(!best_move.isNull(), "Null bestmove");
	std::cout << "bestmove ";
	best_move.print();
	std::cout << '\n';
}

INLINE void SearchResults::print(const Search* search, const Position& pos, TranspositionTable& tt) {
	const uint64_t nps = static_cast<uint64_t>((nodes_cnt * 1000.f) / (duration ? duration : 1));

	std::cout << 
		   "info depth " << depth			 << ' '
		<< "seldepth "   << seldepth		 << ' '
		<< "score "      << score_cp.toStr() << ' '
		<< "nodes "      << nodes_cnt		 << ' '
		<< "time "       << duration		 << ' '
		<< "nps "        << nps				 << ' '
		<< "hashfull "   << tt.getHashfull() << ' '
		<< "pv ";

	Position cpy = pos;

	// print PV line
	while (depth--) {
		TTEntry tt_entry;
		const bool tt_hit = search->_tt.probe(tt_entry, cpy.getZobristKey(), -Score::MateBound, +Score::MateBound, depth);

		Move32b pv_move = unpacked(cpy, tt_entry.move);

		if (!tt_hit or pv_move.isNull()) 
			break;

		pv_move.print(), std::cout << ' ';

		cpy.make(pv_move);
	}

	// flush every line
	std::cout << std::endl;

#if defined(_COLLECT_SEARCH_STATS)
	printSearchStats();
#endif
}

INLINE void SearchResults::printShort() {
	std::cout << "Total nodes: " << nodes_cnt << '\n';
	printBestMove();
#if defined(_COLLECT_SEARCH_STATS)
	printSearchStats();
#endif
}

#if defined(_COLLECT_SEARCH_STATS)
void SearchResults::printSearchStats() {
	const float qnodes_rate      = static_cast<float>(qnodes_cnt) / nodes_cnt * 100;
	const float pvnodes_rate     = static_cast<float>(pvnodes_cnt) / nodes_cnt * 100;
	const float npvnodes_rate    = static_cast<float>(npvnodes_cnt) / nodes_cnt * 100;

	const float qprobes_rate     = static_cast<float>(qtt_probe_cnt) / tt_probe_cnt * 100;
	const float cuts_rate        = static_cast<float>(tt_cut_cnt) / tt_probe_cnt * 100;
	const float qcuts_rate       = static_cast<float>(qtt_cut_cnt) / qtt_probe_cnt * 100;

	const float ttmove_cut_rate  = static_cast<float>(ttmove_cut_cnt) / beta_cut_cnt * 100;
	const float qttmove_rate     = static_cast<float>(qttmove_probe_cnt) / qtt_probe_cnt * 100;
	const float qttmove_cut_rate = static_cast<float>(qttmove_cut_cnt) / qbeta_cut_cnt * 100;

	std::cout << "\n--SEARCH STATISTICS--";

	std::cout
		<< "\nQUIESCENT NODES:         " << qnodes_cnt        << ", " << qnodes_rate << '%'
		<< "\nPV NODES:                " << pvnodes_cnt       << ", " << pvnodes_rate << '%'
		<< "\nNON PV NODES:            " << npvnodes_cnt      << ", " << npvnodes_rate << '%'
		<< "\nTT PROBES:               " << tt_probe_cnt
		<< "\nTT PROBES IN QSEARCH:    " << qtt_probe_cnt     << ", " << qprobes_rate << '%'
		<< "\nTT CUTS:                 " << tt_cut_cnt        << ", " << cuts_rate << '%'
		<< "\nTT CUTS IN QSEARCH:      " << qtt_cut_cnt       << ", " << qcuts_rate << '%'
		<< "\nTTMOVE CUT:              " << ttmove_cut_cnt    << ", " << ttmove_cut_rate << '%'
		<< "\nTTMOVE PROBE IN QSEARCH: " << qttmove_probe_cnt << ", " << qttmove_rate << '%'
		<< "\nTTMOVE CUT IN QSEARCH:   " << qttmove_cut_cnt   << ", " << qttmove_cut_rate << '%'
		<< '\n';

	beta_cut_cnt = !beta_cut_cnt ? 1 : beta_cut_cnt;

	// print beta cutoff rate for each move index
	for (size_t i = 0; i < MaxNodeMoves; i++) {
		const float ind_cut_rate = static_cast<float>(move_cut_cnt[i]) / beta_cut_cnt * 100;

		if (ind_cut_rate > 0.01) {
			std::cout << "\nMOVEIND " << std::setw(3) << i << ": " << ind_cut_rate << '%';
		}
	}

	std::cout << "\n---------------------\n";
}
#endif

void NodeInfo::clear() {
	state 			 = {};
	best_move = move = Move32b::Null;
	score 			 = Score::Undef;
	can_move 		 = false;
	best_score 		 = Score::Undef;
	check 			 = false;
	ply 			 = 0;
	moves_searched 	 = 0;
	move_index 		 = 0;
	eval 	 = Score::Undef;
	bound 			 = TTEntry::NONE;
}

void Search::clearHashTT() {
	_tt.clear();
	_tt.clearHashfull();
}

void Search::resizeHashTT(size_t tt_size_mb) {
	if (tt_size_mb > 0 and tt_size_mb != _tt.getEntriesCount() * sizeof(TTEntry)) {
		_tt.resize(tt_size_mb);
		_tt.clear();
		_tt.clearHashfull();
	}
}

void Search::registerNewGame() {
	clearHashTT();
	_history_buff->clearQuietsHistory();
}

TreeStack::TreeStack() {
	_stack = reinterpret_cast<NodeInfo*>(alignedMalloc(_Count * sizeof(NodeInfo),  CACHELINE_SIZE));
	ASSERT(_stack != nullptr, "Failed to allocate memory");
}

void TreeStack::init(MoveOrderHistoryTables* history_buffer) {
	assert(history_buffer);

	for (size_t i = 0; i < _Count; i++) {
		_stack[i].clear();
		_stack[i].move_picker.setHistoryBuffer(history_buffer);
	}
}

TreeStack::~TreeStack() {
	alignedFree(_stack);
}

INLINE const NodeInfo* TreeStack::getNode(unsigned ply) const {
	assert(ply < _Count);
	return _stack + ply + 1;
}

INLINE NodeInfo* TreeStack::getRootNode() {
	return _stack + 1;
}

INLINE NodeInfo* TreeStack::getPreRootNode() {
	return _stack;
}

Search::Search(TranspositionTable&& tt) 
	: _tt(std::move(tt))
	, _history_buff(reinterpret_cast<MoveOrderHistoryTables*>(
		alignedMalloc(sizeof(MoveOrderHistoryTables), CACHELINE_SIZE))) 
{
	ASSERT(_history_buff != nullptr, "Failed to allocate memory");
	registerNewGame();
	_tree_stack.init(_history_buff);
}

Search::~Search() {
	alignedFree(_history_buff);
}

template <Search::enumInfoLevel InfoLevel>
Move32b Search::bestMove(Position& pos, const FullInfoRecord& game, SearchLimits limits) {
	ASSERT(1 <= limits.depth and limits.depth < MaxDepth, "Invalid depth");

	limits.timer.go();
	limits.search_time = TimeMan::searchTime(pos, limits);
	_tt.newGeneration();

	const Move32b bm = iterativeDeepening<InfoLevel>(pos, game, limits);
	return bm;
}

Move32b Search::_bestMove_unittest(Search& search, 
								   Position& pos, 
								   const FullInfoRecord& game, 
								   SearchLimits limits) 
{
	return search.bestMove<Search::SEARCH_SHORT_INFO>(pos, game, limits);
}

template <Search::enumInfoLevel InfoLevel>
Move32b Search::iterativeDeepening(Position& pos, const FullInfoRecord& game, SearchLimits& limits) {
	SearchResults search_results;
	
	NodeInfo* preroot = _tree_stack.getPreRootNode();
	preroot->accum.refresh(nn::GlobPackedNetwork, pos);
	preroot->move = preroot->best_move = game.currentHalfCount() > 0 ? game.getCurrentMove() 
																	 : Move32b::Null;

	NodeInfo* root = _tree_stack.getRootNode();

	for (unsigned d = 1; d <= limits.depth; d++) {
		search_results.depth = d;

        // TODO: So far, I reject last move when search is finised.
        // TODO: Sometimes it might be actually not really bad.
		if (!search<InfoLevel>(pos, game, limits, search_results))
			break;

		search_results.registerBestMove(root->best_move);
	}

    if constexpr (InfoLevel == SEARCH_FULL_INFO or InfoLevel == SEARCH_ONLY_BM_INFO) {
        search_results.printBestMove();
	}
	else if constexpr (InfoLevel == SEARCH_SHORT_INFO) {
        search_results.printShort();
	}

	return search_results.best_move;
}

template <Search::enumInfoLevel InfoLevel>
bool Search::search(Position& pos, 
					const FullInfoRecord& game, 
					SearchLimits& limits, SearchResults& results) 
{
	const Score score = -negaMax<true>(pos, limits, results, game, _tree_stack.getRootNode(), 
									   -Score::Mate, +Score::Mate, 
									   results.depth, 
									   0);

	_declUnused(score); // score unused so far

    if (results.depth > 1 and (!limits.isTimeLeft()
        or !limits.anyNodesLeft(results.nodes_cnt)
        or !limits.anyQuiesceNodesLeft(results.qnodes_cnt)))
    {
        return false;
    }

	results.duration = limits.timer.duration();

	if constexpr (InfoLevel == SEARCH_FULL_INFO)
		results.print(this, pos, _tt);

	return true;
}

template <bool Root, Search::enumNode NodeType, bool NullMove>
Score Search::negaMax(Position& pos, 
					  SearchLimits& limits, SearchResults& results, 
					  const FullInfoRecord& game, 
					  NodeInfo* node,
					  Score alpha, Score beta, 
					  int depth, int ply) 
{
	assert(0 <= depth and depth < MaxDepth - 1);
	assert(alpha < beta);

	if constexpr (Root) assert(!ply);
	else 				assert(ply > 0);
	
	static constexpr OrderType OrderPolicy = STAGED;
	static constexpr bool	   IsPV = NodeType == PV_NODE;
	static constexpr int 	   RazorDepth = 2;
	static constexpr Score 	   RazorBaseDelta = 150;
	static constexpr Score 	   RazorMultDelta = 25;
	static constexpr int	   IidDepth = 3;
	static constexpr int	   IidDivShift = 1;
	static constexpr int	   RfpDepth = 6;
	static constexpr Score	   RfpMultDelta = 150;
	static constexpr int	   NullReduction = 2;
	static constexpr int	   FutilityDepth = 4;
	static constexpr Score	   FutilityDelta = 32;
	static constexpr int	   LmrDepth = 2;
	static constexpr int	   LmrMoveCount = 2;

	if (!Root and (pos.halfmoveClock() >= 100 or isRepetitionCycle<IsPV>(pos, game, node - 1, ply))) {
		return Score::Draw;
	}
	else if (!Root and (results.nodes_cnt & _CheckNodeCount) == 0 and !limits.isTimeLeft()) {
		return -Score::Undef;
	}
    else if (!Root and (!limits.anyNodesLeft(results.nodes_cnt) or
                        !limits.anyQuiesceNodesLeft(results.qnodes_cnt))) 
    {
        return -Score::Undef;
    }

	const uint64_t hash = pos.getZobristKey();

#if defined(_COLLECT_SEARCH_STATS)
	results.tt_probe_cnt++;
#endif

	TTEntry tt_entry;
	const bool tt_hit = _tt.probe(tt_entry, hash, alpha, beta, depth);
	const bool exact_hit = (!IsPV and tt_hit) or
						   ( IsPV and tt_hit and tt_entry.bound == TTEntry::EXACT);

	if (!Root and exact_hit) {
#if defined(_COLLECT_SEARCH_STATS)
		results.tt_cut_cnt++;
#endif
		return tt_entry.score;
	}

	if (!depth) {
		return quiesce<NodeType>(pos, limits, results, node,
								 alpha, beta,
								 depth,
								 ply);
	}
	
	results.nodes_cnt++;

#if defined(_COLLECT_SEARCH_STATS)
	results.pvnodes_cnt += IsPV;
	results.npvnodes_cnt += !IsPV;
#endif

	const enumColor side2move = pos.getTurn();

	if constexpr (Root)
		node->check = pos.isInCheck(side2move);

	node->move = Move32b::Null;
	node->eval = Score::Undef;

	const NodeInfo* preroot = _tree_stack.getPreRootNode();
	
	const nn::Accumulator* prev_accum = nn::NEval::getPrevAccum(node, preroot); 

	/* Razoring -
	*  if we're at lower depth and the eval is really low
	*  it means there is high propability no move can increase the alpha bar.
	*  To ensure our intuition, we dive into quiescence search to verify the position.
	*  If we fail low, we've got a cutoff.
	*/
	if constexpr (!Root and !IsPV) {
		if (!node->check and
			depth <= RazorDepth)
		{

#if defined(_VERIFY_NN)
			ASSERT(nn::Accumulator::verify(*prev_accum, pos), "Accumulator verification failed");
#endif
			node->eval = nn::NEval::evaluate(nn::GlobPackedNetwork, *prev_accum, side2move);

			if (node->eval + RazorBaseDelta + RazorMultDelta * depth < alpha) {
				const Score qscore = quiesce<NON_PV_NODE>(pos, limits, results, node + 1,
													      alpha - 1, alpha,
													      depth - 1,
													      ply + 1);

				if (qscore < alpha)
					return qscore;
			}
		}
	}

	Move32b ttm32b = unpacked(pos, tt_entry.move);
	Move32b tt_move = ttm32b.isPseudoLegal(pos) ? ttm32b : 
												  Move32b::Null;
	
	/* Internal Iterative Deepening -
	*  done only in PV Nodes. When no hash move is found for said node, 
	*  we allow to do some shallow research in order to obtain one.
	*  That strategy can only pay off when the move ordering is actually 
	*  very important.
	*/
	if constexpr (IsPV) {
		if (depth >= IidDepth and tt_move.isNull()) {
			_UNUSED const Score iid_score =
				negaMax<false, NodeType, false>(pos, limits, results, game, node,
												alpha, beta,
												depth >> IidDivShift,
												ply);

			TTEntry iid_entry;
			_UNUSED const bool iid_tt_hit = _tt.probe(iid_entry, hash, alpha, beta, depth);
			
			ttm32b = unpacked(pos, iid_entry.move);
			tt_move = ttm32b.isPseudoLegal(pos) ? ttm32b 
												: Move32b::Null;
		}
	}

	/* Reverse Futility Pruning (Static Null Move Pruning) -
	*  basically, when we're doing very well, we can prune.
	*  Idea similar to Standing Pat cutoff in Q-Search.
	*/
	if constexpr (!Root and !IsPV) {
		if (!node->check and
			depth <= RfpDepth and
			tt_move.isQuiet())
		{
			if (!node->eval.isValid()) {
#if defined(_VERIFY_NN)
				ASSERT(nn::Accumulator::verify(*prev_accum, pos), "Accumulator verification failed");
#endif
				node->eval = nn::NEval::evaluate(nn::GlobPackedNetwork, *prev_accum, side2move);
			}

			if (node->eval - RfpMultDelta * depth >= beta)
				return node->eval - (depth << 6);
		}
	}

	NodeInfo* const prev_node = node - 1;
	NodeInfo* const next_node = node + 1;

	/* Null Move Pruning -
	*  if we're doing so well even after not making a move, we must be winning here.
	*  So we can do beta cutoff.
	*/
	if constexpr (NullMove) {
		if (!node->check and depth >= NullReduction + 1) {
			assert(prev_node->move != Move32b::Null);
			
			pos.makeNull(node->state);
			node->move = Move32b::Null;

			const Score score = -negaMax<false, NON_PV_NODE, !NullMove>(pos, limits, results, game, next_node,
																		-beta, -beta + 1, 
																		depth - NullReduction - 1, 
																		ply + 1);
			pos.unmakeNull(node->state);

			/* Unless Null Move Pruning is not handled properly in the endgame, 
			*  verification search is just needed to prevent Zugzwang.
			*/
			if (score >= beta) {
				const Score verify = negaMax<false, NON_PV_NODE, !NullMove>(pos, limits, results, game, node,
																			beta - 1, beta, 
																			depth - NullReduction - 1, 
																			ply);

				if (verify >= beta)
					return verify;
			}
		}
	}

	node->move_picker.clear<OrderPolicy>();
	node->move_picker.setHashMove(tt_move);
	
	node->can_move = false;
	node->score = 0;
	node->ply = ply;
	node->best_move = Move32b::Null;
	node->best_score = -Score::Infinity;
	node->moves_searched = 0;
	node->state = pos.getIrreversibleState();
	node->bound = TTEntry::LOWERBOUND;

	for (node->move_index = 0; node->move_picker.nextMove<OrderPolicy, Root>(_tree_stack, pos, node->move); node->move_index++) {

		const uint64_t next_hash = pos.likelyZobristKeyAfterMove(node->move);
		_tt.prefetchBucket(next_hash);

		/* Futility Pruning -
		*  at shallow depths, skip moves that aren't like to rise alpha.
		*/
		if (!node->check and
			depth <= FutilityDepth and
			node->moves_searched > 0 and
			node->move.isQuiet() and
			!node->move.isQueenPromotion())
		{
			if (!node->eval.isValid()) {
#if defined(_VERIFY_NN)
				ASSERT(nn::Accumulator::verify(*prev_accum, pos), "Accumulator verification failed");
#endif
				node->eval = nn::NEval::evaluate(nn::GlobPackedNetwork, *prev_accum, side2move);
			}

			if (node->eval + FutilityDelta * depth * depth < alpha) {
				node->score = alpha;

				if (node->score > node->best_score) {
					node->best_score = node->score;
					node->best_move = node->move;
				}
				
				node->move_picker.skipQuiets();
				continue;
			}
		}

		bool do_full_search = true;

		if (pos.make(node->move, &node->accum, prev_accum)) {
			node->can_move = true;

			const enumColor next_side = !side2move;

			next_node->check = pos.isInCheck(next_side);
			const int extend = calculateExtension(pos, node);

			/* Principle Variation Search -
			*  So far it was avoided in NON-PV nodes.
			*  Now, always searching first move with full window, no matter what.
			*  After that search, every other node is expected CUT node and 
			*  is being search with Null window.
			*/
			if (node->moves_searched > 0) {

				/* Late Move Reduction -
				*  Try to reduce late moves, since they are statistically less interesting.
				*  Prove they fail low using Null window search with some reduction.
				*  If somehow they fail high, then re-search without reduction.
				*/
				if (node->moves_searched >= LmrMoveCount and 
					depth >= LmrDepth and 
					!extend) /* TODO: LMR criteria */
				{
					node->score = -negaMax<false, NON_PV_NODE, true>(pos, limits, results, game, next_node,
																	 -alpha - 1, -alpha, 
																	 depth - 2, 
																	 ply + 1);
				}
				else node->score = alpha + 1;
				
				if (node->score > alpha) {
					node->score = -negaMax<false, NON_PV_NODE, true>(pos, limits, results, game, next_node,
																	 -alpha - 1, -alpha, 
																	 depth - 1 + extend, 
																	 ply + 1);
					/* full search already done */
					if constexpr (!IsPV)
						do_full_search = false;
				}

				if (node->score <= alpha)
					do_full_search = false;
			} 

			if (do_full_search) {
				node->score = -negaMax<false, NodeType, true>(pos, limits, results, game, next_node,
															  -beta, -alpha, 
															  depth - 1 + extend, 
															  ply + 1);
			}

			node->moves_searched++;
		}

		pos.unmake(node->move, node->state);

		if (limits.isTimeLeft() and 
            limits.anyNodesLeft(results.nodes_cnt) and
            limits.anyQuiesceNodesLeft(results.qnodes_cnt) and
			node->move.isLegalMoved() and 
			node->score > node->best_score) 
		{
			node->best_move = node->move;
			node->best_score = node->score;

			if (node->score > alpha) {
				if (node->score >= beta) {
					node->bound = TTEntry::UPPERBOUND;

					if (node->move.isQuiet() and 
						!node->move.isQueenPromotion()) 
					{
						node->move_picker.setKillerMove(node->move);
						node->move_picker.updateQuietsHistory(node->best_move, side2move, depth);
					}

#if defined(_COLLECT_SEARCH_STATS)
					if (node->move == tt_move)
						results.ttmove_cut_cnt++;

					results.beta_cut_cnt++;
					results.move_cut_cnt[node->move_index]++;
#endif

					break;
				}

				node->bound = TTEntry::EXACT;
				alpha = node->score;
			}
		}
		else if (!limits.isTimeLeft() or
                 !limits.anyNodesLeft(results.nodes_cnt) or
                 !limits.anyQuiesceNodesLeft(results.qnodes_cnt))
        {
			if (Root and node->best_move.isNull()) {
				// TODO: move at root assigned here might be illegal.
				node->best_move = node->move;
			}

			return -Score::Undef;
		}
	}
	
	// detect checkmate or stealmate
	if (!node->can_move) {
		node->bound = TTEntry::EXACT;
		node->best_score = node->check ? -Score::Mate + ply : Score::Draw;
	}

	if (!node->best_score.isMateScore() or tt_entry.isEmpty()) {
		const Move16b bestmove16b = packed(node->best_move);
		_tt.write(hash, depth, ply, node->bound, node->best_score, bestmove16b, results);
	}

	next_node->move_picker.setKillerMove(Move32b::Null);

	if constexpr (Root) {
		results.score_cp = node->best_score;
	}

	return node->best_score;
}

template Score Search::negaMax<true> (Position&, 
									  SearchLimits&, SearchResults&, 
									  const FullInfoRecord&, 
									  NodeInfo*,
									  Score, Score, 
									  int, int);
template Score Search::negaMax<false>(Position&,
									  SearchLimits&, SearchResults&, 
									  const FullInfoRecord&, 
									  NodeInfo*,
									  Score, Score, 
									  int, int);

template <Search::enumNode NodeType>
Score Search::quiesce(Position& pos, 
					  SearchLimits& limits, SearchResults& results, 
					  NodeInfo* node, 
					  Score alpha, Score beta, 
					  int depth, int ply) 
{
	assert(alpha < beta);

	static constexpr OrderType QuiescentOrderPolicy = QUIESCENT;
	static constexpr bool	   IsPV = NodeType == PV_NODE;
	static constexpr bool	   Root = false;
	static constexpr bool	   SeeNonExactScore = false;
	static constexpr Score	   MaterialDelta = 900;
	
	const enumColor side2move = pos.getTurn();

	if ((results.nodes_cnt & _CheckNodeCount) == 0 and !limits.isTimeLeft()) {
		return -Score::Undef;
	}
    else if (!limits.anyNodesLeft(results.nodes_cnt) or
             !limits.anyQuiesceNodesLeft(results.qnodes_cnt))
    {
        return -Score::Undef;
    }

	const NodeInfo* preroot = _tree_stack.getPreRootNode();
	
	const nn::Accumulator* prev_accum = nn::NEval::getPrevAccum(node, preroot); 

	if (ply >= static_cast<int>(MaxSelDepth)) _UNLIKELY {
#if defined(_VERIFY_NN)
		ASSERT(nn::Accumulator::verify(*prev_accum, pos), "Accumulator verification failed");
#endif
		return nn::NEval::evaluate(nn::GlobPackedNetwork, *prev_accum, side2move);
	}

#if defined(_COLLECT_SEARCH_STATS)
	results.tt_probe_cnt++;
	results.qtt_probe_cnt++;
#endif

#if defined(_TT_PROBE_QSEARCH)
	TTEntry tt_entry;
	tt_entry.move = Move16b::Null;

	const uint64_t hash = pos.getZobristKey();
	const uint8_t probe_depth = static_cast<uint8_t>(std::max(0, depth));

	const bool tt_hit = _tt.probe(tt_entry, hash, alpha, beta, probe_depth);
	const bool exact_hit = (!IsPV and tt_hit) or 
						   (IsPV and tt_hit and tt_entry.bound == TTEntry::EXACT);

	if (exact_hit and depth < 0) {
#if defined(_COLLECT_SEARCH_STATS)
		results.tt_cut_cnt++;
		results.qtt_cut_cnt++;
#endif
		return tt_entry.score;
	}
#endif

	results.nodes_cnt++;
	results.seldepth = std::max(results.seldepth, static_cast<unsigned>(ply + 1));
	results.qnodes_cnt++;

#if defined(_VERIFY_NN)
	ASSERT(nn::Accumulator::verify(*prev_accum, pos), "Accumulator verification failed");
#endif

	const Score stand_pat = nn::NEval::evaluate(nn::GlobPackedNetwork, *prev_accum, side2move);

	/* Delta Pruning -
	*  when no move has any chance to raise alpha
	*  then prune all of the branches.
	*/
	if (stand_pat + MaterialDelta < alpha)
		return alpha;
	
	/* Standing Pat Cutoff -
	*  when we're already above the beta, we can make a cutoff.
	*/
	else if (stand_pat > alpha) {
		if (stand_pat >= beta) 
			return beta;

		node->score = alpha = stand_pat;
	}

	node->move_picker.clear<QuiescentOrderPolicy>();

	Move32b tt_move = Move32b::Null;

#if defined(_TT_PROBE_QSEARCH)
	const Move16b ttm16b = tt_entry.move;

	if ((!IsPV or tt_entry.bound != TTEntry::UPPERBOUND) and 
		(isCapturePacked(pos, ttm16b) or ttm16b.isQueenPromotion()))
	{
		const Move32b ttm32b = unpacked(pos, tt_entry.move);
		tt_move = ttm32b.isPseudoLegal(pos) ? ttm32b :
											  Move32b::Null;
		node->move_picker.setHashMove(tt_move);

#if defined(_COLLECT_SEARCH_STATS)
		results.qttmove_probe_cnt++;
#endif
	}
#endif

	node->moves_searched = 0;
	node->state = pos.getIrreversibleState();

	for (node->move_index = 0; node->move_picker.nextMove<QuiescentOrderPolicy, Root>(_tree_stack, pos, node->move); node->move_index++) {
		
#if defined(_TT_PROBE_QSEARCH)
		const uint64_t next_hash = pos.likelyZobristKeyAfterMove(node->move);
		_tt.prefetchBucket(next_hash);
#endif

		/* Static Exchange Evaluation Pruning -
		*  ignore losing captures, as they aren't likely to rise alpha anyway.
		*/
		if (node->move.isCapture() and
			!node->move.isEnPassant() and 
			!node->move.isPromotion())
		{
			const Square org = node->move.getOrigin();
			const Square dst = node->move.getTarget();
			const Piece::enumType vic = node->move.getCaptured(pos);
			const Piece::enumType piece = node->move.getPiece();

			const int capt_see_score = pos.StaticExchangeEval<SeeNonExactScore>(org, dst, vic, piece);

			if (capt_see_score < 0)
				continue;
		}

		if (pos.make(node->move, &node->accum, prev_accum)) {
			node->score = -quiesce<NodeType>(pos, limits, results, node + 1,
											 -beta, -alpha,
											 depth - 1,
											 ply + 1);

			node->moves_searched++;
		}

		pos.unmake(node->move, node->state);

		if (limits.isTimeLeft() and 
            limits.anyNodesLeft(results.nodes_cnt) and
            limits.anyQuiesceNodesLeft(results.qnodes_cnt) and
			node->move.isLegalMoved() and 
			node->score > alpha) 
		{
			if (node->score >= beta) {
#if defined(_COLLECT_SEARCH_STATS)
				if (node->move == tt_move) {
					results.ttmove_cut_cnt++;
					results.qttmove_cut_cnt++;
				}

				results.beta_cut_cnt++;
				results.qbeta_cut_cnt++;

				results.move_cut_cnt[node->move_index]++;
#endif
				return beta;
			}
			
			alpha = node->score;
		}
		else if (!limits.isTimeLeft() or
                 !limits.anyNodesLeft(results.nodes_cnt) or
                 !limits.anyQuiesceNodesLeft(results.qnodes_cnt))
        {
			return -Score::Undef;
		}
	}

	return alpha;
}

// TODO: smarter extension calculation
INLINE int Search::calculateExtension(Position& pos, NodeInfo* node) {
	_declUnused(pos);
	NodeInfo* next_node = node + 1;
	return next_node->check;
}

template <bool IsPV>
bool Search::isRepetitionCycle(const Position& pos, 
							   const FullInfoRecord& game, 
							   NodeInfo* node, 
							   int ply) 
{
	const int my_ply = ply;
	const uint64_t my_hashkey = pos.getZobristKey();

	int rep_cnt = 0;

	static constexpr int SearchRepDepth = 37;
	static_assert(SearchRepDepth % 2);

	for (ply = ply - 1; ply >= 0; ply--, node--) {
		const Move32b move = node->move;

		if (((my_ply - ply) & 1) == 1)
			continue;
		else if (move.isIrreversible())
			return false;
		else if (my_hashkey == node->state.hash_key) 
		{
			if constexpr (!IsPV) 
				return true;

			if (++rep_cnt >= 2)
				return true;
		}
	}

	const int my_cnt = static_cast<int>(game.currentHalfCount());

	// iterate through only a subset of all game moves
	for (int i = 1; i <= SearchRepDepth; i++) {
		const int cnt = my_cnt - i;

		if (cnt < 0) 
			return false;

		const Move32b move = game.getPrevMove(cnt);

		// TODO: move.isIrreversible() first?
		if ((i & 1) == 0)
			continue;
		else if (move.isIrreversible())
			return false;
		else if (my_hashkey == game.getPrevKey(cnt))
		{
			if constexpr (!IsPV)
				return true;

			if (++rep_cnt >= 2)
				return true;
		}
	}

	return false;
}

template Move32b Search::bestMove<Search::SEARCH_FULL_INFO>(Position&, const FullInfoRecord&, SearchLimits);
template Move32b Search::bestMove<Search::SEARCH_SHORT_INFO>(Position&, const FullInfoRecord&, SearchLimits);
template Move32b Search::bestMove<Search::SEARCH_ONLY_BM_INFO>(Position&, const FullInfoRecord&, SearchLimits);
template Move32b Search::bestMove<Search::SEARCH_NO_INFO>(Position&, const FullInfoRecord&, SearchLimits);
