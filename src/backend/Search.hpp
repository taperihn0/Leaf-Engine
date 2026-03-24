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
	bool isTimeLimit();

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
struct PVInfo;

struct SearchResults {
	void printBestMove();
	void print(const PVInfo* root_pv_line, uint16_t pv_len, const TranspositionTable& tt);
	void printShort();
	void printPV(const PVInfo* root_pv_line, uint16_t pv_len);

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
	ull 	  nodes_per_depth[MaxDepth + 1] = {};
	time_ms_t time_per_depth[MaxDepth + 1]  = {};

#if defined (_COLLECT_SEARCH_STATS)
	ull       pv_nodes_cnt		= 0,
			  npv_nodes_cnt		= 0,
			  cut_nodes_cnt		= 0,
			  all_nodes_cnt		= 0;

	ull		  tt_probe_cnt		= 0,
			  qtt_probe_cnt		= 0,
			  tt_cut_cnt		= 0,
			  qtt_cut_cnt		= 0,
			  qttmove_probe_cnt = 0;

	ull		  ttmove_cut_cnt	= 0,
			  qttmove_cut_cnt	= 0;

	ull		  beta_cut_cnt		= 0;
	ull		  qbeta_cut_cnt		= 0;

	ull		  move_cut_cnt[MaxNodeMoves] = {};

	ull 	  nmeval_cnt 		= 0;
	ull 	  qeval_cnt 		= 0;

	ull 	  rep_call_cnt		= 0;
	ull 	  rep_cnt			= 0;

	ull 	  cuckoo_rep_cnt    = 0;

	ull		  reduced_search_cnt = 0,
			  reduced_search_fail_high = 0,
			  reduced_search_fail_low = 0;

	ull		  move_reduced_cnt[MaxNodeMoves] = {};
	ull 	  move_reduced_fail_high_cnt[MaxNodeMoves] = {};
	float 	  move_reduction_sum[MaxNodeMoves] = {};
#endif
};

struct AccumulatorCluster {
    nn::AccumulatorCache accum_cache;
    AccumulatorCluster*  prev_cluster;
    AccumulatorCluster*  next_cluster;
};

struct PVInfo {	
	Move16b best_move;
	Score   score;
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
	PVInfo 	       				pv_line[MaxDepth];
	uint16_t 					pv_line_len;
	bool						is_cut;
};

class TreeStack {
public:
	TreeStack();
	~TreeStack();

	TreeStack(TreeStack&&)			 = delete;
	TreeStack(TreeStack&)			 = delete;
	TreeStack operator=(TreeStack&)  = delete;
	TreeStack operator=(TreeStack&&) = delete;

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

/*
*   Tunable parameters in internal search.
*	_P_CONSTEXPR macro expands to constexpr in release builds, but it 
*   is ignored in debug builds to allow changing parameters when tuning.
*/

inline _P_CONSTEXPR int	IidDepth = 3;
inline _P_CONSTEXPR int	IidDepthDiv = 8;
inline _P_CONSTEXPR int	RfpDepth = 4;
inline _P_CONSTEXPR int RazorDepth = 2;
inline _P_CONSTEXPR int	FutilityDepth = 4;
inline _P_CONSTEXPR int	LmrDepth = 3;
inline _P_CONSTEXPR int NullDiffScale = 1081;
inline _P_CONSTEXPR int	NullReduction = 30;
inline _P_CONSTEXPR int NullDepth = 3;
inline _P_CONSTEXPR int	LmrMoveCount = 6;
inline _P_CONSTEXPR int RazorMultDelta = 20;
inline _P_CONSTEXPR int RfpMultDelta = 129;
inline _P_CONSTEXPR int FutilityDelta = 19;
inline _P_CONSTEXPR int FutilityMoveCount = 9;
inline _P_CONSTEXPR int RazorBaseDelta = 108;
inline _P_CONSTEXPR int QMaterialDelta = 1074;
inline _P_CONSTEXPR int QProbeDepth = -2;
inline _P_CONSTEXPR int NNEvalScale = 12;
inline _P_CONSTEXPR int ImprovingRate = 55;
inline _P_CONSTEXPR int RfpImprovingSink = 2;
inline _P_CONSTEXPR int NullMargin = 7;
inline _P_CONSTEXPR int NullImprovingSink = 4;
inline _P_CONSTEXPR int DynImprovementDepth = 10;
inline _P_CONSTEXPR int TTEvalCorrRate = 3;
inline _P_CONSTEXPR int NextDepthTimeRed = 6;
inline _P_CONSTEXPR int UnstableMatMargin = 46;
inline _P_CONSTEXPR int UnstableMultMargin = 8;
inline _P_CONSTEXPR int MinTimeBranchFactor = 1;
inline _P_CONSTEXPR int MaxTimeBranchFactor = 5;
inline _P_CONSTEXPR int ContemptDiv = 126;
inline _P_CONSTEXPR int ImprovingExtensionRate = 8;
inline _P_CONSTEXPR int QuietNotPvNodeReduction = 4;
inline _P_CONSTEXPR int QuietCutNodeReduction = 5;
inline _P_CONSTEXPR int QuietCheckReduction = 59;
inline _P_CONSTEXPR int QuietExtensionReduction = 45;
inline _P_CONSTEXPR int QuietPawnMoveReduction = 2;
inline _P_CONSTEXPR int QuietImprovingReductionRate = 6;
inline _P_CONSTEXPR int QuietHashCapReduction = 17;
inline _P_CONSTEXPR int QuietKillerMoveReduction = 12;
inline _P_CONSTEXPR int QuietTotalReductionRate = 36;
inline _P_CONSTEXPR int CaptureNotPvNodeReduction = 6;
inline _P_CONSTEXPR int CaptureCutNodeReduction = 19;
inline _P_CONSTEXPR int CaptureCheckReduction = 32;
inline _P_CONSTEXPR int CaptureHashCapReduction = 26;
inline _P_CONSTEXPR int CaptureKillerMoveReduction = 30;
inline _P_CONSTEXPR int CaptureExtensionReduction = 31;
inline _P_CONSTEXPR int CaptureImprovingReductionRate = 5;
inline _P_CONSTEXPR int CaptureTotalReductionRate = 47;
inline constexpr    int CheckNodeCount = 2048;

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
	Move32b findBestMove(Position& pos, 
						 const FullInfoRecord& game, 
						 SearchLimits limits);

	static Move32b _findBestMove_unittest(Search& search, 
									      Position& pos, 
									      const FullInfoRecord& game, 
									      SearchLimits limits);
	
	void clearHashTT();
	void resizeHashTT(size_t tt_size_mb);
	void registerNewGame();
private:
	template <enumInfoLevel InfoLevel>
	Move32b goIterativeDeepening(Position& pos, 
								 const FullInfoRecord& game, 
								 SearchLimits& limits);

	template <enumInfoLevel InfoLevel>
	bool goSearch(Position& pos, 
				  const FullInfoRecord& game, 
				  SearchLimits& limits, SearchResults& results);

	template <enumNode NmNodeType, bool NullMove, bool Root = false>
	Score nmSearch(Position& pos, 
				   SearchLimits& limits, SearchResults& results, 
				   const FullInfoRecord& game, 
				   NodeInfo* node,
				   Score alpha, Score beta, 
				   int depth, int ply);

	template <Search::enumNode QNodeType>
	Score qSearch(Position& pos, 
				  SearchLimits& limits, SearchResults& results, 
				  NodeInfo* node, 
				  Score alpha, Score beta, 
				  int depth, int ply);
	
	Score getDrawScore(const NodeInfo* node);
	Score applyContempt(Score score, const NodeInfo* node);

	template <enumNode NodeType>
	Score evaluate(const Position& pos,
				   TreeStack& tree_stack,
				   NodeInfo* node,
				   const NodeInfo* preroot, 
				   enumColor side2move, 
				   SearchResults& results);

	Score adjustEvalScore(Score eval, Score score);

	int getNullSearchDepth(Score eval, Score beta, int depth);

	void refreshPVinTT(const Position& pos, 
					   const PVInfo* root_pv_line, uint16_t pv_len,
					   SearchResults& results);

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

	Score::int_t 			_contempt = Score::Undef;
};

_INLINE constexpr Search::enumNode operator|(Search::enumNode node0, Search::enumNode node1) {
	return static_cast<Search::enumNode>(static_cast<int>(node0) | static_cast<int>(node1));
}
