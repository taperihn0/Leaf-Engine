#pragma once

#include "Common.hpp"
#include "Position.hpp"
#include "Move.hpp"
#include "MoveOrder.hpp"
#include "StaticEval.hpp"
#include "Game.hpp"
#include "Time.hpp"
#include "Score.hpp"
#include "TranspositionTable.hpp"
#include "Accumulator.hpp"

#include <numeric>

class Search;

struct SearchLimits {
    bool isTimeLeft();

    // anyNodesLeft compares current any-node count (both search and quiescent nodes)
    // and returns whether given number is below any-node threshold.
    bool anyNodesLeft(ull nodes_so_far);

    // anyQuiesceNodesLeft compares current quiescent nodes count
    // and returns whether given number is below quiescent-node threshold.
    bool anyQuiesceNodesLeft(ull qnodes_so_far);

	time_ms_t depth		  = 0,
			  wtime		  = 0,
			  btime		  = 0;
	time_ms_t winc		  = 0, 
			  binc		  = 0,
			  search_time = 0;
    ull       nodes       = 0;
    ull       qnodes      = 0;
	Timer     timer;
};

struct SearchResults {
	void registerBestMove(Move32b move);

	void printBestMove();
	void print(const Search* search, const Position& pos, TranspositionTable& tt);
	void printShort();

#if defined (_COLLECT_SEARCH_STATS)
	void printSearchStats();
#endif

	unsigned  depth				= 0,
			  seldepth			= 0;
	Score	  score_cp			= 0;
	ull		  nodes_cnt			= 0,
              qnodes_cnt        = 0;
	size_t	  tt_entries		= 0;
	Move32b   best_move			= Move32b::Null;
	time_ms_t duration			= 0;

#if defined (_COLLECT_SEARCH_STATS)
	ull       pvnodes_cnt		= 0,
			  npvnodes_cnt		= 0;

	ull		  tt_probe_cnt		= 0,
			  qtt_probe_cnt		= 0,
			  tt_cut_cnt		= 0,
			  qtt_cut_cnt		= 0,
			  qttmove_probe_cnt = 0;

	ull		  ttmove_cut_cnt	= 0,
			  qttmove_cut_cnt	= 0;

	ull		  beta_cut_cnt		= 0,
			  qbeta_cut_cnt		= 0;

	ull		  move_cut_cnt[MaxNodeMoves] = {};

	ull 	  nmeval_cnt 		= 0;
	ull 	  qeval_cnt 		= 0;
#endif
};

struct NodeInfo {
	void clear();

	MoveOrder					move_picker;
	Position::IrreversibleState state;
	Move32b						move;
	Move32b						best_move;
	Score						score;
	bool						can_move;
	Score						best_score;
	bool						check;
	unsigned					ply;
	uint8_t						moves_searched;
	uint8_t						move_index;
	TTEntry::Bound				bound;
	nn::AccumulatorCache 		accum_cache;
};

class TreeStack {
public:
	TreeStack();
	~TreeStack();

	void init(MoveOrderHistoryTables* history_buffer);

	NodeInfo* getRootNode();
	NodeInfo* getPreRootNode();
	const NodeInfo* getNode(unsigned ply) const;
private:
	static constexpr size_t _Count = MaxSelDepth;
	NodeInfo* _stack;
};

class Search {
public:
	friend struct SearchResults;

	enum enumNode : int8_t {
		PV_NODE             = 1,
		NON_PV_NODE         = 2,
		QUIESCE_NODE        = 4,
		QUIESCE_PV_NODE     = QUIESCE_NODE | PV_NODE,
		QUIESCE_NON_PV_NODE = QUIESCE_NODE | NON_PV_NODE,
	};

    enum enumInfoLevel : int8_t {
        SEARCH_FULL_INFO    = 0,
        SEARCH_SHORT_INFO   = 1,
        SEARCH_ONLY_BM_INFO = 2,
        SEARCH_NO_INFO      = 3,
    };

	Search(TranspositionTable&& tt);
	~Search();

	template <enumInfoLevel InfoLevel = SEARCH_FULL_INFO>
	Move32b bestMove(Position& pos, const FullInfoRecord& game, SearchLimits limits);
	static Move32b _bestMove_unittest(Search& search, 
									  Position& pos, 
									  const FullInfoRecord& game, 
									  SearchLimits limits);
	
	void clearHashTT();
	void resizeHashTT(size_t tt_size_mb);
	void registerNewGame();
private:
	template <enumInfoLevel InfoLevel>
	Move32b iterativeDeepening(Position& pos, const FullInfoRecord& game, SearchLimits& limits);

	template <enumInfoLevel InfoLevel>
	bool search(Position& pos, 
				const FullInfoRecord& game, 
				SearchLimits& limits, SearchResults& results);

	template <bool Root, enumNode NmNodeType = PV_NODE, bool NullMove = !Root>
	Score negaMax(Position& pos, 
				  SearchLimits& limits, SearchResults& results, 
				  const FullInfoRecord& game, 
				  NodeInfo* node,
				  Score alpha, Score beta, 
				  int depth, int ply);

	template <Search::enumNode QNodeType>
	Score quiesce(Position& pos, 
				  SearchLimits& limits, SearchResults& results, 
				  NodeInfo* node, 
				  Score alpha, Score beta, 
				  int depth, int ply);

	int calculateExtension(Position& pos, NodeInfo* node);
	
	const nn::AccumulatorCache* getCleanAccumulator(const NodeInfo* node, const NodeInfo* preroot);

	void updateDirtyAccumulators(const nn::AccumulatorCache* clean_accum,
								 nn::AccumulatorCache* last_accum);

	template <enumNode NodeType>
	Score evaluate(const Position& pos,
				   const nn::Accumulator* prev_accum, 
				   enumColor side2move, 
				   SearchResults& results);

	template <bool IsPV>
	bool isRepetitionCycle(const Position& pos, 
						   const FullInfoRecord& game, 
						   NodeInfo* node, 
						   int ply);

	static constexpr uint64_t _CheckNodeCount = 4096;

	TreeStack 		   		_tree_stack;
	TranspositionTable 		_tt;
	// Each Search instance should have own history buffer with tables 
	// for very MoveOrder in TreeStack.
	// Also, Search class in responsible for allocation and deallocation.
	MoveOrderHistoryTables* _history_buff;
};

INLINE constexpr Search::enumNode operator|(Search::enumNode node0, Search::enumNode node1) {
	return static_cast<Search::enumNode>(static_cast<int>(node0) | static_cast<int>(node1));
}
