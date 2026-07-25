/*
 * Leaf, a UCI Chess Engine
 * Copyright (C) 2026 taperihn0
 *
 * Leaf is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Leaf is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

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
    int       depth       = 0;
    time_ms_t wtime       = 0,
              btime       = 0;
    time_ms_t winc        = 0, 
              binc        = 0,
              search_time = 0;
    ull       nodes       = 0;
    ull       qnodes      = 0;
    bool      analysis_mode = false;
    Timer     timer;
};

class Search;
struct PVInfo;

struct SearchResults {
    void clear();

    void printBestMove();
    void print(const array1d<PVInfo, MaxSelDepth>& root_pv_line, 
               uint16_t pv_len, 
               const TranspositionTable& tt);
    void printShort();
    void printPV(const array1d<PVInfo, MaxSelDepth>& root_pv_line, 
                 uint16_t pv_len);

#if defined (LEAF_COLLECT_SEARCH_STATS)
    void printSearchStats();
#endif

    unsigned  depth             = 0,
              seldepth          = 0;
    Score     score_cp          = 0;
    ull       nodes_cnt         = 0,
              qnodes_cnt        = 0;
    size_t    tt_entries        = 0;
    Move32b   best_move         = Move32b::Null;
    time_ms_t duration          = 0;
    array1d<ull, MaxDepth + 1> nodes_per_depth = {};
    array1d<time_ms_t, MaxDepth + 1> time_per_depth  = {};

#if defined(LEAF_COLLECT_SEARCH_STATS)
    ull       pv_nodes_cnt      = 0,
              npv_nodes_cnt     = 0,
              cut_nodes_cnt     = 0,
              all_nodes_cnt     = 0;

    ull       tt_probe_cnt      = 0,
              qtt_probe_cnt     = 0,
              tt_cut_cnt        = 0,
              qtt_cut_cnt       = 0,
              qttmove_probe_cnt = 0;

    ull       ttmove_cut_cnt    = 0,
              qttmove_cut_cnt   = 0;

    ull       beta_cut_cnt      = 0;
    ull       qbeta_cut_cnt     = 0;

    ull       nmeval_cnt        = 0;
    ull       qeval_cnt         = 0;

    ull       rep_call_cnt      = 0;
    ull       rep_cnt           = 0;

    ull       cuckoo_rep_cnt    = 0;

    ull       reduced_search_cnt = 0,
              reduced_search_fail_high = 0,
              reduced_search_fail_low = 0;

    ull       null_moves_cnt     = 0;
    ull       null_zungzwang_detected = 0;

    ull       syzygy_tb_probe_cnt = 0;
    ull       syzygy_tb_cuts      = 0;

    array1d<ull, MaxNodeMoves>   move_cut_cnt = {};
    array1d<ull, MaxNodeMoves>   move_reduced_cnt = {};
    array1d<ull, MaxNodeMoves>   move_reduced_fail_high_cnt = {};
    array1d<float, MaxNodeMoves> move_reduction_sum = {};
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

    enumColor                    side2move;
    MoveOrder                    move_picker;
    Position::IrreversibleState  state;
    Move32b                      move;
    Move32b                      best_move;
    Score                        score;
    Score                        eval;
    float                        improving_rate;
    bool                         can_move;
    Score                        best_score;
    bool                         check;
    uint8_t                      moves_searched;
    uint8_t                      move_index;
    TTBound                      bound;
    AccumulatorCluster           cluster;
    array1d<PVInfo, MaxSelDepth> pv_line;
    uint16_t                     pv_line_len;
    bool                         is_cut;
};

class TreeStack {
public:
    TreeStack();

    TreeStack(const TreeStack&)           = delete;
    TreeStack(TreeStack&&)                = delete;
    TreeStack operator=(const TreeStack&) = delete;
    TreeStack operator=(TreeStack&&)      = delete;

    void clear(MoveOrderHistoryTables* history_buffer);

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
    mem::AlignedUniquePtr<NodeInfo> _stack;
};

/*
*   Tunable parameters in Search.
*    _P_CONSTEXPR macro expands to constexpr when _ENABLE_TUNING macro is not defined.
*   On _ENABLE_TUNING defined tuning mode is turned on and _P_CONSTEXPR and _P_STATIC are empty.
*/

inline _P_CONSTEXPR int IidDepth = roundi<float>(2.04679f);
inline _P_CONSTEXPR int IidDepthDiv = roundi<float>(15.3313f);
inline _P_CONSTEXPR int RfpDepth = roundi<float>(5.99441f);
inline _P_CONSTEXPR int RazorDepth = roundi<float>(2.17005f);
inline _P_CONSTEXPR int FutilityDepth = roundi<float>(4.82571f);
inline _P_CONSTEXPR int LmrDepth = roundi<float>(2.02543f);
inline _P_CONSTEXPR int NullReduction = roundi<float>(35.7776f);
inline _P_CONSTEXPR int LmrMoveCount = roundi<float>(4.8846f);
inline _P_CONSTEXPR int RazorMultDelta = roundi<float>(8.51974f);
inline _P_CONSTEXPR int RfpMultDelta = roundi<float>(117.564f);
inline _P_CONSTEXPR int FutilityMoveCount = roundi<float>(9.70808f);
inline _P_CONSTEXPR int FutilityDelta = roundi<float>(17.4389f);
inline _P_CONSTEXPR int RazorBaseDelta = roundi<float>(83.0085f);
inline _P_CONSTEXPR int QMaterialDelta = roundi<float>(1115.86f);
inline _P_CONSTEXPR int QProbeDepth = roundi<float>(0.535417f);
inline _P_CONSTEXPR int NNEvalScale = roundi<float>(14.4784f);
inline _P_CONSTEXPR int ImprovingRate = roundi<float>(38.9676f);
inline _P_CONSTEXPR int RfpImprovingSink = roundi<float>(1.84889f);
inline _P_CONSTEXPR int NullMargin = roundi<float>(3.54293f);
inline _P_CONSTEXPR int NullImprovingSink = roundi<float>(7.90867f);
inline _P_CONSTEXPR int DynImprovementDepth = roundi<float>(11.1463f);
inline _P_CONSTEXPR int TTEvalCorrRate = roundi<float>(2.79444f);
inline _P_CONSTEXPR int NullDiffScale = roundi<float>(1070.f);
inline _P_CONSTEXPR int NullDepth = roundi<float>(2.81013f);
inline _P_CONSTEXPR int NextDepthTimeRed = roundi<float>(6.84012f);
inline _P_CONSTEXPR int UnstableMatMargin = roundi<float>(19.9507f);
inline _P_CONSTEXPR int UnstableMultMargin = roundi<float>(4.64543f);
inline _P_CONSTEXPR int ContemptDiv = roundi<float>(75.6344f);
inline _P_CONSTEXPR int ImprovingExtensionRate = roundi<float>(8.21316f);
inline _P_CONSTEXPR int QuietNotPvNodeReduction = roundi<float>(19.6915f);
inline _P_CONSTEXPR int QuietCutNodeReduction = roundi<float>(3.04484f);
inline _P_CONSTEXPR int QuietCheckReduction = roundi<float>(63.1862f);
inline _P_CONSTEXPR int QuietExtensionReduction = roundi<float>(30.9815f);
inline _P_CONSTEXPR int QuietPawnMoveReduction = roundi<float>(3.09287f);
inline _P_CONSTEXPR int QuietImprovingReductionRate = roundi<float>(4.38258f);
inline _P_CONSTEXPR int QuietHashCapReduction = roundi<float>(16.3663f);
inline _P_CONSTEXPR int QuietKillerMoveReduction = roundi<float>(29.0805f);
inline _P_CONSTEXPR int QuietTotalReductionRate = roundi<float>(26.3769f);
inline _P_CONSTEXPR int CaptureNotPvNodeReduction = roundi<float>(19.7447f);
inline _P_CONSTEXPR int CaptureCutNodeReduction = roundi<float>(12.3891f);
inline _P_CONSTEXPR int CaptureCheckReduction = roundi<float>(34.7674f);
inline _P_CONSTEXPR int CaptureHashCapReduction = roundi<float>(43.7795f);
inline _P_CONSTEXPR int CaptureKillerMoveReduction = roundi<float>(38.8519f);
inline _P_CONSTEXPR int CaptureExtensionReduction = roundi<float>(49.8009f);
inline _P_CONSTEXPR int CaptureImprovingReductionRate = roundi<float>(2.47342f);
inline _P_CONSTEXPR int CaptureTotalReductionRate = roundi<float>(45.2667f);
inline _P_CONSTEXPR int HalfMovesEvalLimit = roundi<float>(17.1531f);
inline _P_CONSTEXPR int NullVerifyDepth = roundi<float>(8.70259f);
inline _P_CONSTEXPR int MoveCheckExtensionRate = roundi<float>(11.1183f);
inline _P_CONSTEXPR int MoveCheckExtensionDiv = roundi<float>(11.7016f);
inline _P_CONSTEXPR int ImprovingExtensionMateRate = roundi<float>(10.3031f);
inline _P_CONSTEXPR int MateThreadFracExtensionRate = roundi<float>(11.4543f);
inline _P_CONSTEXPR int MateThreadFracExtensionDiv = roundi<float>(16.1453f);
inline _P_CONSTEXPR int MaxMoveExtensionRate = roundi<float>(16.5613f);
inline _P_CONSTEXPR int MaxMoveExtensionDiv = roundi<float>(11.9389f);
inline _P_CONSTEXPR int NullVerifyDepthMult = roundi<float>(4.17217f);
inline _P_CONSTEXPR int ExtensionDepth = roundi<float>(21.2842f);
inline _P_CONSTEXPR int SingularDepth = roundi<float>(4.71072f);
inline _P_CONSTEXPR int SingularDepthMargin = roundi<float>(1.61158f);
inline _P_CONSTEXPR int SingularExtensionRate = roundi<float>(3.90049f);
inline _P_CONSTEXPR int SingularBetaDepthMult = roundi<float>(3.13396f);
inline _P_CONSTEXPR int SingularDepthMult = roundi<float>(119.323f);
inline _P_CONSTEXPR int SingularDepthBase = roundi<float>(525.397f);
inline _P_CONSTEXPR int SingularBetaExtensionRate = roundi<float>(8.4984f);
inline _P_CONSTEXPR int TablebaseProbeDepth = roundi<float>(7.2936f);
inline _P_CONSTEXPR int TablebasePieceCountLimit = roundi<float>(6.36403f);
inline _P_CONSTEXPR int TablebaseWinScore = roundi<float>(31039.1f);
inline _P_CONSTEXPR int TablebasePieceDiffMult = roundi<float>(90.277f);
inline _P_CONSTEXPR int TablebaseScoreScale = roundi<float>(18.2255f);
inline _P_CONSTEXPR int AspirationSearchDepth = roundi<float>(4.30574f);
inline _P_CONSTEXPR int AspirationFirstWindow = roundi<float>(57.1045f);
inline _P_CONSTEXPR int AspirationUnstableFactor = roundi<float>(184.446f);
inline _P_CONSTEXPR int AspirationDepthRate = roundi<float>(1.04618f);
inline _P_CONSTEXPR int AspirationMaxDepthInfl = roundi<float>(10.7805f);
inline _P_CONSTEXPR int AspirationWindowScoreDiv = roundi<float>(5204.01f);
inline _P_CONSTEXPR int AspirationMaxWindow = roundi<float>(750.307f);
inline _P_CONSTEXPR int AspirationCount = roundi<float>(3.07948f);
inline _P_CONSTEXPR int AspirationWidenRate = roundi<float>(4.70541f);
inline _P_CONSTEXPR int SeeValuePrune = roundi<float>(-25.5263f);

/* Static parameters -
*  These are not tuned.
*/

inline constexpr int    SingularExtensionDiv = 16;
inline constexpr int    SingularBetaExtensionDiv = 32;
inline constexpr int    CheckNodeCount = 2048;
inline constexpr bool   UseSyzygyTablebase = _USE_SYZYGY_TB;
inline constexpr bool   UseSyzygyTablebaseRoot = _USE_SYZYGY_TB_ROOT;
inline constexpr double MinTimeBranchFactor = 1.;
inline constexpr double MaxTimeBranchFactor = 5.;

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

    Search(Search&&)             = delete;
    Search(Search&)              = delete;
    Search operator=(Search&)    = delete;
    Search operator=(Search&& t) = delete;

    template <enumInfoLevel InfoLevel = SEARCH_FULL_INFO>
    _NODISCARD Move32b findBestMove(Position& pos, 
                                    const FullInfoRecord& game, 
                                    SearchLimits limits);

    template <enumInfoLevel InfoLevel = SEARCH_FULL_INFO>
    _NODISCARD Move32b findBestMove(Position& pos, 
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
    Move32b goIterativeDeepening(Position& pos, 
                                 const FullInfoRecord& game, 
                                 SearchLimits& limits,
                                 SearchResults& search_results,
                                 enumInfoLevel info_lv);

    bool goSearch(Position& pos, 
                  const FullInfoRecord& game, 
                  SearchLimits& limits, SearchResults& results,
                  Score alpha, Score beta);

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
    bool isTablebaseScore(Score score) const;
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
                       const array1d<PVInfo, MaxSelDepth>& root_pv_line, 
                       uint16_t pv_len,
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

    TreeStack          _tree_stack;
    CuckooTables       _cuckoo_tables;
    TranspositionTable _tt;

    /* Each Search instance should have own history buffer with tables 
    *  for very MoveOrder in TreeStack.
    *  Also, Search class in responsible for allocation and deallocation.
    */
    mem::AlignedUniquePtr<MoveOrderHistoryTables> 
                 _history_buff;
    Score::int_t _contempt = Score::Undef;
};

_INLINE constexpr Search::enumNode operator|(Search::enumNode node0, Search::enumNode node1) {
    return static_cast<Search::enumNode>(static_cast<int>(node0) | static_cast<int>(node1));
}
