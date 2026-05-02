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
#include "Tablebase.hpp"

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
	void clear();

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

	ull		  null_moves_cnt 	= 0;
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
	const NodeInfo* getRootNode() const;
	NodeInfo* getPreRootNode();
	const NodeInfo* getPreRootNode() const;
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

inline _P_CONSTEXPR int IidDepth = roundi<float>(2.71396f);
inline _P_CONSTEXPR int IidDepthDiv = roundi<float>(9.55298f);
inline _P_CONSTEXPR int RfpDepth = roundi<float>(3.8337f);
inline _P_CONSTEXPR int RazorDepth = roundi<float>(2.19616f);
inline _P_CONSTEXPR int FutilityDepth = roundi<float>(4.42344f);
inline _P_CONSTEXPR int LmrDepth = roundi<float>(3.66373f);
inline _P_CONSTEXPR int NullReduction = roundi<float>(27.9257f);
inline _P_CONSTEXPR int LmrMoveCount = roundi<float>(5.2673f);
inline _P_CONSTEXPR int RazorMultDelta = roundi<float>(16.0559f);
inline _P_CONSTEXPR int RfpMultDelta = roundi<float>(110.34f);
inline _P_CONSTEXPR int FutilityMoveCount = roundi<float>(9.40972f);
inline _P_CONSTEXPR int FutilityDelta = roundi<float>(19.9469f);
inline _P_CONSTEXPR int RazorBaseDelta = roundi<float>(110.336f);
inline _P_CONSTEXPR int QMaterialDelta = roundi<float>(1096.21f);
inline _P_CONSTEXPR int QProbeDepth = -roundi<float>(1.91036f);
inline _P_CONSTEXPR int NNEvalScale = roundi<float>(11.2013f);
inline _P_CONSTEXPR int ImprovingRate = roundi<float>(45.6534f);
inline _P_CONSTEXPR int RfpImprovingSink = roundi<float>(1.88982f);
inline _P_CONSTEXPR int NullMargin = roundi<float>(7.19693f);
inline _P_CONSTEXPR int NullImprovingSink = roundi<float>(3.55683f);
inline _P_CONSTEXPR int DynImprovementDepth = roundi<float>(7.8314f);
inline _P_CONSTEXPR int TTEvalCorrRate = roundi<float>(2.71009f);
inline _P_CONSTEXPR int NullDiffScale = roundi<float>(1083.65f);
inline _P_CONSTEXPR int NullDepth = roundi<float>(3.10287f);
inline _P_CONSTEXPR int NextDepthTimeRed = roundi<float>(5.50724f);
inline _P_CONSTEXPR int UnstableMatMargin = roundi<float>(38.662f);
inline _P_CONSTEXPR int UnstableMultMargin = roundi<float>(6.85276f);
inline _P_CONSTEXPR int MinTimeBranchFactor = roundi<float>(1.11326f);
inline _P_CONSTEXPR int MaxTimeBranchFactor = roundi<float>(4.68838f);
inline _P_CONSTEXPR int ContemptDiv = roundi<float>(126.079f);
inline _P_CONSTEXPR int ImprovingExtensionRate = roundi<float>(5.12942f);
inline _P_CONSTEXPR int QuietNotPvNodeReduction = roundi<float>(14.0541f);
inline _P_CONSTEXPR int QuietCutNodeReduction = roundi<float>(2.06097f);
inline _P_CONSTEXPR int QuietCheckReduction = roundi<float>(41.5495f);
inline _P_CONSTEXPR int QuietExtensionReduction = roundi<float>(48.0492f);
inline _P_CONSTEXPR int QuietPawnMoveReduction = roundi<float>(4.51564f);
inline _P_CONSTEXPR int QuietImprovingReductionRate = roundi<float>(6.38783f);
inline _P_CONSTEXPR int QuietHashCapReduction = roundi<float>(15.841f);
inline _P_CONSTEXPR int QuietKillerMoveReduction = roundi<float>(17.6522f);
inline _P_CONSTEXPR int QuietTotalReductionRate = roundi<float>(39.9405f);
inline _P_CONSTEXPR int CaptureNotPvNodeReduction = roundi<float>(13.2607f);
inline _P_CONSTEXPR int CaptureCutNodeReduction = roundi<float>(17.7044f);
inline _P_CONSTEXPR int CaptureCheckReduction = roundi<float>(35.484f);
inline _P_CONSTEXPR int CaptureHashCapReduction = roundi<float>(31.3566f);
inline _P_CONSTEXPR int CaptureKillerMoveReduction = roundi<float>(30.1209f);
inline _P_CONSTEXPR int CaptureExtensionReduction = roundi<float>(23.9629f);
inline _P_CONSTEXPR int CaptureImprovingReductionRate = roundi<float>(3.02985f);
inline _P_CONSTEXPR int CaptureTotalReductionRate = roundi<float>(40.1615f);
inline _P_CONSTEXPR int HalfMovesEvalLimit = roundi<float>(11.9435f);
inline _P_CONSTEXPR int NullVerifyDepth = roundi<float>(3.53335f);
inline _P_CONSTEXPR int MoveCheckExtensionRate = roundi<float>(12.9377f);
inline _P_CONSTEXPR int MoveCheckExtensionDiv = roundi<float>(14.1679f);
inline _P_CONSTEXPR int ImprovingExtensionMateRate = roundi<float>(13.4723f);
inline _P_CONSTEXPR int MateThreadFracExtensionRate = roundi<float>(10.f);
inline _P_CONSTEXPR int MateThreadFracExtensionDiv = roundi<float>(17.9866f);
inline _P_CONSTEXPR int MaxMoveExtensionRate = roundi<float>(20.1009f);
inline _P_CONSTEXPR int MaxMoveExtensionDiv = roundi<float>(14.6002f);
inline _P_CONSTEXPR int NullVerifyDepthMult = roundi<float>(2.24884f);
inline _P_CONSTEXPR int ExtensionDepth = roundi<float>(10.7021f);
inline _P_CONSTEXPR int SingularDepth = roundi<float>(6.56764f);
inline _P_CONSTEXPR int SingularDepthMargin = roundi<float>(3.01553f);
inline _P_CONSTEXPR int SingularExtensionRate = roundi<float>(10.429f);
inline _P_CONSTEXPR int SingularBetaDepthMult = roundi<float>(3.47528f);
inline _P_CONSTEXPR int SingularDepthMult = roundi<float>(119.723f);
inline _P_CONSTEXPR int SingularDepthBase = roundi<float>(523.061f);
inline _P_CONSTEXPR int SingularBetaExtensionRate = roundi<float>(1.76967f);
inline _P_CONSTEXPR int TablebaseProbeDepth = 10; // !
inline _P_CONSTEXPR int TablebasePieceCountLimit = 8; // !
inline _P_CONSTEXPR int TablebaseWinScore = static_cast<int>(31000); // !
inline _P_CONSTEXPR int TablebasePieceDiffMult = 10;

/* Static parameters */
inline _P_CONSTEXPR int  TablebaseLossScore = -TablebaseWinScore;
inline constexpr 	int  SingularExtensionDiv = 16;
inline constexpr    int  SingularBetaExtensionDiv = 32;
inline constexpr    int  CheckNodeCount = 2048;
inline constexpr 	bool UseSyzygyTablebase = _USE_SYZYGY_TB;
inline constexpr 	bool UseSyzygyTablebaseRoot = _USE_SYZYGY_TB_ROOT;

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

	template <enumInfoLevel InfoLevel = SEARCH_FULL_INFO>
	Move32b findBestMove(Position& pos, 
						 const FullInfoRecord& game, 
						 SearchLimits limits,
						 SearchResults& search_results);

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
								 SearchLimits& limits,
								 SearchResults& search_results);

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
	
	Score getDrawScore(const NodeInfo* node) const;
	Score getTablebaseScore(SyzygyTablebase::TbWdlInfo wdl, 
							const Position& pos, 
							const NodeInfo* node, 
							int ply) const;
	Score applyContempt(Score score, const NodeInfo* node) const;

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
