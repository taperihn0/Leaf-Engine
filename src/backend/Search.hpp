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
    Position::ReversibleState    state;
    Move32b                      move;
    Move32b                      best_move;
    Score                        score;
    Score                        eval;
    float                        improving;
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

_PARAM_ATTRIBS int IidDepth = roundi<float>(2.6917f);
_PARAM_ATTRIBS int IidDepthDiv = roundi<float>(12.0625f);
_PARAM_ATTRIBS int RfpDepth = roundi<float>(5.21208f);
_PARAM_ATTRIBS int RazorDepth = roundi<float>(1.55689f);
_PARAM_ATTRIBS int FutilityDepth = roundi<float>(2.80804f);
_PARAM_ATTRIBS int LmrDepth = roundi<float>(1.87427f);
_PARAM_ATTRIBS int LmrBaseReduction = roundi<float>(28.f);
_PARAM_ATTRIBS int LmrLogDepthMovesMult = roundi<float>(12.f);
_PARAM_ATTRIBS int NullReduction = roundi<float>(35.4356f);
_PARAM_ATTRIBS int LmrMoveCount = roundi<float>(4.68328f);
_PARAM_ATTRIBS int RazorMultDelta = roundi<float>(10.2223f);
_PARAM_ATTRIBS int RazorCutDelta = roundi<float>(10.f);
_PARAM_ATTRIBS int RazorBetaLimit = roundi<float>(1200.f);
_PARAM_ATTRIBS int RfpMultDelta = roundi<float>(113.653f);
_PARAM_ATTRIBS int RfpMarginThreshold = roundi<float>(6.f);
_PARAM_ATTRIBS int RfpReturnValueWeight = roundi<float>(64.f);
_PARAM_ATTRIBS int FutilityMoveCount = roundi<float>(7.8672f);
_PARAM_ATTRIBS int FutilityDelta = roundi<float>(22.8808f);
_PARAM_ATTRIBS int FutilityScoreMult = roundi<float>(9.f);
_PARAM_ATTRIBS int RazorBaseDelta = roundi<float>(119.431f);
_PARAM_ATTRIBS int QMaterialDelta = roundi<float>(1106.39f);
_PARAM_ATTRIBS int NNEvalScale = roundi<float>(13.7321f);
_PARAM_ATTRIBS int ImprovingRate = roundi<float>(39.2507f);
_PARAM_ATTRIBS int RfpImprovingSink = roundi<float>(1.86056f);
_PARAM_ATTRIBS int NullMargin = roundi<float>(4.33824f);
_PARAM_ATTRIBS int NullImprovingSink = roundi<float>(9.12689f);
_PARAM_ATTRIBS int TTEvalCorrRate = roundi<float>(2.36075f);
_PARAM_ATTRIBS int NullDiffScale = roundi<float>(1053.47f);
_PARAM_ATTRIBS int NullDepth = roundi<float>(3.01463f);
_PARAM_ATTRIBS int NextDepthTimeRed = roundi<float>(6.82642f);
_PARAM_ATTRIBS int UnstableMatMargin = roundi<float>(13.8272f);
_PARAM_ATTRIBS int UnstableMultMargin = roundi<float>(6.80492f);
_PARAM_ATTRIBS int ContemptDiv = roundi<float>(86.5476f);
_PARAM_ATTRIBS int ImprovingExtensionRate = roundi<float>(6.64036f);
_PARAM_ATTRIBS int QuietNotPvNodeReduction = roundi<float>(19.8129f);
_PARAM_ATTRIBS int QuietCutNodeReduction = roundi<float>(2.41664f);
_PARAM_ATTRIBS int QuietCheckReduction = roundi<float>(59.3266f);
_PARAM_ATTRIBS int QuietExtensionReduction = roundi<float>(19.4389f);
_PARAM_ATTRIBS int QuietPawnMoveReduction = roundi<float>(6.08098f);
_PARAM_ATTRIBS int QuietImprovingReductionRate = roundi<float>(3.68642f);
_PARAM_ATTRIBS int QuietHashCapReduction = roundi<float>(22.385f);
_PARAM_ATTRIBS int QuietKillerMoveReduction = roundi<float>(36.6035f);
_PARAM_ATTRIBS int QuietTotalReductionRate = roundi<float>(43.95f);
_PARAM_ATTRIBS int CaptureNotPvNodeReduction = roundi<float>(19.0004f);
_PARAM_ATTRIBS int CaptureCutNodeReduction = roundi<float>(12.3796f);
_PARAM_ATTRIBS int CaptureCheckReduction = roundi<float>(35.2899f);
_PARAM_ATTRIBS int CaptureHashCapReduction = roundi<float>(32.1251f);
_PARAM_ATTRIBS int CaptureKillerMoveReduction = roundi<float>(33.5878f);
_PARAM_ATTRIBS int CaptureExtensionReduction = roundi<float>(15.8489f);
_PARAM_ATTRIBS int CaptureImprovingReductionRate = roundi<float>(5.63286f);
_PARAM_ATTRIBS int CaptureTotalReductionRate = roundi<float>(54.3614f);
_PARAM_ATTRIBS int HalfMovesEvalLimit = roundi<float>(11.8268f);
_PARAM_ATTRIBS int NullVerifyDepth = roundi<float>(10.12013f);
_PARAM_ATTRIBS int MoveCheckExtensionRate = roundi<float>(12.2576f);
_PARAM_ATTRIBS int MoveCheckExtensionDiv = roundi<float>(9.95162f);
_PARAM_ATTRIBS int ImprovingExtensionMateRate = roundi<float>(12.6104f);
_PARAM_ATTRIBS int MateThreadFracExtensionRate = roundi<float>(11.6797f);
_PARAM_ATTRIBS int MateThreadFracExtensionDiv = roundi<float>(17.9521f);
_PARAM_ATTRIBS int MaxMoveExtensionRate = roundi<float>(16.3261f);
_PARAM_ATTRIBS int MaxMoveExtensionDiv = roundi<float>(10.9455f);
_PARAM_ATTRIBS int NullVerifyDepthMult = roundi<float>(2.94352f);
_PARAM_ATTRIBS int ExtensionDepth = roundi<float>(16.8311f);
_PARAM_ATTRIBS int SingularDepth = roundi<float>(4.03396f);
_PARAM_ATTRIBS int SingularDepthMargin = roundi<float>(1.71455f);
_PARAM_ATTRIBS int SingularExtensionRate = roundi<float>(3.08547f);
_PARAM_ATTRIBS int SingularBetaDepthMult = roundi<float>(2.60718f);
_PARAM_ATTRIBS int SingularDepthMult = roundi<float>(124.551f);
_PARAM_ATTRIBS int SingularDepthBase = roundi<float>(535.727f);
_PARAM_ATTRIBS int SingularBetaExtensionRate = roundi<float>(8.8357f);
_PARAM_ATTRIBS int TablebaseProbeDepth = roundi<float>(7.33313f);
_PARAM_ATTRIBS int TablebasePieceCountLimit = roundi<float>(5.63201f);
_PARAM_ATTRIBS int TablebaseWinScore = roundi<float>(31036.8f);
_PARAM_ATTRIBS int TablebasePieceDiffMult = roundi<float>(105.374f);
_PARAM_ATTRIBS int TablebaseScoreScale = roundi<float>(17.0653f);
_PARAM_ATTRIBS int AspirationSearchDepth = roundi<float>(4.42753f);
_PARAM_ATTRIBS int AspirationFirstWindow = roundi<float>(68.2525f);
_PARAM_ATTRIBS int AspirationUnstableFactor = roundi<float>(179.176f);
_PARAM_ATTRIBS int AspirationDepthRate = roundi<float>(0.906031f);
_PARAM_ATTRIBS int AspirationMaxDepthInfl = roundi<float>(8.44154f);
_PARAM_ATTRIBS int AspirationWindowScoreDiv = roundi<float>(5208.23f);
_PARAM_ATTRIBS int AspirationMaxWindow = roundi<float>(736.662f);
_PARAM_ATTRIBS int AspirationCount = roundi<float>(3.4091f);
_PARAM_ATTRIBS int AspirationWidenRate = roundi<float>(4.68001f);
_PARAM_ATTRIBS int SeeValuePrune = roundi<float>(-8.86404f);
_PARAM_ATTRIBS int RfpQuietPenaltyMult = roundi<float>(19.f);

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
