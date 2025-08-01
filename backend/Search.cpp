#include "Search.hpp"
#include "Eval.hpp"
#include "MoveGen.hpp"
#include "Time.hpp"
#include "TranspositionTable.hpp"

#include <sstream>

INLINE bool SearchLimits::isTimeLeft() {
	return !search_time or timer.duration() < search_time;
}

INLINE void SearchResults::registerBestMove(Move32b move) {
	best_move = move;
}

INLINE void SearchResults::printBestMove() {
	ASSERT(!best_move.isNull(), "Null bestmove error");

	std::cout << "bestmove ";
	best_move.print();
	std::cout << '\n';
}

INLINE void SearchResults::print(const Search* search, const Position& pos, TranspositionTable& tt) {
	const uint64_t nps = static_cast<uint64_t>((nodes_cnt * 1000.f) / (duration ? duration : 1));

	std::cout << "info depth " << depth
		<< " seldepth " << seldepth
		<< " score " << score_cp.toStr()
		<< " nodes " << nodes_cnt
		<< " time " << duration
		<< " nps " << nps
		<< " hashfull " << tt.getHashfull()
		<< " pv ";

	Position cpy = pos;

	while (depth--) {
		TTEntry tt_entry;
		const bool tt_hit = search->_tt.probe(tt_entry, cpy.getZobristKey(), -Score::Infinity, +Score::Infinity, depth);

		Move32b pv_move = unpacked(cpy, tt_entry.move);

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
	_tt.clearHashfull();
	MoveOrder::clearQuietsHistory();
}

TreeStack::TreeStack() {
	_stack = reinterpret_cast<NodeInfo*>(alignedMalloc(_Count * sizeof(NodeInfo),  CACHELINE_SIZE));
	ASSERT(_stack != nullptr, "Failed to allocate memory");
}

TreeStack::~TreeStack() {
	alignedFree(_stack);
}

INLINE const NodeInfo* TreeStack::getNode(unsigned ply) const {
	assert(ply < _Count);
	return _stack + ply;
}

INLINE NodeInfo* TreeStack::getRootNode() {
	return _stack;
}

template <bool PrintFullInfo>
Move32b Search::bestMove(Position& pos, const Game& game, SearchLimits limits) {
	ASSERT(1 <= limits.depth and limits.depth < MaxDepth, "Invalid depth");

	limits.timer.go();
	limits.search_time = TimeMan::searchTime(pos, limits);
	_tt.newGeneration();

	const Move32b bm = iterativeDeepening<PrintFullInfo>(pos, game, limits);
	return bm;
}

Move32b Search::_bestMove_unittest(Search& search, Position& pos, const Game& game, SearchLimits limits) {
	return search.bestMove<false>(pos, game, limits);
}

template <bool PrintFullInfo>
Move32b Search::iterativeDeepening(Position& pos, const Game& game, SearchLimits& limits) {
	SearchResults search_results;
		
	NodeInfo* root = _tree_stack.getRootNode();

	for (unsigned d = 1; d <= limits.depth; d++) {
		search_results.depth = d;

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
	const Score score = -negaMax<true>(pos, limits, results, game, _tree_stack.getRootNode(), 
									   -Score::Mate, +Score::Mate, 
									   results.depth, 
									   0);

	if (results.depth > 1 and !limits.isTimeLeft())
		return false;

	results.duration = limits.timer.duration();

	if constexpr (PrintFullInfo)
		results.print(this, pos, _tt);

	return true;
}

template <bool Root, Search::enumNode NodeType, bool NullMove>
Score Search::negaMax(Position& pos, SearchLimits& limits, SearchResults& results, const Game& game, NodeInfo* node,
					  Score alpha, Score beta, int depth, int ply) 
{
	assert(0 <= depth and depth < MaxDepth - 1);
	assert(alpha < beta);

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

	if (!Root and pos.halfmoveClock() >= 100 or isRepetitionCycle<IsPV>(pos, game, node - 1, ply)) {
		return Score::Draw;
	}
	else if (!Root and (results.nodes_cnt & _CheckNodeCount) == 0 and !limits.isTimeLeft()) {
		return -Score::Undef;
	}
	else if (!depth) {
		return quiesce<NodeType>(pos, limits, results, node,
								 alpha,
								 beta,
								 depth,
								 ply);
	}

	const uint64_t hash = pos.getZobristKey();

	TTEntry tt_entry;
	const bool tt_hit = _tt.probe(tt_entry, hash, alpha, beta, depth);

	if constexpr (!Root and !IsPV) {
		if (tt_hit) return tt_entry.score;
	}
	else if constexpr (!Root) {
		if (tt_hit and tt_entry.bound == TTEntry::EXACT) 
			return tt_entry.score;
	}
	
	results.nodes_cnt++;

	const enumColor side2move = pos.getTurn();

	if constexpr (Root)
		node->check = pos.isInCheck(side2move);

	node->move = Move32b::Null;
	node->static_eval = Score::Undef;

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
			node->static_eval = _eval.staticEval(pos);

			if (node->static_eval + RazorBaseDelta + RazorMultDelta * depth < alpha) {
				const Score qscore = quiesce<NodeType>(pos, limits, results, node + 1,
													   alpha - 1, alpha,
													   depth,
													   ply);

				if (qscore < alpha)
					return qscore;
			}
		}
	}

	Move32b ttm32b = unpacked(pos, tt_entry.move);
	Move32b tt_move = ttm32b.isPseudoLegal(pos) ? ttm32b : 
												  Move32b::Null;

	TTEntry iid_entry;

	if constexpr (IsPV) {
		if (depth >= IidDepth and tt_move.isNull()) {
			_UNUSED const Score iid_score =
				negaMax<false, NodeType, NullMove>(pos, limits, results, game, node,
												   alpha, beta,
												   depth >> IidDivShift,
												   ply);

			_tt.probe(iid_entry, hash, alpha, beta, depth);

			ttm32b = unpacked(pos, iid_entry.move);
			tt_move = ttm32b.isPseudoLegal(pos) ? ttm32b :
												  Move32b::Null;
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
			if (!node->static_eval.isValid())
				node->static_eval = _eval.staticEval(pos);

			if (node->static_eval - RfpMultDelta * depth >= beta)
				return node->static_eval - (depth << 6);
		}
	}

	NodeInfo* next_node = node + 1;

	/* Null Move Pruning -
	*  if we're doing so well even after not making a move, we must be winning here.
	*  So we can do beta cutoff.
	*/
	if constexpr (NullMove) {
		if (!node->check and depth >= NullReduction + 1) {
			pos.makeNull(node->state);
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

	while (node->move_picker.nextMove<OrderPolicy, Root>(_tree_stack, pos, node->move)) 
	{

		/* Futility Pruning -
		*  at shallow depths, skip moves that aren't like to rise alpha.
		*/
		if (!node->check and
			depth <= FutilityDepth and
			node->moves_searched > 0 and
			node->move.isQuiet() and
			!node->move.isQueenPromotion())
		{
			if (!node->static_eval.isValid())
				node->static_eval = _eval.staticEval(pos);

			if (node->static_eval + FutilityDelta * depth * depth < alpha) {
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

		if (pos.make(node->move)) {
			node->can_move = true;

			const enumColor next_side = !side2move;

			(node + 1)->check = pos.isInCheck(next_side);
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

					if constexpr (NodeType == NON_PV_NODE)
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

					break;
				}

				node->bound = TTEntry::EXACT;
				alpha = node->score;
			}
		}
		else if (!limits.isTimeLeft()) {
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

	if (!node->best_score.isMateScore()) {
		const Move16b bestmove16b = packed(node->best_move);
		_tt.write(hash, depth, ply, node->bound, node->best_score, bestmove16b, results);
	}

	next_node->move_picker.setKillerMove(Move32b::Null);

	if constexpr (Root) {
		results.score_cp = node->best_score;
	}

	return node->best_score;
}

template Score Search::negaMax<true>(Position& pos, SearchLimits& limits, SearchResults& results, const Game& game, NodeInfo* node,
									 Score alpha, Score beta, int depth, int ply);
template Score Search::negaMax<false>(Position& pos, SearchLimits& limits, SearchResults& results, const Game& game, NodeInfo* node,
									  Score alpha, Score beta, int depth, int ply);

template <Search::enumNode NodeType>
Score Search::quiesce(Position& pos, SearchLimits& limits, SearchResults& results, NodeInfo* node, 
					  Score alpha, Score beta, int depth, int ply) 
{
	static constexpr OrderType QuiescentOrderPolicy = QUIESCENT;
	static constexpr bool	   IsPV = NodeType == PV_NODE;
	static constexpr bool	   Root = false;
	static constexpr bool	   SeeNonExactScore = false;

	static constexpr Score	   MaterialDelta = 900;

	if ((results.nodes_cnt & _CheckNodeCount) == 0 and !limits.isTimeLeft()) {
		return -Score::Undef;
	}
	else if (ply >= MaxSelDepth) _UNLIKELY {
		return _eval.staticEval(pos);
	}

	const uint64_t hash = pos.getZobristKey();
	const uint8_t probe_depth = static_cast<uint8_t>(std::max(0, depth));

	TTEntry tt_entry;
	const bool tt_hit = _tt.probe(tt_entry, hash, alpha, beta, probe_depth);

	if constexpr (!IsPV) {
		if (tt_hit and depth <= 0)
			return tt_entry.score;
	}
	else {
		if (tt_hit and 
			depth <= 0 and 
			tt_entry.bound == TTEntry::EXACT)
			return tt_entry.score;
	}
	

	results.nodes_cnt++;
	results.seldepth = std::max(results.seldepth, static_cast<unsigned>(ply + 1));

	assert(alpha < beta);

	const enumColor side2move = pos.getTurn();
	const Score stand_pat = _eval.staticEval(pos);

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
	
	const Move16b ttm16b = tt_entry.move;

	if ((!IsPV or tt_entry.bound != TTEntry::UPPERBOUND) and 
		(isCapturePacked(pos, ttm16b) or ttm16b.isQueenPromotion())) 
	{
		const Move32b ttm32b = unpacked(pos, tt_entry.move);
		const Move32b tt_move = ttm32b.isPseudoLegal(pos) ? ttm32b :
															Move32b::Null;
		node->move_picker.setHashMove(tt_move);
	}

	node->moves_searched = 0;
	node->state = pos.getIrreversibleState();

	while (node->move_picker.nextMove<QuiescentOrderPolicy, Root>(_tree_stack, pos, node->move)) 
	{
		/* Static Exchange Evaluation Pruning -
		*  ignore losing captures, as they aren't likely to rise alpha anyway.
		*/
		if (node->move.isCapture() and
			!node->move.isEnPassant() and 
			!node->move.isPromotion())
		{
			const Square org = node->move.getOrigin();
			const Square dst = node->move.getTarget();
			const Piece::enumType vic = pos.pieceOn(node->move.getTarget(), pos.getOppositeTurn());
			const Piece::enumType piece = node->move.getPiece();

			const int capt_see_score = pos.StaticExchangeEval<SeeNonExactScore>(org, dst, vic, piece);

			if (capt_see_score < 0)
				continue;
		}

		if (pos.make(node->move)) {
			node->score = -quiesce<NodeType>(pos, limits, results, node + 1,
											 -beta, -alpha,
											 depth - 1,
											 ply + 1);

			node->moves_searched++;
		}

		pos.unmake(node->move, node->state);

		if (limits.isTimeLeft() and 
			node->move.isLegalMoved() and 
			node->score > alpha) 
		{
			if (node->score >= beta)
				return beta;
			
			alpha = node->score;
		}
		else if (!limits.isTimeLeft()) {
			return -Score::Undef;
		}
	}

	return alpha;
}

// TODO: smarter extension calculation
INLINE int Search::calculateExtension(Position& pos, NodeInfo* node) {
	NodeInfo* next_node = node + 1;
	return next_node->check;
}

template <bool IsPV>
bool Search::isRepetitionCycle(const Position& pos, const Game& game, NodeInfo* node, int ply) {
	const int my_ply = ply;
	const uint64_t my_hashkey = pos.getZobristKey();

	int rep_cnt = 0;

	static constexpr int SearchRepDepth = 15;
	static_assert(SearchRepDepth % 2);

	for (ply = ply - 1; ply >= 0; ply--, node--) {
		const Move32b move = node->move;

		if (((my_ply - ply) & 1) == 1)
			continue;
		else if (move.isIrreversible())
			return false;
		else if (my_hashkey == node->state.hash_key /* previous hashkey */) 
		{
			if constexpr (IsPV) 
				return true;

			rep_cnt++;

			if (rep_cnt >= 2)
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

		if ((i & 1) == 0)
			continue;
		else if (move.isIrreversible())
			return false;
		else if (my_hashkey == game.getPrevKey(cnt))
		{
			if constexpr (IsPV)
				return true;

			rep_cnt++;

			if (rep_cnt >= 2)
				return true;
		}
	}

	return false;
}

template Move32b Search::bestMove<true>(Position&, const Game&, SearchLimits);
template Move32b Search::bestMove<false>(Position&, const Game&, SearchLimits);
