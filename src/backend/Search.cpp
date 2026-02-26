#include "Search.hpp"
#include "NetworkEval.hpp"
#include "Tuning.hpp"

#ifdef _COLLECT_SEARCH_STATS
#include <iomanip>
#endif

#define _TT_PROBE_QSEARCH
//#define _CUCKOO_DRAW

#if defined(_CUCKOO_DRAW)
#error "No proper draw value handling"
#endif

INLINE bool SearchLimits::isTimeLimit() {
	return search_time;
}

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

#define _ASSERT_NONZERO(x) 			    \
	do { x = x ? x : 1; } while (false) \

	ull nmnodes = nodes_cnt - qnodes_cnt;

	_ASSERT_NONZERO(nodes_cnt);
	_ASSERT_NONZERO(qnodes_cnt);
	_ASSERT_NONZERO(tt_probe_cnt);
	_ASSERT_NONZERO(qtt_probe_cnt);
	_ASSERT_NONZERO(beta_cut_cnt);
	_ASSERT_NONZERO(qtt_probe_cnt);
	_ASSERT_NONZERO(qbeta_cut_cnt);
	_ASSERT_NONZERO(nmnodes);

	const float qnodes_rate      = static_cast<float>(qnodes_cnt) / nodes_cnt * 100;
	const float pvnodes_rate     = static_cast<float>(pvnodes_cnt) / nodes_cnt * 100;
	const float npvnodes_rate    = static_cast<float>(npvnodes_cnt) / nodes_cnt * 100;

	const float qprobes_rate     = static_cast<float>(qtt_probe_cnt) / tt_probe_cnt * 100;
	const float cuts_rate        = static_cast<float>(tt_cut_cnt) / tt_probe_cnt * 100;
	const float qcuts_rate       = static_cast<float>(qtt_cut_cnt) / qtt_probe_cnt * 100;

	const float ttmove_cut_rate  = static_cast<float>(ttmove_cut_cnt) / beta_cut_cnt * 100;
	const float qttmove_rate     = static_cast<float>(qttmove_probe_cnt) / qtt_probe_cnt * 100;
	const float qttmove_cut_rate = static_cast<float>(qttmove_cut_cnt) / qbeta_cut_cnt * 100;

	const float nmeval_rate  	 = static_cast<float>(nmeval_cnt) / nmnodes * 100;
	const float qeval_rate 		 = static_cast<float>(qeval_cnt) / qnodes_cnt * 100;

	std::cout << "\n--SEARCH STATISTICS--";

	std::cout
		<< "\nQUIESCENT NODES:             " << qnodes_cnt        << ", " << qnodes_rate << '%'
		<< "\nPV NODES:                    " << pvnodes_cnt       << ", " << pvnodes_rate << '%'
		<< "\nNON PV NODES:                " << npvnodes_cnt      << ", " << npvnodes_rate << '%'
		<< "\nTT PROBES:                   " << tt_probe_cnt
		<< "\nTT PROBES IN QSEARCH:        " << qtt_probe_cnt     << ", " << qprobes_rate << '%'
		<< "\nTT CUTS:                     " << tt_cut_cnt        << ", " << cuts_rate << '%'
		<< "\nTT CUTS IN QSEARCH:          " << qtt_cut_cnt       << ", " << qcuts_rate << '%'
		<< "\nHASH-MOVE CUT:               " << ttmove_cut_cnt    << ", " << ttmove_cut_rate << '%'
		<< "\nHASH-MOVE PROBE IN QSEARCH:  " << qttmove_probe_cnt << ", " << qttmove_rate << '%'
		<< "\nHASH-MOVE CUT IN QSEARCH:    " << qttmove_cut_cnt   << ", " << qttmove_cut_rate << '%'
		<< "\nEVAL CALLS IN NEGA-M-SEARCH: " << nmeval_cnt 		  << ", " << nmeval_rate << '%'
		<< "\nEVAL CALLS IN QSEARCH:       " << qeval_cnt 		  << ", " << qeval_rate << '%'
		<< "\nREPETITION CALLS:            " << rep_call_cnt
		<< "\nREPETITION CYCLES:           " << rep_cnt
		<< "\nCUCKOO CYCLES:               " << cuckoo_rep_cnt
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

#undef _ASSERT_NONZERO

}
#endif

void NodeInfo::clear() {
	side2move		 = WHITE;
	state 			 = {};
	best_move = move = Move32b::Null;
	score 			 = Score::Undef;
	eval 			 = Score::Undef;
	improving_rate   = 0.f;
	can_move 		 = false;
	best_score 		 = Score::Undef;
	check 			 = false;
	moves_searched 	 = 0;
	move_index 		 = 0;
	bound 			 = TTEntry::NONE;

	cluster.accum_cache.clearBuffers();
    cluster.next_cluster = nullptr;
    cluster.prev_cluster = nullptr;

	cuckoo_check	= false;
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
	_stack = reinterpret_cast<NodeInfo*>(alignedMalloc(sizeof(NodeInfo) * _Count, CACHELINE_SIZE));
	ASSERT(_stack != nullptr, "Failed to allocate memory");
}

void TreeStack::init(MoveOrderHistoryTables* history_buffer) {
	ASSERTNOLOG(history_buffer);

	for (int i = 0; i < static_cast<int>(_Count); i++) {
        NodeInfo* const node = &_stack[i];

		node->clear();
		node->move_picker.setHistoryBuffer(history_buffer);
        node->cluster.prev_cluster = i - 1 >= 0 ? &(node - 1)->cluster : nullptr;
        node->cluster.next_cluster = i + 1 < static_cast<int>(_Count) ? &(node + 1)->cluster : nullptr;
	}
}

TreeStack::~TreeStack() {
	alignedFree(_stack);
	//delete[] _stack;
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

INLINE const AccumulatorCluster* TreeStack::getCleanAccumulatorCluster(const AccumulatorCluster* const accum_cluster,
																	   const NodeInfo* const preroot)
{
	for (const AccumulatorCluster* prev_accum_cluster = accum_cluster->prev_cluster;
		prev_accum_cluster != &preroot->cluster;
		prev_accum_cluster = prev_accum_cluster->prev_cluster) {

		if (!prev_accum_cluster->accum_cache.isDirty())
			return prev_accum_cluster;
	}

	return &preroot->cluster;
}

INLINE void TreeStack::updateDirtyAccumulators(const AccumulatorCluster* const clean_accum_cluster,
											   AccumulatorCluster* const accum_cluster)
{
	for (AccumulatorCluster* prev_cluster = const_cast<AccumulatorCluster*>(clean_accum_cluster->next_cluster);
		prev_cluster != accum_cluster;
		prev_cluster = prev_cluster->next_cluster) {

		int added_features_index[2][2];
		int removed_features_index[2][2];

		nn::AccumulatorCache& accum_cache = prev_cluster->accum_cache;

		for (size_t i = 0; i < accum_cache.added_features_cnt; i++) {
			nn::FeatureData feature_data = accum_cache.added_features[i];

			added_features_index[WHITE][i] = nn::Accumulator::featureIndex<WHITE>(
																	feature_data.sq,
																	feature_data.piece_type,
																	feature_data.side);

			added_features_index[BLACK][i] = nn::Accumulator::featureIndex<BLACK>(
																	feature_data.sq,
																	feature_data.piece_type,
																	feature_data.side);
		}

		for (size_t i = 0; i < accum_cache.removed_features_cnt; i++) {
			nn::FeatureData feature_data = accum_cache.removed_features[i];

			removed_features_index[WHITE][i] = nn::Accumulator::featureIndex<WHITE>(
																	feature_data.sq,
																	feature_data.piece_type,
																	feature_data.side);

			removed_features_index[BLACK][i] = nn::Accumulator::featureIndex<BLACK>(
																	feature_data.sq,
																	feature_data.piece_type,
																	feature_data.side);
		}

		const nn::AccumulatorCache& prev_accum_cache = prev_cluster->prev_cluster->accum_cache;
		assert(prev_accum_cache.isClean());

		accum_cache.accum.update(nn::GlobPackedNetwork,
								  &prev_accum_cache.accum,
								  added_features_index[WHITE],
								  accum_cache.added_features_cnt,
								  removed_features_index[WHITE],
								  accum_cache.removed_features_cnt,
								  WHITE);
		accum_cache.accum.update(nn::GlobPackedNetwork,
								  &prev_accum_cache.accum,
								  added_features_index[BLACK],
								  accum_cache.added_features_cnt,
								  removed_features_index[BLACK],
								  accum_cache.removed_features_cnt,
								  BLACK);
		accum_cache.markClean();
	}
}

Search::Search(TranspositionTable&& tt) 
	: _tt(std::move(tt))
	, _history_buff(reinterpret_cast<MoveOrderHistoryTables*>(
				    alignedMalloc(sizeof(MoveOrderHistoryTables), CACHELINE_SIZE))) 
{
	ASSERT(_history_buff != nullptr, "Failed to allocate memory");
	registerNewGame();

	_tree_stack.init(_history_buff);
	_cuckoo_tables.init();
}

Search::~Search() {
	alignedFree(_history_buff);
}

template <Search::enumInfoLevel InfoLevel>
Move32b Search::bestMove(Position& pos, const FullInfoRecord& game, SearchLimits limits) {
	ASSERT(1 <= limits.depth and limits.depth < MaxDepth, "Invalid depth");

	_tt.newGeneration();
	
	limits.timer.go();
	limits.search_time = TimeMan::searchTime(pos, limits);

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
	preroot->cluster.accum_cache.accum.refresh(nn::GlobPackedNetwork, pos);
	preroot->cluster.accum_cache.markClean();
	preroot->move = preroot->best_move = game.currentHalfCount() > 0 ? game.getCurrentMove() 
																	 : Move32b::Null;

	NodeInfo* root = _tree_stack.getRootNode();
    root->cluster.prev_cluster = &preroot->cluster;
    preroot->cluster.next_cluster = &root->cluster;

	bool unstable = false;

	for (int d = 1; d <= limits.depth; d++) {
		search_results.depth = d;

		/* We approximate how much time we need for 
		*  next search. This is done via Efective Branch Factor (EBF)
		*  estimator. We also consider unstability of the search in order
		*  to setup a rational breaking system when we got too little time
		*  for deeper search.
		*/
		if (limits.isTimeLimit() and d - 2 > 0 and 
			search_results.nodes_per_depth[d - 2] > 0) 
		{
			double ef_branch_factor = search_results.nodes_per_depth[d - 1] / 
									  search_results.nodes_per_depth[d - 2];

			if (d - 3 > 0 and 
				search_results.nodes_per_depth[d - 3] > 0) 
			{
				double ef_branch_factor2 = search_results.nodes_per_depth[d - 2] / 
									  	   search_results.nodes_per_depth[d - 3];

				ef_branch_factor = (ef_branch_factor + ef_branch_factor2) / 2.;
			}

			// avoid strange instabilities in shallow depths
			ef_branch_factor = std::clamp(ef_branch_factor, 
										  static_cast<double>(MinTimeBranchFactor), 
										  static_cast<double>(MaxTimeBranchFactor)); 

			const time_ms_t approx_search_time = static_cast<time_ms_t>(search_results.time_per_depth[d - 1] * 
																		ef_branch_factor);

			const float time_margin_mult = unstable ? (UnstableMultMargin / 4.) : 1.;

			if (time_margin_mult * limits.search_time < 4. * approx_search_time / NextDepthTimeRed)
				break;
		}

		const ull		prev_total_node_cnt = d > 1 ? search_results.nodes_cnt : 0;
		const time_ms_t prev_total_duration = d > 1 ? search_results.duration  : 0_ms;
		const Score     prev_best_score 	= d > 1 ? root->best_score 		   : Score::Undef;
		const Move32b	prev_best_move		= d > 1 ? root->best_move  		   : Move32b::Null;

        // TODO: So far, I reject last move when search is finished.
        // TODO: Sometimes it might be actually not really bad.
		if (!search<InfoLevel>(pos, game, limits, search_results))
			break;
		
		search_results.registerBestMove(root->best_move);

		search_results.nodes_per_depth[d] = search_results.nodes_cnt - prev_total_node_cnt;
		search_results.time_per_depth[d]  = search_results.duration  - prev_total_duration;

		/* Setting up unstability flag */

		unstable = false;

		if (prev_best_score.isValid() and root->best_score.isValid()) {
			const int score_diff = static_cast<int>(root->best_score) - static_cast<int>(prev_best_score);
			unstable |= (abs(score_diff) > UnstableMatMargin);
		}
		
		if (!prev_best_move.isNull() and !root->best_move.isNull())
			unstable |= (prev_best_move != root->best_move);
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
	const Score root_score = -negaMax<true>(pos, limits, results, game, _tree_stack.getRootNode(), 
									   		-Score::Mate, +Score::Mate, 
									   		results.depth, 
									   		0);

	_declUnused(root_score);

	results.seldepth = std::max(results.seldepth, results.depth);

    if (results.depth > 1 and (!limits.isTimeLeft()
        or !limits.anyNodesLeft(results.nodes_cnt)
        or !limits.anyQuiesceNodesLeft(results.qnodes_cnt)))
    {
        return false;
    }

	results.duration = limits.timer.duration();

	if constexpr (InfoLevel == SEARCH_FULL_INFO) {
		results.print(this, pos, _tt);
	}

	return true;
}

template <bool Root, Search::enumNode NmNodeType, bool NullMove>
Score Search::negaMax(Position& pos, 
					  SearchLimits& limits, SearchResults& results, 
					  const FullInfoRecord& game, 
					  NodeInfo* node,
					  Score alpha, Score beta, 
					  int depth, int ply) 
{
	assert(0 <= depth and depth <= MaxDepth - 1);
	assert(alpha < beta);

	if constexpr (Root) assert(!ply);
	else 				assert(ply > 0);

	static constexpr OrderType OrderPolicy = STAGED;
	static constexpr bool	   IsPV 	   = NmNodeType & PV_NODE;

	if constexpr (!Root) {

		if (pos.getHalfmoveClock() >= 100 or isInsufficientMaterial(pos))
			return getDrawScore<Root>(node);
	}
	
	node->side2move = pos.getTurn();
	NodeInfo* const prev_node = node - 1;

#if !defined(DEBUG) /* disable annoying warning in RELEASE builds */
	_declUnused(prev_node);
#endif

#if !defined(_CUCKOO_DRAW)

	if constexpr (!Root) {
		if (isRepetitionCycle<IsPV>(pos, game, node, ply, results)) {

#if defined(_COLLECT_SEARCH_STATS)
			results.rep_cnt++;
#endif
			return getDrawScore<Root>(node);
		}
	}

#else // Cuckoo further draw checking

	if constexpr (!Root) {

		/* Repetition rule -
		*  however, we could already check if there is any repetition out there in cuckoo tables.
		*  If my parent searched for a repetition and failed, we probably don't have any repetition.
		*/
		if (!prev_node->cuckoo_check) {

			if (isRepetitionCycle<IsPV>(pos, game, node, ply, results)) {

#if defined(_COLLECT_SEARCH_STATS)
				results.rep_cnt++;
#endif

				return Score::Draw;
			}
		}
	}

	if constexpr (!Root and !IsPV) {

		node->cuckoo_check = alpha < Score::Draw;

		if (alpha < Score::Draw and canRepetitionDraw(pos, node, ply)) {

#if defined(_COLLECT_SEARCH_STATS)
			results.cuckoo_rep_cnt++;
#endif

			alpha = Score::Draw;

			if (alpha >= beta) {
				return beta;
			}
		}
	}

#endif
	
	else if (!Root and (results.nodes_cnt & CheckNodeCount) == 0 and !limits.isTimeLeft()) {
		return -Score::Undef;
	}
    
	else if (!Root and (!limits.anyNodesLeft(results.nodes_cnt) or
                        !limits.anyQuiesceNodesLeft(results.qnodes_cnt))) {
        return -Score::Undef;
    }

	const uint64_t hash = pos.getZobristKey();

#if defined(_COLLECT_SEARCH_STATS)
	results.tt_probe_cnt++;
#endif

	TTEntry tt_entry;
	tt_entry.eval = Score::Undef;
	tt_entry.move = Move16b::Null;
	tt_entry.score = Score::Undef;

	const bool tt_hit = _tt.probe(tt_entry, hash, alpha, beta, depth);
	const bool exact_hit = (!IsPV and tt_hit) or
						   (IsPV and tt_hit and tt_entry.bound == TTEntry::EXACT);

	if (!Root and exact_hit) {
#if defined(_COLLECT_SEARCH_STATS)
		results.tt_cut_cnt++;
#endif
		return tt_entry.score;
	}

	if (!depth) {
		return quiesce<QUIESCE_NODE | NmNodeType>(pos, limits, results, node,
								 				  alpha, beta,
								 				  depth,
								 				  ply);
	}

	results.nodes_cnt++;

#if defined(_COLLECT_SEARCH_STATS)
	results.pvnodes_cnt += IsPV;
	results.npvnodes_cnt += !IsPV;
#endif

	if constexpr (Root)
		node->check = pos.isInCheck(node->side2move);

	NodeInfo* const preroot = _tree_stack.getPreRootNode();
	NodeInfo* const next_node = node + 1;

	node->move = Move32b::Null;
	node->eval = tt_entry.eval;
	node->improving_rate = 0.f;

	Score corr_eval = Score::Undef;

	/* Razoring -
	*  if we're at lower depth and the eval is really low
	*  it means there is high probability no move can increase the alpha bar.
	*  To ensure our intuition, we dive into quiescence search to verify the position.
	*  If we fail low, we've got a cutoff.
	*/
	if constexpr (!Root and !IsPV) {
		if (!node->check and
			depth <= RazorDepth)
		{
			if (!node->eval.isValid()) {
				node->eval = evaluate<NmNodeType>(pos, _tree_stack, node, preroot, node->side2move, results);
			}
			
			corr_eval = adjustEvalScore(node->eval, tt_entry.score);

			if (corr_eval + RazorBaseDelta + RazorMultDelta * depth < alpha) {
				const Score qscore = quiesce<QUIESCE_NODE | NON_PV_NODE>(pos, limits, results, node,
													      		  	 	 alpha - 1, alpha,
													      		  	 	 depth - 1,
													      		  	 	 ply + 1);
				
				if (qscore < alpha) {
					return qscore;
				}
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
	if constexpr (!Root and IsPV) {
		if (depth >= IidDepth and tt_move.isNull()) {
			_UNUSED const Score iid_score =
				negaMax<false, NmNodeType, false>(pos, limits, results, game, node,
												  alpha, beta,
												  4 * depth / IidDepthDiv,
												  ply);

			TTEntry iid_entry;
			iid_entry.eval = Score::Undef;
			iid_entry.move = Move16b::Null;
			iid_entry.score = Score::Undef;

			_UNUSED const bool iid_tt_hit = _tt.probe(iid_entry, hash, alpha, beta, depth);
			
			ttm32b = unpacked(pos, iid_entry.move);
			tt_move = ttm32b.isPseudoLegal(pos) ? ttm32b 
												: Move32b::Null;

			if (!tt_move.isNull() and iid_entry.eval.isValid()) {
				node->eval = iid_entry.eval;
			}
		}
	}

	/* Dynamic Improving implementation -
	*  we're clamping improvement rate to range [-1., 1.]
	*/

	if (!Root and !IsPV and !node->check) {

		if (node->eval.isValid() or depth <= DynImprovementDepth) {

			if (!node->eval.isValid()) {
				node->eval = evaluate<NmNodeType>(pos, _tree_stack, node, preroot, node->side2move, results);
			}

			const NodeInfo* prev_eval_node = nullptr;

			if (node - preroot > 2 and (node - 2)->eval.isValid())
				prev_eval_node = node - 2;
			else if (node - preroot > 4 and (node - 4)->eval.isValid())
				prev_eval_node = node - 4;
		
			if (prev_eval_node) {
				const Score diff = node->eval - prev_eval_node->eval;
				node->improving_rate = std::clamp(prev_eval_node->improving_rate + static_cast<float>(diff) / ImprovingRate, 
												  -1.f, 1.f);
			}
		}
	}

	/* Reverse Futility Pruning (Static Null Move Pruning) -
	*  basically, when we're doing very well, we can prune.
	*  Idea similar to Standing Pat cutoff in Q-Search.
	*/
	if constexpr (!Root and !IsPV) {
		if (!node->check and
			depth <= RfpDepth and
			(tt_move.isNull() or tt_move.isQuiet()))
		{
			if (!node->eval.isValid()) {
				node->eval = evaluate<NmNodeType>(pos, _tree_stack, node, preroot, node->side2move, results);
			}

			if (node->eval - (-node->improving_rate / RfpImprovingSink + 1.) * RfpMultDelta * depth >= beta) {
				const Score reduced_eval = (node->eval + beta) / 2;
				return reduced_eval;
			}
		}
	}

    nn::AccumulatorCache* const accum_cache = &node->cluster.accum_cache;

	/* Null Move Pruning -
	*  if we're doing so well even after not making a move, we must be winning here.
	*  So we can do beta cutoff.
	*/
	if constexpr (!Root and NullMove) {

		if (!node->check and 
			depth >= NullDepth) {

			if (!node->eval.isValid()) {
				node->eval = evaluate<NmNodeType>(pos, _tree_stack, node, preroot, node->side2move, results);
			}

			if (node->eval - (-node->improving_rate / NullImprovingSink + 1.) * NullMargin * depth >= beta) {

				assert(prev_node->move != Move32b::Null);
				
				pos.makeNull(node->state, accum_cache);

				AccumulatorCluster* const curr_cluster = &node->cluster;
				AccumulatorCluster* const next_cluster = curr_cluster->next_cluster;
				AccumulatorCluster* const prev_cluster = curr_cluster->prev_cluster;

				assert(curr_cluster->prev_cluster->next_cluster == curr_cluster);

				node->move = Move32b::Null;
				next_cluster->prev_cluster = prev_cluster;
				
				const int nm_depth = getNullSearchDepth(node->eval, beta, depth);

				const Score score = -negaMax<false, NON_PV_NODE, !NullMove>(pos, limits, results, game, next_node,
																			-beta, -beta + 1, 
																			nm_depth, 
																			ply + 1);
				pos.unmakeNull(node->state);
				next_cluster->prev_cluster = curr_cluster;

				/* Unless Null Move Pruning is not handled properly in the endgame, 
				*  verification search is just needed to prevent Zugzwang.
				*/
				if (score >= beta) {
					const Score verify = negaMax<false, NON_PV_NODE, !NullMove>(pos, limits, results, game, node,
																				beta - 1, beta, 
																				nm_depth, 
																				ply);
					
					if (verify >= beta) {
						_tt.write(hash, 
								nm_depth, ply, 
								TTEntry::UPPERBOUND, 
								verify, packed(tt_move), node->eval, 
								results);

						return verify;
					}
				}
			}
		}
	}
		
	node->move_picker.clear<OrderPolicy>();
	node->move_picker.setHashMove(tt_move);
	node->can_move 	 	 = false;
	node->score 	 	 = 0;
	node->best_move  	 = Move32b::Null;
	node->best_score 	 = -Score::Infinity;
	node->moves_searched = 0;
	node->state 		 = pos.getIrreversibleState();
	node->bound 		 = TTEntry::LOWERBOUND;

	for (node->move_index = 0; 
		 node->move_picker.nextMove<OrderPolicy, Root>(_tree_stack, pos, node->move); 
		 node->move_index++) 
	{

		const uint64_t next_hash = pos.likelyZobristKeyAfterMove(node->move);
		_tt.prefetchBucket(next_hash);

		/* Futility Pruning -
		*  at shallow depths, skip moves that aren't like to rise alpha.
		*/
		if (!node->check and
			depth <= FutilityDepth and
			node->moves_searched >= FutilityMoveCount and
			node->move.isQuiet() and
			!node->move.isQueenPromotion())
		{
			if (!node->eval.isValid()) {
				node->eval = evaluate<NmNodeType>(pos, _tree_stack, node, preroot, node->side2move, results);
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

		if (pos.make(node->move, accum_cache)) {
			node->can_move = true;

			const enumColor next_side = !node->side2move;

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
				node->score = -negaMax<false, NmNodeType, true>(pos, limits, results, game, next_node,
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
						node->move_picker.updateQuietsHistory(node->best_move, node->side2move, depth);
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
		node->best_score = node->check ? -Score::Mate + ply : getDrawScore<Root>(node);
	}

	if (!node->best_score.isMateScore() or tt_entry.isEmpty()) {
		const Move16b bestmove16b = packed(node->best_move);

		_tt.write(hash, 
				  depth, ply, 
				  node->bound, 
				  node->best_score, bestmove16b, node->eval, 
				  results);
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

template <Search::enumNode QNodeType>
Score Search::quiesce(Position& pos, 
					  SearchLimits& limits, SearchResults& results, 
					  NodeInfo* node, 
					  Score alpha, Score beta, 
					  int depth, int ply) 
{
	assert(alpha < beta);

	static constexpr OrderType QuiescentOrderPolicy = QUIESCENT;
	static constexpr bool	   IsPV = QNodeType & PV_NODE; 
	static constexpr bool	   Root = false;
	static constexpr bool	   SeeNonExactScore = false;
	
	node->side2move = pos.getTurn();

	if (isInsufficientMaterial(pos))
		return getDrawScore<Root>(node);

	if ((results.nodes_cnt & CheckNodeCount) == 0 and !limits.isTimeLeft()) {
		return -Score::Undef;
	}
    
	if (!limits.anyNodesLeft(results.nodes_cnt) or
        !limits.anyQuiesceNodesLeft(results.qnodes_cnt)) {
        return -Score::Undef;
    }

	const NodeInfo* const preroot = _tree_stack.getPreRootNode();
	
	if (ply >= static_cast<int>(MaxSelDepth)) _UNLIKELY {
		return evaluate<QNodeType>(pos, _tree_stack, node, preroot, node->side2move, results);
	}

#if defined(_COLLECT_SEARCH_STATS)
	results.tt_probe_cnt++;
	results.qtt_probe_cnt++;
#endif

#if defined(_TT_PROBE_QSEARCH)
	TTEntry tt_entry;
	tt_entry.eval = Score::Undef;
	tt_entry.move = Move16b::Null;
	tt_entry.score = Score::Undef;

	const uint64_t hash = pos.getZobristKey();
	const uint8_t probe_depth = static_cast<uint8_t>(std::max(0, depth));

	const bool tt_hit = _tt.probe(tt_entry, hash, alpha, beta, probe_depth);
	const bool exact_hit = (!IsPV and tt_hit) or 
						   (IsPV and tt_hit and tt_entry.bound == TTEntry::EXACT);

	if (exact_hit and depth <= QProbeDepth) {
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

#if defined(_TT_PROBE_QSEARCH)
	node->eval = !tt_entry.eval.isValid() _LIKELY ? evaluate<QNodeType>(pos, _tree_stack, node, preroot, node->side2move, results)
												  : tt_entry.eval;
#else
	node->eval = evaluate<QNodeType>(pos, _tree_stack, node, preroot, node->side2move, results);
#endif

	/* Delta Pruning -
	*  when no move has any chance to raise alpha
	*  then prune all of the branches.
	*/
	if (node->eval + QMaterialDelta < alpha)
		return alpha;
	
	/* Standing Pat Cutoff -
	*  when we're already above the beta, we can make a cutoff.
	*/
	else if (node->eval > alpha) {
		if (node->eval >= beta) 
			return beta;

		node->score = alpha = node->eval;
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

    nn::AccumulatorCache* const accum_cache = &node->cluster.accum_cache;

	node->moves_searched = 0;
	node->state = pos.getIrreversibleState();

	for (node->move_index = 0; 
		node->move_picker.nextMove<QuiescentOrderPolicy, Root>(_tree_stack, pos, node->move); 
		node->move_index++) 
	{
		
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

		if (pos.make(node->move, accum_cache)) {
			node->score = -quiesce<QNodeType>(pos, limits, results, node + 1,
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

template <bool Root>
_FORCEINLINE Score Search::getDrawScore(_UNUSED const NodeInfo* node) {
	return Score::Draw;
}

// TODO: smarter extension calculation
INLINE int Search::calculateExtension(Position& pos, NodeInfo* node) {
	_declUnused(pos);
	NodeInfo* next_node = node + 1;
	return next_node->check;
}

template <Search::enumNode NodeType>
INLINE Score Search::evaluate(const Position& pos,
							  TreeStack& _tree_stack,
							  NodeInfo* node,
							  const NodeInfo* preroot,
							  enumColor side2move, 
							  SearchResults& results)
{

#if defined(_COLLECT_SEARCH_STATS)
	if constexpr (NodeType & QUIESCE_NODE)
		results.qeval_cnt++;
		
	else
		results.nmeval_cnt++;
#else
	_declUnused(results);
#endif

    AccumulatorCluster* curr_accum_cluster = &node->cluster;
    const AccumulatorCluster* prev_accum_cluster = curr_accum_cluster->prev_cluster;

	if (prev_accum_cluster->accum_cache.isDirty()) {
		const AccumulatorCluster* clean_accum_cluster = _tree_stack.getCleanAccumulatorCluster(curr_accum_cluster, preroot);
		_tree_stack.updateDirtyAccumulators(clean_accum_cluster, curr_accum_cluster);
	}

	assert(prev_accum_cluster->accum_cache.isClean());

    const nn::Accumulator& prev_accum = prev_accum_cluster->accum_cache.accum;

#if defined(_VERIFY_NN)
	ASSERT(nn::Accumulator::verify(prev_accum, pos), "Accumulator verification failed");
#else
	_declUnused(pos);
#endif

	const Score eval = nn::NEval::evaluate(nn::GlobPackedNetwork, prev_accum, side2move);
	const int scaled_eval = 8 * static_cast<int>(eval) / NNEvalScale;

	assert(std::abs(scaled_eval) < Score::Mate);

	return static_cast<Score>(scaled_eval);
}

_FORCEINLINE Score Search::adjustEvalScore(Score eval, Score tt_score) {
	assert(eval.isValid());

	if (!tt_score.isValid() or eval.isMateScore())
		return eval;

	const Score tt_eval_diff = tt_score - eval;
	return eval + tt_eval_diff / TTEvalCorrRate;
}

_FORCEINLINE int Search::getNullSearchDepth(Score eval, Score beta, int depth) {
	const float diff_reduction = std::min(1.31f, static_cast<float>(eval - beta) / NullDiffScale);
	const float diff_scale = 1.5f + 1.f / (diff_reduction - 2.f);
	assert(8 * depth / NullReduction < depth); // don't return same depth, we could stuck in a loop
	return std::max<int>(std::lroundf(8.f * diff_scale * depth / NullReduction), 1);
}

template <bool IsPV>
bool Search::isRepetitionCycle(const Position& pos,
							   const FullInfoRecord& game,
							   const NodeInfo* node,
							   int ply,
							   SearchResults& results)
{
#if defined(_COLLECT_SEARCH_STATS)
	results.rep_call_cnt++;
#else
	_declUnused(results);
#endif

	const uint64_t curr_hashkey = pos.getZobristKey();
	const NodeInfo* prev_node = node;

	int rep_cnt = 0;

	for (int p = ply - 1; 
		 p >= 0 and p >= ply - pos.getHalfmoveClock(); 
		 p -= 2) 
	{
		prev_node--;

		if (prev_node->move.isNull() or prev_node->move.isIrreversible())
			return false;

		prev_node--;

		if (prev_node->move.isNull() or prev_node->move.isIrreversible())
			return false;

		if (curr_hashkey == prev_node->state.hash_key) {

			if constexpr (!IsPV)
				return true;

			if (++rep_cnt >= 2)
				return true;
		}
	}

	if (ply >= pos.getHalfmoveClock())
		return false;

	const int game_rep_depth = 50 - ply;
	const int curr_halfclock = static_cast<int>(game.currentHalfCount());

	// iterate through only a subset of all game moves
	for (int halfclock = curr_halfclock - 1;
		 halfclock >= 0 and curr_halfclock - halfclock <= game_rep_depth;
		 halfclock--) 
	{
		const Move32b move = game.getPrevMove(halfclock);

		if (move.isIrreversible())
			return false;

		if (((curr_halfclock - halfclock) & 1) and 
			curr_hashkey == game.getPrevKey(halfclock)) {

			if constexpr (!IsPV)
				return true;

			if (++rep_cnt >= 2)
				return true;
		}
	}
	
	return false;
}

/*
* Upcoming repetition detection based on a paper:
* http://web.archive.org/web/20201107002606/https://marcelk.net/2013-04-06/paper/upcoming-rep-v2.pdf
*/

bool Search::canRepetitionDraw(const Position& pos,
							   const NodeInfo* node,
							   int ply)
{
	if (pos.getHalfmoveClock() < 3)
		return false;

	const uint64_t curr_hash = pos.getZobristKey();
	const NodeInfo* prev_node = node - 1;

	if (prev_node->move.isNull() or prev_node->move.isIrreversible())
		return false;

	size_t idx = static_cast<size_t>(-1);

	for (int p = ply - 1; 
		 p >= 2 and p >= ply - pos.getHalfmoveClock() + 2; 
		 p -= 2) 
	{
		prev_node--;
		
		if (prev_node->move.isNull() or prev_node->move.isIrreversible())
			break;
		
		prev_node--;

		if (prev_node->move.isNull() or prev_node->move.isIrreversible())
			break;

		assert(prev_node->side2move != node->side2move);
		assert(prev_node->state.hash_key);

		const uint32_t move_hash = static_cast<uint32_t>(prev_node->state.hash_key ^ curr_hash);

		if ((idx = CuckooTables::cuckooIndex1(move_hash), 
				_cuckoo_tables.getMoveHash(idx) == move_hash) or
			(idx = CuckooTables::cuckooIndex2(move_hash), 
				_cuckoo_tables.getMoveHash(idx) == move_hash)) {

			// simplified verification for obtained cuckoo move

			const Move16b move16b = _cuckoo_tables.getMove16b(idx);

			const Square org = move16b.getOrigin();
			const Square dst = move16b.getTarget();

			const BitBoard occupied = pos.getOccupied();
			
			if (!(occupied & (BitBoard(org) | BitBoard(dst))))
				continue;
				
			if (!(onlyBetween(org, dst) & occupied) or move16b.isKnight())
				return true;
		}
	}

	return false;
}

bool Search::isInsufficientMaterial(const Position& pos) {

	if (pos.getPawns() or pos.getQueens())
		return false;

	const int piece_cnt = pos.getOccupied().popCount();

	// King versus King
	if (piece_cnt == 2)
		return true;

	// King + Bishop versus King
	if (piece_cnt == 3 and
		pos.getBishops() /* .popCount() == 1 */)
		return true;

	// King + Knight versus King
	if (piece_cnt == 3 and
		pos.getKnights() /*.popCount() == 1 */)
		return true;

	// King + Bishop versus King + Bishop with same-color Bishops
	if (piece_cnt == 4 and
		pos.getBishopsBySide(WHITE).isSingleBit() and
		pos.getBishopsBySide(BLACK).isSingleBit()) {

		// check colors matching

		const BitBoard bishops = pos.getBishops();
		const BitBoard white_square_bishops = bishops & BitBoard::White_Squares;

		return white_square_bishops == bishops or white_square_bishops.isEmpty();
	}

	return false;
}

template Move32b Search::bestMove<Search::SEARCH_FULL_INFO>(Position&, const FullInfoRecord&, SearchLimits);
template Move32b Search::bestMove<Search::SEARCH_SHORT_INFO>(Position&, const FullInfoRecord&, SearchLimits);
template Move32b Search::bestMove<Search::SEARCH_ONLY_BM_INFO>(Position&, const FullInfoRecord&, SearchLimits);
template Move32b Search::bestMove<Search::SEARCH_NO_INFO>(Position&, const FullInfoRecord&, SearchLimits);
