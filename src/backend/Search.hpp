#pragma once

#include "Common.hpp"
#include "Position.hpp"
#include "Move.hpp"
#include "MoveOrder.hpp"
#include "Game.hpp"
#include "Time.hpp"
#include "Score.hpp"
#include "TranspositionTable.hpp"
#include "Accumulator.hpp"
#include "Cuckoo.hpp"

struct SearchLimits {
    bool isTimeLeft();

    // anyNodesLeft compares current any-node count (both search and quiescent nodes)
    // and returns whether given number is below any-node threshold.
    bool anyNodesLeft(ull nodes_so_far);

    // anyQuiesceNodesLeft compares current quiescent nodes count
    // and returns whether given number is below quiescent-node threshold.
    bool anyQuiesceNodesLeft(ull qnodes_so_far);

	int 	  depth		  = 0;
	time_ms_t wtime		  = 0,
			  btime		  = 0;
	time_ms_t winc		  = 0, 
			  binc		  = 0,
			  search_time = 0;
    ull       nodes       = 0;
    ull       qnodes      = 0;
	Timer     timer;
};

class Search;

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

	ull 	  rep_call_cnt		= 0;
	ull 	  rep_cnt			= 0;

	ull 	  cuckoo_rep_cnt    = 0;
#endif
};

struct AccumulatorCluster {
    nn::AccumulatorCache accum_cache;
    AccumulatorCluster*  prev_cluster;
    AccumulatorCluster*  next_cluster;
};

struct NodeInfo {
	void clear();

	enumColor					side2move;
	MoveOrder					move_picker;
	Position::IrreversibleState state;
	Move32b						move;
	Move32b						best_move;
	Score						score;
	Score						eval;
	float 						improving_rate;
	bool						can_move;
	Score						best_score;
	bool						check;
	uint8_t						moves_searched;
	uint8_t						move_index;
	TTEntry::Bound				bound;
    AccumulatorCluster          cluster;
	bool						cuckoo_check;
};

class TreeStack {
public:
	TreeStack();
	~TreeStack();

	TreeStack(TreeStack&&)			   = delete;
	TreeStack(TreeStack&)			   = delete;
	TreeStack operator=(TreeStack&)    = delete;
	TreeStack operator=(TreeStack&& t) = delete;

	void init(MoveOrderHistoryTables* history_buffer);

	NodeInfo* getRootNode();
	NodeInfo* getPreRootNode();
	const NodeInfo* getNode(unsigned ply) const;

	const AccumulatorCluster* getCleanAccumulatorCluster(const AccumulatorCluster* const accum_cluster,
														 const NodeInfo* const preroot);

	void updateDirtyAccumulators(const AccumulatorCluster* const clean_accum_cluster,
								 AccumulatorCluster* const accum_cluster);
private:
	static constexpr size_t _Count = MaxSelDepth;
	NodeInfo* _stack;
};

inline _P_CONSTEXPR int	IidDepth 	      = 4;//3;
inline _P_CONSTEXPR int	IidDepthDiv       = 10;//8;
inline _P_CONSTEXPR int	RfpDepth          = 3;//6;
inline _P_CONSTEXPR int RazorDepth        = 2;//2;
inline _P_CONSTEXPR int	FutilityDepth     = 4;//4;
inline _P_CONSTEXPR int	LmrDepth 	      = 3;//2;
inline _P_CONSTEXPR int NullDiffScale	  = 1100;//1000;
inline _P_CONSTEXPR int	NullReduction     = 29;//16;
inline _P_CONSTEXPR int NullDepth		  = 5;//3;
inline _P_CONSTEXPR int	LmrMoveCount      = 4;//2;
inline _P_CONSTEXPR int RazorMultDelta    = 22;//25;
inline _P_CONSTEXPR int RfpMultDelta      = 139;//150;
inline _P_CONSTEXPR int FutilityDelta     = 10;//32;
inline _P_CONSTEXPR int FutilityMoveCount = 6;//1;
inline _P_CONSTEXPR int RazorBaseDelta    = 100;//150;
inline _P_CONSTEXPR int QMaterialDelta    = 976;//900;
inline _P_CONSTEXPR int QProbeDepth		  = -2;//-1;
inline _P_CONSTEXPR int NNEvalScale		  = 9;//8;
inline _P_CONSTEXPR int ImprovingRate     = 60;//50;
inline _P_CONSTEXPR int RfpImprovingSink  = 3;//2;
inline _P_CONSTEXPR int NullMargin 		  = 11;//20;
inline _P_CONSTEXPR int NullImprovingSink = 2;//4;
inline _P_CONSTEXPR int DynImprovementDepth = 8;//= 6;
inline _P_CONSTEXPR int TTEvalCorrRate	  = 2;//2;

inline constexpr int CheckNodeCount = 2048;

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

	Search() = default;
	Search(TranspositionTable&& tt);
	~Search();

	Search(Search&&)			 = delete;
	Search(Search&)				 = delete;
	Search operator=(Search&)    = delete;
	Search operator=(Search&& t) = delete;

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
	
	template <bool Root>
	Score getDrawScore(const NodeInfo* node);

	int calculateExtension(Position& pos, NodeInfo* node);

	template <enumNode NodeType>
	Score evaluate(const Position& pos,
				   TreeStack& tree_stack,
				   NodeInfo* node,
				   const NodeInfo* preroot, 
				   enumColor side2move, 
				   SearchResults& results);

	Score adjustEvalScore(Score eval, Score tt_score);

	int getNullSearchDepth(Score eval, Score beta, int depth);

	template <bool IsPV>
	bool isRepetitionCycle(const Position& pos, 
						   const FullInfoRecord& game, 
						   const NodeInfo* node, 
						   int ply,
						   SearchResults& results);

	bool canRepetitionDraw(const Position& pos, 
						   const NodeInfo* node, 
						   int ply);

	bool isInsufficientMaterial(const Position& pos);

	TreeStack 		   		_tree_stack;
	CuckooTables			_cuckoo_tables;
	TranspositionTable 		_tt;
	// Each Search instance should have own history buffer with tables 
	// for very MoveOrder in TreeStack.
	// Also, Search class in responsible for allocation and deallocation.
	MoveOrderHistoryTables* _history_buff;
};

INLINE constexpr Search::enumNode operator|(Search::enumNode node0, Search::enumNode node1) {
	return static_cast<Search::enumNode>(static_cast<int>(node0) | static_cast<int>(node1));
}
