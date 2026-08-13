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
#include "Tuning.hpp"

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
    Position::ReversibleState    state;
    Move32b                      move;
    Move32b                      best_move;
    Score                        score;
    Score                        eval;
    int32_t                      improving;
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
    bool                         mate_thread;
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
*/

_DEFINE_TUNABLE_PARAMETER(IidDepth, int32_t, 2.6917f, 2.f, 5.f, 0.9f);
_DEFINE_TUNABLE_PARAMETER(IidDepthDiv, int32_t, 12.0625f, 8.f, 16.f, 0.9f);
_DEFINE_TUNABLE_PARAMETER(RfpDepth, int32_t, 5.21208f, 2.f, 6.f, 0.9f);
_DEFINE_TUNABLE_PARAMETER(RazorDepth, int32_t, 1.55689f, 1.f, 4.f, 0.9f);
_DEFINE_TUNABLE_PARAMETER(FutilityDepth, int32_t, 2.80804f, 2.f, 5.f, 0.9f);
_DEFINE_TUNABLE_PARAMETER(LmrDepth, int32_t, 1.87427f, 1.f, 4.f, 1.5f);
_DEFINE_TUNABLE_PARAMETER(LmrBaseQuietReduction, int32_t, 162.9090f, 130.f, 190.f, 0.4f);
_DEFINE_TUNABLE_PARAMETER(LmrLogQuietDepthMovesMult, int32_t, 69.8181f, 50.f, 90.f, 0.4f);
_DEFINE_TUNABLE_PARAMETER(LmrBaseCaptureReduction, int32_t, 132.7407f, 110.f, 150.f, 0.4f);
_DEFINE_TUNABLE_PARAMETER(LmrLogCaptureDepthMovesMult, int32_t, 56.8888f, 40.f, 76.f, 0.4f);
_DEFINE_TUNABLE_PARAMETER(NullReduction, int32_t, 35.4356f, 15.f, 51.f, 1.1f);
_DEFINE_TUNABLE_PARAMETER(LmrMoveCount, int32_t, 4.68328f, 1.f, 16.f, 0.6f);
_DEFINE_TUNABLE_PARAMETER(RazorMultDelta, int32_t, 10.2223f, 5.f, 40.f, 0.4f);
_DEFINE_TUNABLE_PARAMETER(RazorCutDelta, int32_t, 10.f, 5.f, 35.f, 1.f);
_DEFINE_TUNABLE_PARAMETER(RazorBetaLimit, int32_t, 1200.f, 1000.f, 2000.f, 0.5f);
_DEFINE_TUNABLE_PARAMETER(RfpMultDelta, int32_t, 113.653f, 10.f, 220.f, 0.4f);
_DEFINE_TUNABLE_PARAMETER(RfpMarginThreshold, int32_t, 6.f, 1.f, 30.f, 1.2f);
_DEFINE_TUNABLE_PARAMETER(RfpReturnValueWeight, int32_t, 64.f, 4.f, 128.f, 0.5f);
_DEFINE_TUNABLE_PARAMETER(FutilityMoveCount, int32_t, 7.8672f, 1.f, 16.f, 0.5f);
_DEFINE_TUNABLE_PARAMETER(FutilityDelta, int32_t, 22.8808f, 2.f, 50.f, 0.7f);
_DEFINE_TUNABLE_PARAMETER(FutilityScoreMult, int32_t, 9.f, 5.f, 13.f, 0.9f);
_DEFINE_TUNABLE_PARAMETER(RazorBaseDelta, int32_t, 119.431f, 20.f, 500.f, 0.4f);
_DEFINE_TUNABLE_PARAMETER(QMaterialDelta, int32_t, 1106.39f, 700.f, 1200.f, 0.6f);
_DEFINE_TUNABLE_PARAMETER(NNEvalScale, int32_t, 8.7321f, 7.f, 17.f, 1.3f);
_DEFINE_TUNABLE_PARAMETER(ImprovingRate, int32_t, 39.2507f, 30.f, 90.f, 0.6f);
_DEFINE_TUNABLE_PARAMETER(RfpImprovingSinkMult, int32_t, 128.0f, 100.f, 160.f, 0.6f);
_DEFINE_TUNABLE_PARAMETER(NullMargin, int32_t, 4.33824f, 1.f, 14.f, 0.5f);
_DEFINE_TUNABLE_PARAMETER(NullImprovingSinkMult, int32_t, 28.4444f, 16.f, 38.f, 1.f);
_DEFINE_TUNABLE_PARAMETER(TTEvalCorrRate, int32_t, 2.36075f, 1.f, 4.f, 0.8f);
_DEFINE_TUNABLE_PARAMETER(NullDiffScale, int32_t, 1053.47f, 800.f, 1300.f, 0.5f);
_DEFINE_TUNABLE_PARAMETER(NullDepth, int32_t, 3.01463f, 2.f, 5.f, 1.3f);
_DEFINE_TUNABLE_PARAMETER(NextDepthTimeRed, int32_t, 6.82642f, 4.f, 8.f, 1.2f);
_DEFINE_TUNABLE_PARAMETER(UnstableMatMargin, int32_t, 13.8272f, 10.f, 80.f, 1.f);
_DEFINE_TUNABLE_PARAMETER(UnstableMultMargin, int32_t, 6.80492f, 4.f, 12.f, 2.5f);
_DEFINE_TUNABLE_PARAMETER(ContemptDiv, int32_t, 86.5476f, 30.f, 160.f, 1.7f);
_DEFINE_TUNABLE_PARAMETER(ImprovingExtensionRate, int32_t, 36.57036f, 10.f, 70.f, 0.8f);
_DEFINE_TUNABLE_PARAMETER(QuietNotPvNodeReduction, int32_t, 115.2750f, 40.f, 200.f, 1.5f);
_DEFINE_TUNABLE_PARAMETER(QuietCutNodeReduction, int32_t, 14.0604f, 0.f, 40.f, 0.5f);
_DEFINE_TUNABLE_PARAMETER(QuietCheckReduction, int32_t, 345.1729f, 100.f, 600.f, 0.9f);
_DEFINE_TUNABLE_PARAMETER(QuietExtensionReduction, int32_t, 113.0990f, 40.f, 200.f, 1.5f);
_DEFINE_TUNABLE_PARAMETER(QuietPawnMoveReduction, int32_t, 35.3802f, 10.f, 70.f, 0.8f);
_DEFINE_TUNABLE_PARAMETER(QuietImprovingReductionRate, int32_t, 21.4482f, 5.f, 50.f, 0.5f);
_DEFINE_TUNABLE_PARAMETER(QuietHashCapReduction, int32_t, 130.2399f, 40.f, 220.f, 1.5f);
_DEFINE_TUNABLE_PARAMETER(QuietKillerMoveReduction, int32_t, 212.9658f, 80.f, 380.f, 1.f);
_DEFINE_TUNABLE_PARAMETER(CaptureNotPvNodeReduction, int32_t, 90.0759f, 30.f, 160.f, 1.2f);
_DEFINE_TUNABLE_PARAMETER(CaptureCutNodeReduction, int32_t, 58.6884f, 20.f, 110.f, 0.9f);
_DEFINE_TUNABLE_PARAMETER(CaptureCheckReduction, int32_t, 167.3002f, 50.f, 300.f, 2.0f);
_DEFINE_TUNABLE_PARAMETER(CaptureHashCapReduction, int32_t, 152.2967f, 50.f, 260.f, 1.8f);
_DEFINE_TUNABLE_PARAMETER(CaptureKillerMoveReduction, int32_t, 159.2310f, 50.f, 280.f, 1.8f);
_DEFINE_TUNABLE_PARAMETER(CaptureExtensionReduction, int32_t, 75.1489f, 20.f, 140.f, 1.0f);
_DEFINE_TUNABLE_PARAMETER(CaptureImprovingReductionRate, int32_t, 26.63286f, 8.f, 55.f, 0.5f);
_DEFINE_TUNABLE_PARAMETER(HalfMovesEvalLimit, int32_t, 11.8268f, 5.f, 40.f, 1.f);
_DEFINE_TUNABLE_PARAMETER(NullVerifyDepth, int32_t, 10.12013f, 5.f, 16.f, 1.3f);
_DEFINE_TUNABLE_PARAMETER(MoveCheckExtensionRate, int32_t, 12.2576f, 8.f, 16.f, 1.8f);
_DEFINE_TUNABLE_PARAMETER(MoveCheckExtensionDiv, int32_t, 9.95162f, 8.f, 16.f, 1.4f);
_DEFINE_TUNABLE_PARAMETER(ImprovingExtensionMateRate, int32_t, 19.6923f, 12.f, 28.f, 1.f);
_DEFINE_TUNABLE_PARAMETER(MateThreadFracExtensionRate, int32_t, 11.6797f, 10.f, 30.f, 0.9f);
_DEFINE_TUNABLE_PARAMETER(MateThreadFracExtensionDiv, int32_t, 17.9521f, 12.f, 26.f, 1.f);
_DEFINE_TUNABLE_PARAMETER(MaxMoveExtensionRate, int32_t, 16.3261f, 10.f, 30.f, 1.7f);
_DEFINE_TUNABLE_PARAMETER(MaxMoveExtensionDiv, int32_t, 10.9455f, 10.f, 18.f, 1.f);
_DEFINE_TUNABLE_PARAMETER(NullVerifyDepthMult, int32_t, 2.94352f, 1.f, 12.f, 1.1f);
_DEFINE_TUNABLE_PARAMETER(ExtensionDepth, int32_t, 16.8311f, 4.f, 32.f, 1.1f);
_DEFINE_TUNABLE_PARAMETER(SingularDepth, int32_t, 4.03396f, 2.f, 8.f, 1.9f);
_DEFINE_TUNABLE_PARAMETER(SingularDepthMargin, int32_t, 1.71455f, 1.f, 4.f, 1.2f);
_DEFINE_TUNABLE_PARAMETER(SingularExtensionRate, int32_t, 3.08547f, 0.8f, 12.f, 1.8f);
_DEFINE_TUNABLE_PARAMETER(SingularBetaDepthMult, int32_t, 47.f, 1.f, 96.f, 1.2f);
_DEFINE_TUNABLE_PARAMETER(SingularDepthMult, int32_t, 124.551f, 90.f, 180.f, 1.3f);
_DEFINE_TUNABLE_PARAMETER(SingularDepthBase, int32_t, 535.727f, 400.f, 650.f, 0.6f);
_DEFINE_TUNABLE_PARAMETER(SingularBetaExtensionRate, int32_t, 8.8357f, 1.f, 15.f, 1.2f);
_DEFINE_TUNABLE_PARAMETER(TablebaseProbeDepth, int32_t, 7.33313f, 2.f, 16.f, 2.f);
_DEFINE_TUNABLE_PARAMETER(TablebasePieceCountLimit, int32_t, 5.63201f, 2.f, 10.f, 1.7f);
_DEFINE_TUNABLE_PARAMETER(TablebasePieceDiffMult, int32_t, 105.374f, 10.f, 250.f, 0.4f);
_DEFINE_TUNABLE_PARAMETER(TablebaseScoreScale, int32_t, 17.0653f, 8.f, 20.f, 1.5f);
_DEFINE_TUNABLE_PARAMETER(AspirationSearchDepth, int32_t, 4.42753f, 2.f, 5.f, 1.5f);
_DEFINE_TUNABLE_PARAMETER(AspirationFirstWindow, int32_t, 68.2525f, 10.f, 120.f, 0.9f);
_DEFINE_TUNABLE_PARAMETER(AspirationUnstableFactor, int32_t, 179.176f, 10.f, 220.f, 0.8f);
_DEFINE_TUNABLE_PARAMETER(AspirationDepthRate, int32_t, 0.906031f, 0.f, 1.5f, 0.3f);
_DEFINE_TUNABLE_PARAMETER(AspirationMaxDepthInfl, int32_t, 8.44154f, 1.f, 14.f, 1.1f);
_DEFINE_TUNABLE_PARAMETER(AspirationWindowScoreDiv, int32_t, 5208.23f, 2000.f, 6500.f, 0.2f);
_DEFINE_TUNABLE_PARAMETER(AspirationMaxWindow, int32_t, 736.662f, 400.f, 1000.f, 0.6f);
_DEFINE_TUNABLE_PARAMETER(AspirationCount, int32_t, 3.4091f, 2.f, 5.f, 1.f);
_DEFINE_TUNABLE_PARAMETER(AspirationWidenRate, int32_t, 4.68001f, 2.f, 5.f, 1.f);
_DEFINE_TUNABLE_PARAMETER(QSeePruningThreshold, int32_t, -8.86404f, -150.f, 80.f, 1.2f);
_DEFINE_TUNABLE_PARAMETER(RfpQuietPenaltyMult, int32_t, 19.f, 10.f, 40.f, 1.1f);
_DEFINE_TUNABLE_PARAMETER(SeePruneDepth, int32_t, 2.f, 0.f, 5.f, 0.9f);
_DEFINE_TUNABLE_PARAMETER(SeePruneMarginMult, int32_t, 8.f, 0.f, 20.f, 0.8f);
_DEFINE_TUNABLE_PARAMETER(SeeCapturePruneThreshold, int32_t, -165.f, -250.f, -60.f, 0.7f);
_DEFINE_TUNABLE_PARAMETER(SeeQuietScoreThreshold, int32_t, 2048.f, 1000.f, 4000.f, 1.f);
_DEFINE_TUNABLE_PARAMETER(SeeQuietPruneThreshold, int32_t, -65.f, -90.f, -30.f, 0.9f);
_DEFINE_TUNABLE_PARAMETER(QDeltaPruningEvalWeight, int32_t, 8.f, 0.f, 128.f, 0.6f);
_DEFINE_TUNABLE_PARAMETER(QBetaCutoffEvalWeight, int32_t, 120.f, 0.f, 128.f, 0.6f);

/* Static parameters -
*  These are not tuned.
*/

inline constexpr int32_t SingularExtensionDiv = 16;
inline constexpr int32_t SingularBetaExtensionDiv = 32;
inline constexpr int32_t CheckNodeCount = 2048;
inline constexpr bool    UseSyzygyTablebase = _USE_SYZYGY_TB;
inline constexpr bool    UseSyzygyTablebaseRoot = _USE_SYZYGY_TB_ROOT;
inline constexpr double  MinTimeBranchFactor = 1.;
inline constexpr double  MaxTimeBranchFactor = 5.;
inline constexpr int32_t FixedPointMult = 65536;
inline constexpr int32_t CaptureTotalReductionRate = 256;
inline constexpr int32_t QuietTotalReductionRate = 256;
inline constexpr int32_t TablebaseWinScore = 31000;

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

    Score correctedEvalScore(Score eval, Score score);

    int16_t getRfpQuietHistPenalty(NodeInfo* parent_node);

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
