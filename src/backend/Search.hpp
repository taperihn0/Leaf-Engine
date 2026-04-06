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

	ull		  null_moves_cnt = 0;
	ull		  null_zungzwang_detected = 0;
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
	PVInfo 	       				pv_line[MaxSelDepth];
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

inline _P_CONSTEXPR int IidDepth = std::lroundf(2.47003f);
inline _P_CONSTEXPR int IidDepthDiv = std::lroundf(10.1113f);
inline _P_CONSTEXPR int RfpDepth = std::lroundf(3.81788f);
inline _P_CONSTEXPR int RazorDepth = std::lroundf(2.16913f);
inline _P_CONSTEXPR int FutilityDepth = std::lroundf(4.5281f);
inline _P_CONSTEXPR int LmrDepth = std::lroundf(3.72648f);
inline _P_CONSTEXPR int NullReduction = std::lroundf(28.477f);
inline _P_CONSTEXPR int LmrMoveCount = std::lroundf(4.73438f);
inline _P_CONSTEXPR int RazorMultDelta = std::lroundf(16.4158f);
inline _P_CONSTEXPR int RfpMultDelta = std::lroundf(111.774f);
inline _P_CONSTEXPR int FutilityMoveCount = std::lroundf(9.12248f);
inline _P_CONSTEXPR int FutilityDelta = std::lroundf(19.411f);
inline _P_CONSTEXPR int RazorBaseDelta = std::lroundf(122.113f);
inline _P_CONSTEXPR int QMaterialDelta = std::lroundf(1086.4f);
inline _P_CONSTEXPR int QProbeDepth = std::lroundf(-2.18116f);
inline _P_CONSTEXPR int NNEvalScale = std::lroundf(11.5293f);
inline _P_CONSTEXPR int ImprovingRate = std::lroundf(45.9603f);
inline _P_CONSTEXPR int RfpImprovingSink = std::lroundf(2.10981f);
inline _P_CONSTEXPR int NullMargin = std::lroundf(6.89969f);
inline _P_CONSTEXPR int NullImprovingSink = std::lroundf(3.67167f);
inline _P_CONSTEXPR int DynImprovementDepth = std::lroundf(9.0707f);
inline _P_CONSTEXPR int TTEvalCorrRate = std::lroundf(2.73521f);
inline _P_CONSTEXPR int NullDiffScale = std::lroundf(1080.3f);
inline _P_CONSTEXPR int NullDepth = std::lroundf(3.48527f);
inline _P_CONSTEXPR int NextDepthTimeRed = std::lroundf(5.31268f);
inline _P_CONSTEXPR int UnstableMatMargin = std::lroundf(46.6373f);
inline _P_CONSTEXPR int UnstableMultMargin = std::lroundf(6.74322f);
inline _P_CONSTEXPR int MinTimeBranchFactor = std::lroundf(1.1361f);
inline _P_CONSTEXPR int MaxTimeBranchFactor = std::lroundf(4.71125f);
inline _P_CONSTEXPR int ContemptDiv = std::lroundf(131.813f);
inline _P_CONSTEXPR int ImprovingExtensionRate = std::lroundf(6.17919f);
inline _P_CONSTEXPR int QuietNotPvNodeReduction = std::lroundf(13.8642f);
inline _P_CONSTEXPR int QuietCutNodeReduction = std::lroundf(2.2681f);
inline _P_CONSTEXPR int QuietCheckReduction = std::lroundf(47.5786f);
inline _P_CONSTEXPR int QuietExtensionReduction = std::lroundf(48.1537f);
inline _P_CONSTEXPR int QuietPawnMoveReduction = std::lroundf(3.64305f);
inline _P_CONSTEXPR int QuietImprovingReductionRate = std::lroundf(6.58544f);
inline _P_CONSTEXPR int QuietHashCapReduction = std::lroundf(15.7638f);
inline _P_CONSTEXPR int QuietKillerMoveReduction = std::lroundf(19.9784f);
inline _P_CONSTEXPR int QuietMoveScoreReductionRate = std::lroundf(12.58f);
inline _P_CONSTEXPR int QuietMoveScoreReductionDiv = std::lroundf(4.62642f);
inline _P_CONSTEXPR int QuietTotalReductionRate = std::lroundf(44.7274f);
inline _P_CONSTEXPR int CaptureNotPvNodeReduction = std::lroundf(13.1658f);
inline _P_CONSTEXPR int CaptureCutNodeReduction = std::lroundf(15.6317f);
inline _P_CONSTEXPR int CaptureCheckReduction = std::lroundf(33.8204f);
inline _P_CONSTEXPR int CaptureHashCapReduction = std::lroundf(28.9586f);
inline _P_CONSTEXPR int CaptureKillerMoveReduction = std::lroundf(30.7286f);
inline _P_CONSTEXPR int CaptureMoveScoreReductionDiv = std::lroundf(63.6271f);
inline _P_CONSTEXPR int CaptureExtensionReduction = std::lroundf(25.1764f);
inline _P_CONSTEXPR int CaptureImprovingReductionRate = std::lroundf(3.34965f);
inline _P_CONSTEXPR int CaptureTotalReductionRate = std::lroundf(40.0938f);
inline _P_CONSTEXPR int HalfMovesEvalLimit = std::lroundf(11.6388f);
inline _P_CONSTEXPR int NullVerifyDepth = std::lroundf(4.3009f);
inline _P_CONSTEXPR int MoveCheckExtensionRate = std::lroundf(13.5076f);
inline _P_CONSTEXPR int MoveCheckExtensionDiv = std::lroundf(15.623f);
inline _P_CONSTEXPR int ImprovingExtensionMateRate = std::lroundf(14.1619f);
inline _P_CONSTEXPR int MateThreadFracExtensionRate = std::lroundf(10.8101f);
inline _P_CONSTEXPR int MateThreadFracExtensionDiv = std::lroundf(17.9312f);
inline _P_CONSTEXPR int MaxMoveExtensionRate = std::lroundf(18.529f);
inline _P_CONSTEXPR int MaxMoveExtensionDiv = std::lroundf(15.2157f);
inline _P_CONSTEXPR int NullVerifyDepthMult = std::lroundf(2.05879f);
inline _P_CONSTEXPR int ExtensionDepth = std::lroundf(11.9872f);
inline _P_CONSTEXPR int SingularDepth = std::lroundf(6.8324f);
inline _P_CONSTEXPR int SingularDepthMargin = std::lroundf(2.93688f);
inline _P_CONSTEXPR int SingularExtensionRate = std::lroundf(13.4039f);
inline _P_CONSTEXPR int SingularBetaDepthMult = std::lroundf(3.60704f);
inline _P_CONSTEXPR int SingularDepthMult = std::lroundf(119.091f);
inline _P_CONSTEXPR int SingularDepthBase = std::lroundf(529.819f);
inline _P_CONSTEXPR int SingularBetaExtensionRate = std::lroundf(3.3303f);
inline constexpr 	int SingularExtensionDiv = 16;
inline constexpr    int SingularBetaExtensionDiv = 32;
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

	template <Search::enumNode QNodeType, bool Root = false>
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
	int getNullVerifyDepth(int nm_depth);

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
