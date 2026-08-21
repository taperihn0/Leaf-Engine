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

#include "Game.hpp"
#include "Cuckoo.hpp"
#include "SearchUtils.hpp"
#include "Tablebase.hpp"

namespace engine {

/*
*   Tunable parameters in Search.
*/

_DEFINE_TUNABLE_PARAMETER(IidDepth, int32_t, 3.18375f, 2.f, 5.f, 0.9f);
_DEFINE_TUNABLE_PARAMETER(IidDepthDiv, int32_t, 12.4526f, 8.f, 16.f, 0.9f);
_DEFINE_TUNABLE_PARAMETER(RfpDepth, int32_t, 5.56051f, 2.f, 6.f, 0.9f);
_DEFINE_TUNABLE_PARAMETER(RazorDepth, int32_t, 2.04193f, 1.f, 4.f, 0.9f);
_DEFINE_TUNABLE_PARAMETER(FutilityDepth, int32_t, 2.97471f, 2.f, 5.f, 0.9f);
_DEFINE_TUNABLE_PARAMETER(LmrDepth, int32_t, 1.8089f, 1.f, 4.f, 1.5f);
_DEFINE_TUNABLE_PARAMETER(LmrBaseQuietReduction, int32_t, 164.885f, 130.f, 190.f, 0.4f);
_DEFINE_TUNABLE_PARAMETER(LmrLogQuietDepthMovesMult, int32_t, 68.363f, 50.f, 90.f, 0.4f);
_DEFINE_TUNABLE_PARAMETER(LmrBaseCaptureReduction, int32_t, 132.546f, 110.f, 150.f, 0.4f);
_DEFINE_TUNABLE_PARAMETER(LmrLogCaptureDepthMovesMult, int32_t, 58.315f, 40.f, 76.f, 0.4f);
_DEFINE_TUNABLE_PARAMETER(NullDepthMult, int32_t, 55.1882f, 35.f, 75.f, 0.7f);
_DEFINE_TUNABLE_PARAMETER(LmrMoveCount, int32_t, 5.13746f, 1.f, 16.f, 0.6f);
_DEFINE_TUNABLE_PARAMETER(RazorMultDelta, int32_t, 8.95066f, 5.f, 40.f, 0.4f);
_DEFINE_TUNABLE_PARAMETER(RazorCutDelta, int32_t, 12.5004f, 5.f, 35.f, 1.f);
_DEFINE_TUNABLE_PARAMETER(RazorBetaLimit, int32_t, 1200.72f, 1000.f, 2000.f, 0.5f);
_DEFINE_TUNABLE_PARAMETER(RfpMultDelta, int32_t, 110.309f, 10.f, 220.f, 0.4f);
_DEFINE_TUNABLE_PARAMETER(RfpMarginThreshold, int32_t, 4.2035f, 1.f, 30.f, 1.2f);
_DEFINE_TUNABLE_PARAMETER(RfpReturnValueWeight, int32_t, 62.5048f, 4.f, 128.f, 0.5f);
_DEFINE_TUNABLE_PARAMETER(FutilityMoveCount, int32_t, 7.81757f, 1.f, 16.f, 0.5f);
_DEFINE_TUNABLE_PARAMETER(FutilityDelta, int32_t, 15.5301f, 2.f, 50.f, 0.7f);
_DEFINE_TUNABLE_PARAMETER(FutilityScoreMult, int32_t, 9.75754f, 2.f, 13.f, 0.9f);
_DEFINE_TUNABLE_PARAMETER(RazorBaseDelta, int32_t, 106.164f, 20.f, 500.f, 0.4f);
_DEFINE_TUNABLE_PARAMETER(QMaterialDelta, int32_t, 1119.44f, 700.f, 1200.f, 0.6f);
_DEFINE_TUNABLE_PARAMETER(NNEvalScale, int32_t, 7.70538f, 7.f, 17.f, 1.3f);
_DEFINE_TUNABLE_PARAMETER(ImprovingRate, int32_t, 40.7981f, 30.f, 90.f, 0.6f);
_DEFINE_TUNABLE_PARAMETER(RfpImprovingSinkMult, int32_t, 127.949f, 100.f, 160.f, 0.6f);
_DEFINE_TUNABLE_PARAMETER(NullMargin, int32_t, 4.0552f, 1.f, 14.f, 0.5f);
_DEFINE_TUNABLE_PARAMETER(NullImprovingSinkMult, int32_t, 29.7565f, 16.f, 38.f, 1.f);
_DEFINE_TUNABLE_PARAMETER(TTEvalCorrRate, int32_t, 1.79133f, 1.f, 4.f, 0.8f);
_DEFINE_TUNABLE_PARAMETER(NullDiffScale, int32_t, 1057.86f, 800.f, 1300.f, 0.5f);
_DEFINE_TUNABLE_PARAMETER(NullDepth, int32_t, 2.63786f, 2.f, 5.f, 1.3f);
_DEFINE_TUNABLE_PARAMETER(NextDepthTimeRed, int32_t, 7.25379f, 4.f, 8.f, 1.2f);
_DEFINE_TUNABLE_PARAMETER(UnstableMatMargin, int32_t, 12.174f, 10.f, 80.f, 1.f);
_DEFINE_TUNABLE_PARAMETER(UnstableMultMargin, int32_t, 7.68833f, 4.f, 12.f, 2.5f);
_DEFINE_TUNABLE_PARAMETER(ContemptDiv, int32_t, 95.1723f, 30.f, 160.f, 1.7f);
_DEFINE_TUNABLE_PARAMETER(ImprovingExtensionRate, int32_t, 30.7245f, 10.f, 70.f, 0.8f);
_DEFINE_TUNABLE_PARAMETER(QuietNotPvNodeReduction, int32_t, 70.4208f, 40.f, 200.f, 1.5f);
_DEFINE_TUNABLE_PARAMETER(QuietCutNodeReduction, int32_t, 13.3248f, 0.f, 40.f, 0.5f);
_DEFINE_TUNABLE_PARAMETER(QuietCheckReduction, int32_t, 363.279f, 100.f, 600.f, 0.9f);
_DEFINE_TUNABLE_PARAMETER(QuietExtensionReduction, int32_t, 158.601f, 40.f, 200.f, 1.5f);
_DEFINE_TUNABLE_PARAMETER(QuietPawnMoveReduction, int32_t, 44.1255f, 10.f, 70.f, 0.8f);
_DEFINE_TUNABLE_PARAMETER(QuietImprovingReductionRate, int32_t, 24.455f, 5.f, 50.f, 0.5f);
_DEFINE_TUNABLE_PARAMETER(QuietHashCapReduction, int32_t, 152.965f, 40.f, 220.f, 1.5f);
_DEFINE_TUNABLE_PARAMETER(QuietKillerMoveReduction, int32_t, 238.954f, 80.f, 380.f, 1.f);
_DEFINE_TUNABLE_PARAMETER(CaptureNotPvNodeReduction, int32_t, 83.0206f, 30.f, 160.f, 1.2f);
_DEFINE_TUNABLE_PARAMETER(CaptureCutNodeReduction, int32_t, 59.3601f, 20.f, 110.f, 0.9f);
_DEFINE_TUNABLE_PARAMETER(CaptureCheckReduction, int32_t, 122.593f, 50.f, 300.f, 2.0f);
_DEFINE_TUNABLE_PARAMETER(CaptureHashCapReduction, int32_t, 135.385f, 50.f, 260.f, 1.8f);
_DEFINE_TUNABLE_PARAMETER(CaptureKillerMoveReduction, int32_t, 147.625f, 50.f, 280.f, 1.8f);
_DEFINE_TUNABLE_PARAMETER(CaptureExtensionReduction, int32_t, 72.7094f, 20.f, 140.f, 1.0f);
_DEFINE_TUNABLE_PARAMETER(CaptureImprovingReductionRate, int32_t, 27.1306f, 8.f, 55.f, 0.5f);
_DEFINE_TUNABLE_PARAMETER(EvalHalfMovesEvalLimit, int32_t, 14.4607f, 6.f, 25.f, 1.1f);
_DEFINE_TUNABLE_PARAMETER(EvalEgHalfMovesEvalLimit, int32_t, 20.4607f, 6.f, 30.f, 1.1f);
_DEFINE_TUNABLE_PARAMETER(NullVerifyDepth, int32_t, 8.9219f, 5.f, 16.f, 1.3f);
_DEFINE_TUNABLE_PARAMETER(MoveCheckExtensionRate, int32_t, 10.5909f, 8.f, 16.f, 1.8f);
_DEFINE_TUNABLE_PARAMETER(MoveCheckExtensionDiv, int32_t, 9.53864f, 8.f, 16.f, 1.4f);
_DEFINE_TUNABLE_PARAMETER(ImprovingExtensionMateRate, int32_t, 20.3026f, 12.f, 28.f, 1.f);
_DEFINE_TUNABLE_PARAMETER(MateThreadFracExtensionRate, int32_t, 10.4043f, 10.f, 30.f, 0.9f);
_DEFINE_TUNABLE_PARAMETER(MateThreadFracExtensionDiv, int32_t, 16.7514f, 12.f, 26.f, 1.f);
_DEFINE_TUNABLE_PARAMETER(MaxMoveExtensionRate, int32_t, 14.2369f, 10.f, 30.f, 1.7f);
_DEFINE_TUNABLE_PARAMETER(MaxMoveExtensionDiv, int32_t, 10.0644f, 10.f, 18.f, 1.f);
_DEFINE_TUNABLE_PARAMETER(NullVerifyDepthMult, int32_t, 15.8f, 6.f, 25.f, 1.f);
_DEFINE_TUNABLE_PARAMETER(ExtensionDepth, int32_t, 15.9831f, 4.f, 32.f, 1.1f);
_DEFINE_TUNABLE_PARAMETER(SingularDepth, int32_t, 3.4227f, 2.f, 8.f, 1.9f);
_DEFINE_TUNABLE_PARAMETER(SingularDepthMargin, int32_t, 2.21081f, 1.f, 4.f, 1.2f);
_DEFINE_TUNABLE_PARAMETER(SingularExtensionRate, int32_t, 1.20531f, 0.8f, 12.f, 1.8f);
_DEFINE_TUNABLE_PARAMETER(SingularBetaDepthMult, int32_t, 54.2774f, 1.f, 96.f, 1.2f);
_DEFINE_TUNABLE_PARAMETER(SingularDepthMult, int32_t, 129.993f, 90.f, 180.f, 1.3f);
_DEFINE_TUNABLE_PARAMETER(SingularDepthBase, int32_t, 535.757f, 400.f, 650.f, 0.6f);
_DEFINE_TUNABLE_PARAMETER(SingularBetaExtensionRate, int32_t, 10.5274f, 1.f, 15.f, 1.2f);
_DEFINE_TUNABLE_PARAMETER(TablebaseProbeDepth, int32_t, 8.29697f, 2.f, 16.f, 2.f);
_DEFINE_TUNABLE_PARAMETER(TablebasePieceCountLimit, int32_t, 5.63622f, 2.f, 10.f, 1.7f);
_DEFINE_TUNABLE_PARAMETER(AspirationSearchDepth, int32_t, 3.63818f, 2.f, 5.f, 1.5f);
_DEFINE_TUNABLE_PARAMETER(AspirationFirstWindow, int32_t, 63.6752f, 10.f, 120.f, 0.9f);
_DEFINE_TUNABLE_PARAMETER(AspirationUnstableFactor, int32_t, 190.508f, 10.f, 220.f, 0.8f);
_DEFINE_TUNABLE_PARAMETER(AspirationDepthRate, int32_t, 0.927315f, 0.f, 1.5f, 0.3f);
_DEFINE_TUNABLE_PARAMETER(AspirationMaxDepthInfl, int32_t, 5.72772f, 1.f, 14.f, 1.1f);
_DEFINE_TUNABLE_PARAMETER(AspirationWindowScoreDiv, int32_t, 5223.4f, 2000.f, 6500.f, 0.2f);
_DEFINE_TUNABLE_PARAMETER(AspirationMaxWindow, int32_t, 801.184f, 400.f, 1000.f, 0.6f);
_DEFINE_TUNABLE_PARAMETER(AspirationCount, int32_t, 3.14328f, 2.f, 5.f, 1.f);
_DEFINE_TUNABLE_PARAMETER(AspirationWidenRate, int32_t, 4.97933f, 2.f, 5.f, 1.f);
_DEFINE_TUNABLE_PARAMETER(QSeePruningThreshold, int32_t, -9.63953f, -150.f, 80.f, 1.2f);
_DEFINE_TUNABLE_PARAMETER(RfpQuietPenaltyMult, int32_t, 13.8314f, 10.f, 40.f, 1.1f);
_DEFINE_TUNABLE_PARAMETER(SeePruneDepth, int32_t, 1.57986f, 0.f, 5.f, 0.9f);
_DEFINE_TUNABLE_PARAMETER(SeePruneMarginMult, int32_t, 10.813f, 0.f, 20.f, 0.8f);
_DEFINE_TUNABLE_PARAMETER(SeeCapturePruneThreshold, int32_t, -143.187f, -250.f, -60.f, 0.7f);
_DEFINE_TUNABLE_PARAMETER(SeeQuietScoreThreshold, int32_t, 1719.19f, 1000.f, 4000.f, 1.f);
_DEFINE_TUNABLE_PARAMETER(SeeQuietPruneThreshold, int32_t, -71.9894f, -90.f, -30.f, 0.9f);
_DEFINE_TUNABLE_PARAMETER(QDeltaPruningEvalWeight, int32_t, 5.17858f, 0.f, 128.f, 0.6f);
_DEFINE_TUNABLE_PARAMETER(QBetaCutoffEvalWeight, int32_t, 124.406f, 0.f, 128.f, 0.6f);
_DEFINE_TUNABLE_PARAMETER(QuietMoveScoreReductionRate, int32_t, 2420.91f, 2300.f, 2500.f, 0.2f);
_DEFINE_TUNABLE_PARAMETER(CaptureMoveScoreReductionRate, int32_t, 11.4842f, 7.f, 15.f, 0.6f);
_DEFINE_TUNABLE_PARAMETER(QuietDepthShiftMult, int32_t, 257.727f, 200.f, 300.f, 0.2f);

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
inline constexpr int32_t MoveReductionBase = 256;
inline constexpr int32_t TablebaseWinScore = 32000;
inline constexpr int32_t TablebasePieceDiffMult = 116;
inline constexpr int32_t TablebaseScoreScale = 16;

class SearchLimitsWrapper;
class SearchResultsWrapper;
enum enumParameterIndex : uint8_t;

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

class Search {
public:
    explicit Search(tt::TranspositionTable&& tt);
    ~Search();
    
    Search()                      = delete;
    Search(Search&&)              = delete;
    Search(Search&)               = delete;
    Search& operator=(Search&)    = delete;
    Search& operator=(Search&& t) = delete;

    template <enumInfoLevel InfoLevel = SEARCH_FULL_INFO>
    _NODISCARD Move32b findBestMove(Position& pos, 
                                    const FullInfoRecord& game, 
                                    SearchLimits limits);

    template <enumInfoLevel InfoLevel = SEARCH_FULL_INFO>
    _NODISCARD Move32b findBestMove(Position& pos, 
                                    const FullInfoRecord& game, 
                                    SearchLimits limits,
                                    SearchResults& results);

    static Move32b _findBestMove_unittest(Search& search, 
                                          Position& pos, 
                                          const FullInfoRecord& game, 
                                          SearchLimits limits);
    
    void clearHash();
    void resizeHash(size_t tt_size_mb);
    void onNewGame();
private:
    Move32b goIterativeDeepening(Position& pos, 
                                 const FullInfoRecord& game, 
                                 const SearchLimitsWrapper& limits,
                                 SearchResultsWrapper& search_results,
                                 enumInfoLevel info_lv);

    bool goSearch(Position& pos, 
                  const FullInfoRecord& game, 
                  const SearchLimitsWrapper& limits, 
                  SearchResultsWrapper& results,
                  sc::Score alpha, sc::Score beta);

    template <enumNode NmNodeType, bool AllowNullMove, bool Root = false>
    sc::Score nmSearch(Position& pos, 
                       const SearchLimitsWrapper& limits, 
                       SearchResultsWrapper& results, 
                       const FullInfoRecord& game, 
                       NodeInfo* node,
                       sc::Score alpha, sc::Score beta, 
                       int depth, int ply);

    template <enumNode QNodeType, bool Root = false>
    sc::Score qSearch(Position& pos, 
                      const SearchLimitsWrapper& limits, 
                      SearchResultsWrapper& results, 
                      NodeInfo* node, 
                      sc::Score alpha, sc::Score beta, 
                      int depth, int ply);
    
    sc::Score getDrawScore(const NodeInfo* node) const;
    sc::Score getTablebaseScore(SyzygyTablebase::TbWdlInfo wdl, 
                                const Position& pos, 
                                const NodeInfo* node, 
                                int ply) const;
    bool isTablebaseScore(sc::Score score) const;
    sc::Score applyContempt(sc::Score score, const NodeInfo* node) const;

    template <enumNode NodeType>
    sc::Score evaluate(const Position& pos,
                       SearchStack* tree_stack,
                       NodeInfo* node,
                       NodeInfo* preroot, 
                       enumColor side2move, 
                       _MAYBE_UNUSED SearchResultsWrapper& results);

    sc::Score correctedEvalScore(sc::Score eval, sc::Score score);

    int16_t getRfpQuietHistPenalty(NodeInfo* parent_node);

    int getNullSearchDepth(sc::Score eval, sc::Score beta, int depth);
    int getNullVerifyDepth(int nm_depth);

    template <bool IsPv>
    _NODISCARD int32_t getMoveReduction(const NodeInfo* node, 
                                        int depth, 
                                        int32_t move_extension,
                                        Move32b tt_move, 
                                        Move32b killer);

    template <bool IsPv, enumParameterIndex Index>
    _NODISCARD int32_t getMoveReduction(const NodeInfo* node, 
                                        int depth, 
                                        int32_t move_extension,
                                        Move32b tt_move, 
                                        Move32b killer);

    template <enumParameterIndex Index>
    _NODISCARD static int32_t getScoreMoveReduction(mvo::SMoveScore s);

    void refreshPVinTT(const Position& pos, 
                       const std::array<PvInfo, MaxSelDepth>& root_pv_line, 
                       uint16_t pv_len,
                       SearchResultsWrapper& results);

    template <bool IsPv>
    bool isRepetitionCycle(const Position& pos, 
                           const FullInfoRecord& game, 
                           const NodeInfo* node, 
                           int ply,
                           SearchResultsWrapper& results);

    bool canRepetitionDraw(const Position& pos, 
                           const NodeInfo* node, 
                           int ply);

    bool isInsufficientMaterial(const Position& pos);

    tt::TranspositionTable       _tt;
    std::unique_ptr<SearchStack> _stack;
    CuckooTables                 _cuckoo_tables;

    /* Each Search instance should have own history buffer with tables 
    *  for very MoveOrder in SearchStack.
    *  Also, Search class in responsible for allocation and deallocation.
    */
    mem::AlignedSharedPtr<mvo::HistoryTablesCluster> 
              _history_cluster;
    sc::Score _contempt = sc::Undef;
};

constexpr enumNode operator|(enumNode node0, enumNode node1) {
    return static_cast<enumNode>(static_cast<int>(node0) | static_cast<int>(node1));
}

} // namespace engine
