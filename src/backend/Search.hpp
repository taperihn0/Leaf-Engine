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

namespace search {

/*
*   Tunable parameters in Search.
*/

_DEFINE_TUNABLE_PARAMETER_I32(IirDepth, 6.18375f, 2.f, 8.f, 1.2f);
_DEFINE_TUNABLE_PARAMETER_I32(RfpDepth, 5.56051f, 2.f, 6.f, 0.9f);
_DEFINE_TUNABLE_PARAMETER_I32(RazorDepth, 2.04193f, 1.f, 4.f, 0.9f);
_DEFINE_TUNABLE_PARAMETER_I32(FutilityDepth, 2.97471f, 2.f, 5.f, 0.9f);
_DEFINE_TUNABLE_PARAMETER_I32(LmrDepth, 1.8089f, 1.f, 4.f, 1.5f);
_DEFINE_TUNABLE_PARAMETER_I32(LmrBaseQuietReduction, 164.885f, 130.f, 190.f, 0.4f);
_DEFINE_TUNABLE_PARAMETER_I32(LmrLogQuietDepthMovesMult, 68.363f, 50.f, 90.f, 0.4f);
_DEFINE_TUNABLE_PARAMETER_I32(LmrBaseCaptureReduction, 132.546f, 110.f, 150.f, 0.4f);
_DEFINE_TUNABLE_PARAMETER_I32(LmrLogCaptureDepthMovesMult, 58.315f, 40.f, 76.f, 0.4f);
_DEFINE_TUNABLE_PARAMETER_I32(NullDepthMult, 55.1882f, 35.f, 75.f, 0.7f);
_DEFINE_TUNABLE_PARAMETER_I32(LmrMoveCount, 5.13746f, 1.f, 16.f, 0.6f);
_DEFINE_TUNABLE_PARAMETER_I32(RazorMultDelta, 8.95066f, 5.f, 40.f, 0.4f);
_DEFINE_TUNABLE_PARAMETER_I32(RazorCutDelta, 12.5004f, 5.f, 35.f, 1.f);
_DEFINE_TUNABLE_PARAMETER_I32(RazorBetaLimit, 1200.72f, 1000.f, 2000.f, 0.5f);
_DEFINE_TUNABLE_PARAMETER_I32(RfpMultDelta, 110.309f, 10.f, 220.f, 0.4f);
_DEFINE_TUNABLE_PARAMETER_I32(RfpMarginThreshold, 4.2035f, 1.f, 30.f, 1.2f);
_DEFINE_TUNABLE_PARAMETER_I32(RfpReturnValueWeight, 62.5048f, 4.f, 128.f, 0.5f);
_DEFINE_TUNABLE_PARAMETER_I32(FutilityMoveCount, 7.81757f, 1.f, 16.f, 0.5f);
_DEFINE_TUNABLE_PARAMETER_I32(FutilityDelta, 15.5301f, 2.f, 50.f, 0.7f);
_DEFINE_TUNABLE_PARAMETER_I32(FutilityScoreMult, 9.75754f, 2.f, 13.f, 0.9f);
_DEFINE_TUNABLE_PARAMETER_I32(RazorBaseDelta, 106.164f, 20.f, 500.f, 0.4f);
_DEFINE_TUNABLE_PARAMETER_I32(QMaterialDelta, 1119.44f, 700.f, 1200.f, 0.6f);
_DEFINE_TUNABLE_PARAMETER_I32(NNEvalScale, 7.70538f, 7.f, 17.f, 1.3f);
_DEFINE_TUNABLE_PARAMETER_I32(ImprovingRate, 40.7981f, 30.f, 90.f, 0.6f);
_DEFINE_TUNABLE_PARAMETER_I32(RfpImprovingSinkMult, 127.949f, 100.f, 160.f, 0.6f);
_DEFINE_TUNABLE_PARAMETER_I32(NullMargin, 4.0552f, 1.f, 14.f, 0.5f);
_DEFINE_TUNABLE_PARAMETER_I32(NullImprovingSinkMult, 29.7565f, 16.f, 38.f, 1.f);
_DEFINE_TUNABLE_PARAMETER_I32(TTEvalCorrRate, 1.79133f, 1.f, 4.f, 0.8f);
_DEFINE_TUNABLE_PARAMETER_I32(NullDiffScale, 1057.86f, 800.f, 1300.f, 0.5f);
_DEFINE_TUNABLE_PARAMETER_I32(NullDepth, 2.63786f, 2.f, 5.f, 1.3f);
_DEFINE_TUNABLE_PARAMETER_I32(NextDepthTimeRed, 7.25379f, 4.f, 8.f, 1.2f);
_DEFINE_TUNABLE_PARAMETER_I32(UnstableMatMargin, 12.174f, 10.f, 80.f, 1.f);
_DEFINE_TUNABLE_PARAMETER_I32(UnstableMultMargin, 7.68833f, 4.f, 12.f, 2.5f);
_DEFINE_TUNABLE_PARAMETER_I32(ContemptDiv, 95.1723f, 30.f, 160.f, 1.7f);
_DEFINE_TUNABLE_PARAMETER_I32(ImprovingExtensionRate, 30.7245f, 10.f, 70.f, 0.8f);
_DEFINE_TUNABLE_PARAMETER_I32(QuietNotPvNodeReduction, 70.4208f, 40.f, 200.f, 1.5f);
_DEFINE_TUNABLE_PARAMETER_I32(QuietCutNodeReduction, 13.3248f, 0.f, 40.f, 0.5f);
_DEFINE_TUNABLE_PARAMETER_I32(QuietCheckReduction, 363.279f, 100.f, 600.f, 0.9f);
_DEFINE_TUNABLE_PARAMETER_I32(QuietExtensionReduction, 158.601f, 40.f, 200.f, 1.5f);
_DEFINE_TUNABLE_PARAMETER_I32(QuietPawnMoveReduction, 44.1255f, 10.f, 70.f, 0.8f);
_DEFINE_TUNABLE_PARAMETER_I32(QuietImprovingReductionRate, 24.455f, 5.f, 50.f, 0.5f);
_DEFINE_TUNABLE_PARAMETER_I32(QuietHashCapReduction, 152.965f, 40.f, 220.f, 1.5f);
_DEFINE_TUNABLE_PARAMETER_I32(QuietKillerMoveReduction, 238.954f, 80.f, 380.f, 1.f);
_DEFINE_TUNABLE_PARAMETER_I32(CaptureNotPvNodeReduction, 83.0206f, 30.f, 160.f, 1.2f);
_DEFINE_TUNABLE_PARAMETER_I32(CaptureCutNodeReduction, 59.3601f, 20.f, 110.f, 0.9f);
_DEFINE_TUNABLE_PARAMETER_I32(CaptureCheckReduction, 122.593f, 50.f, 300.f, 2.0f);
_DEFINE_TUNABLE_PARAMETER_I32(CaptureHashCapReduction, 135.385f, 50.f, 260.f, 1.8f);
_DEFINE_TUNABLE_PARAMETER_I32(CaptureKillerMoveReduction, 147.625f, 50.f, 280.f, 1.8f);
_DEFINE_TUNABLE_PARAMETER_I32(CaptureExtensionReduction, 72.7094f, 20.f, 140.f, 1.0f);
_DEFINE_TUNABLE_PARAMETER_I32(CaptureImprovingReductionRate, 27.1306f, 8.f, 55.f, 0.5f);
_DEFINE_TUNABLE_PARAMETER_I32(EvalHalfMovesEvalLimit, 14.4607f, 6.f, 25.f, 1.1f);
_DEFINE_TUNABLE_PARAMETER_I32(EvalEgHalfMovesEvalLimit, 20.4607f, 6.f, 30.f, 1.1f);
_DEFINE_TUNABLE_PARAMETER_I32(MoveCheckExtensionRate, 10.5909f, 8.f, 16.f, 1.8f);
_DEFINE_TUNABLE_PARAMETER_I32(MoveCheckExtensionDiv, 9.53864f, 8.f, 16.f, 1.4f);
_DEFINE_TUNABLE_PARAMETER_I32(ImprovingExtensionMateRate, 20.3026f, 12.f, 28.f, 1.f);
_DEFINE_TUNABLE_PARAMETER_I32(MateThreadFracExtensionRate, 10.4043f, 10.f, 30.f, 0.9f);
_DEFINE_TUNABLE_PARAMETER_I32(MateThreadFracExtensionDiv, 16.7514f, 12.f, 26.f, 1.f);
_DEFINE_TUNABLE_PARAMETER_I32(MaxMoveExtensionRate, 14.2369f, 10.f, 30.f, 1.7f);
_DEFINE_TUNABLE_PARAMETER_I32(MaxMoveExtensionDiv, 10.0644f, 10.f, 18.f, 1.f);
_DEFINE_TUNABLE_PARAMETER_I32(ExtensionDepth, 15.9831f, 4.f, 32.f, 1.1f);
_DEFINE_TUNABLE_PARAMETER_I32(SingularDepth, 5.4227f, 2.f, 8.f, 1.9f);
_DEFINE_TUNABLE_PARAMETER_I32(SingularDepthMargin, 0.48081f, 0.f, 3.f, 1.2f);
_DEFINE_TUNABLE_PARAMETER_I32(SingularExtensionRate, 1.20531f, 0.8f, 12.f, 1.8f);
_DEFINE_TUNABLE_PARAMETER_I32(SingularBetaDepthMult, 54.2774f, 1.f, 96.f, 1.2f);
_DEFINE_TUNABLE_PARAMETER_I32(SingularDepthMult, 129.993f, 90.f, 180.f, 1.3f);
_DEFINE_TUNABLE_PARAMETER_I32(SingularDepthBase, 535.757f, 400.f, 620.f, 0.6f);
_DEFINE_TUNABLE_PARAMETER_I32(SingularBetaExtensionRate, 10.5274f, 1.f, 15.f, 1.2f);
_DEFINE_TUNABLE_PARAMETER_I32(TablebaseProbeDepth, 8.29697f, 2.f, 16.f, 2.f);
_DEFINE_TUNABLE_PARAMETER_I32(TablebasePieceCountLimit, 5.63622f, 2.f, 10.f, 1.7f);
_DEFINE_TUNABLE_PARAMETER_I32(AspirationSearchDepth, 3.63818f, 2.f, 5.f, 1.5f);
_DEFINE_TUNABLE_PARAMETER_I32(AspirationFirstWindow, 63.6752f, 10.f, 120.f, 0.9f);
_DEFINE_TUNABLE_PARAMETER_I32(AspirationUnstableFactor, 190.508f, 10.f, 220.f, 0.8f);
_DEFINE_TUNABLE_PARAMETER_I32(AspirationDepthRate, 0.927315f, 0.f, 1.5f, 0.3f);
_DEFINE_TUNABLE_PARAMETER_I32(AspirationMaxDepthInfl, 5.72772f, 1.f, 14.f, 1.1f);
_DEFINE_TUNABLE_PARAMETER_I32(AspirationWindowScoreDiv, 5223.4f, 2000.f, 6500.f, 0.2f);
_DEFINE_TUNABLE_PARAMETER_I32(AspirationMaxWindow, 801.184f, 400.f, 1000.f, 0.6f);
_DEFINE_TUNABLE_PARAMETER_I32(AspirationCount, 3.14328f, 2.f, 5.f, 1.f);
_DEFINE_TUNABLE_PARAMETER_I32(AspirationWidenRate, 4.97933f, 2.f, 5.f, 1.f);
_DEFINE_TUNABLE_PARAMETER_I32(QSeePruningThreshold, -9.63953f, -150.f, 80.f, 1.2f);
_DEFINE_TUNABLE_PARAMETER_I32(RfpQuietPenaltyMult, 13.8314f, 10.f, 40.f, 1.1f);
_DEFINE_TUNABLE_PARAMETER_I32(SeePruneDepth, 1.57986f, 0.f, 5.f, 0.9f);
_DEFINE_TUNABLE_PARAMETER_I32(SeePruneMarginMult, 10.813f, 0.f, 20.f, 0.8f);
_DEFINE_TUNABLE_PARAMETER_I32(SeeCapturePruneThreshold, -143.187f, -250.f, -60.f, 0.7f);
_DEFINE_TUNABLE_PARAMETER_I32(SeeQuietScoreThreshold, 1719.19f, 1000.f, 4000.f, 1.f);
_DEFINE_TUNABLE_PARAMETER_I32(SeeQuietPruneThreshold, -71.9894f, -90.f, -30.f, 0.9f);
_DEFINE_TUNABLE_PARAMETER_I32(QDeltaPruningEvalWeight, 5.17858f, 0.f, 128.f, 0.6f);
_DEFINE_TUNABLE_PARAMETER_I32(QBetaCutoffEvalWeight, 124.406f, 0.f, 128.f, 0.6f);
_DEFINE_TUNABLE_PARAMETER_I32(QuietMoveScoreReductionRate, 2420.91f, 2300.f, 2500.f, 0.2f);
_DEFINE_TUNABLE_PARAMETER_I32(CaptureMoveScoreReductionRate, 11.4842f, 7.f, 15.f, 0.6f);
_DEFINE_TUNABLE_PARAMETER_I32(QuietDepthShiftMult, 257.727f, 200.f, 300.f, 0.2f);
_DEFINE_TUNABLE_PARAMETER_I32(ProbBetaMargin, 155.f, 120.f, 190.f, 0.8f);
_DEFINE_TUNABLE_PARAMETER_I32(ProbDepthThreshold, 6.f, 4.f, 8.f, 1.5f);
_DEFINE_TUNABLE_PARAMETER_I32(ProbHashDepthDiffThreshold, 0.48f, 0.f, 3.f, 1.2f);
_DEFINE_TUNABLE_PARAMETER_I32(ProbDepthReduction, 5.f, 3.f, 7.f, 1.5f);

/* Static parameters -
*  These are not tuned.
*/

inline constexpr int32_t CheckNodeCount = 2048;
inline constexpr bool    UseSyzygyTablebase = _USE_SYZYGY_TB;
inline constexpr bool    UseSyzygyTablebaseRoot = _USE_SYZYGY_TB_ROOT;
inline constexpr double  MinTimeBranchFactor = 1.;
inline constexpr double  MaxTimeBranchFactor = 5.;
inline constexpr int32_t FixedPointMult = 65536;
inline constexpr int32_t MoveReductionBase = 256;
inline constexpr int32_t TablebaseWinScore = 31000;
inline constexpr int32_t TablebasePieceDiffMult = 116;
inline constexpr int32_t SingularExtensionDiv = 16;
inline constexpr int32_t SingularBetaExtensionDiv = 32;

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
                                    utils::SearchLimits limits);

    template <enumInfoLevel InfoLevel = SEARCH_FULL_INFO>
    _NODISCARD Move32b findBestMove(Position& pos, 
                                    const FullInfoRecord& game, 
                                    utils::SearchLimits limits,
                                    utils::SearchResults& results);

    static Move32b _findBestMove_unittest(Search& search, 
                                          Position& pos, 
                                          const FullInfoRecord& game, 
                                          utils::SearchLimits limits);
    
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
                       utils::NodeInfo* node,
                       sc::Score alpha, sc::Score beta, 
                       int depth, int ply);

    template <enumNode QNodeType, bool Root = false>
    sc::Score qSearch(Position& pos, 
                      const SearchLimitsWrapper& limits, 
                      SearchResultsWrapper& results, 
                      utils::NodeInfo* node, 
                      sc::Score alpha, sc::Score beta, 
                      int depth, int ply);

    sc::Score probCut(Position& pos,
                      const SearchLimitsWrapper& limits, 
                      SearchResultsWrapper& results, 
                      const FullInfoRecord& game, 
                      utils::NodeInfo* node,
                      sc::Score beta,
                      const tt::TTEntryValues& tt_entry,
                      const Move32b tt_move,
                      int depth, int ply);

    sc::Score getDrawScore(const utils::NodeInfo* node) const;
    sc::Score getTablebaseScore(SyzygyTablebase::TbWdlInfo wdl, 
                                const Position& pos, 
                                const utils::NodeInfo* node, 
                                int ply) const;
    bool isTablebaseScore(sc::Score score) const;
    sc::Score applyContempt(sc::Score score, const utils::NodeInfo* node) const;

    template <enumNode NodeType>
    sc::Score evaluate(const Position& pos,
                       utils::NodeInfo* node,
                       enumColor side2move, 
                       _MAYBE_UNUSED SearchResultsWrapper& results);

    sc::Score correctedEvalScore(sc::Score eval, sc::Score score);

    int16_t getRfpQuietHistPenalty(utils::NodeInfo* parent_node);

    int getNullSearchDepth(sc::Score eval, sc::Score beta, int depth);

    template <bool IsPv>
    _NODISCARD int32_t getMoveReduction(const utils::NodeInfo* node, 
                                        int depth, 
                                        int32_t move_extension,
                                        Move32b tt_move, 
                                        Move32b killer);

    template <bool IsPv, enumParameterIndex Index>
    _NODISCARD int32_t getMoveReduction(const utils::NodeInfo* node, 
                                        int depth, 
                                        int32_t move_extension,
                                        Move32b tt_move, 
                                        Move32b killer);

    template <enumParameterIndex Index>
    _NODISCARD static int32_t getScoreMoveReduction(mvo::SMoveScore s);

    void refreshPVinTT(const Position& pos, 
                       const std::array<utils::PvInfo, MaxSelDepth>& root_pv_line, 
                       uint16_t pv_len,
                       SearchResultsWrapper& results);

    template <bool IsPv>
    bool isRepetitionCycle(const Position& pos, 
                           const FullInfoRecord& game, 
                           const utils::NodeInfo* node, 
                           int ply,
                           SearchResultsWrapper& results);

    bool canRepetitionDraw(const Position& pos, 
                           const utils::NodeInfo* node, 
                           int ply);

    bool isInsufficientMaterial(const Position& pos);

    tt::TranspositionTable       _tt;
    std::unique_ptr<utils::SearchStack> _stack;
    CuckooTables                 _cuckoo_tables;

    /* Each Search instance should have own history buffer with tables 
    *  for very MoveOrder in utils::SearchStack.
    *  Also, Search class in responsible for allocation and deallocation.
    */
    mem::AlignedSharedPtr<mvo::HistoryTablesCluster> 
              _history_cluster;
    sc::Score _contempt = sc::Undef;
};

constexpr enumNode operator|(enumNode node0, enumNode node1) {
    return static_cast<enumNode>(static_cast<int>(node0) | static_cast<int>(node1));
}

} // namespace search
