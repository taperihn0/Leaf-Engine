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

#include "MoveList.hpp"
#include "MoveGen.hpp"
#include "Memory.hpp"
#include "Tuning.hpp"
#include "History.hpp"

namespace search { class NodeInfo; }

namespace mvorder {

/*
*  Tunable parameters in move ordering.
*/

_DEFINE_TUNABLE_PARAMETER(MvOrQuietMoveScoreReductionRate, int32_t, 2420.91f, 2300.f, 2500.f, 0.2f);
_DEFINE_TUNABLE_PARAMETER(MvOrCaptureMoveScoreReductionRate, int32_t, 11.4842f, 7.f, 15.f, 0.6f);
_DEFINE_TUNABLE_PARAMETER(MvOrQuietDepthShiftMult, int32_t, 257.727f, 200.f, 300.f, 0.2f);
_DEFINE_TUNABLE_PARAMETER(MvOrKnightCapturedScore, int32_t, 277.59f, 260.f, 350.f, 1.3f);
_DEFINE_TUNABLE_PARAMETER(MvOrBishopCapturedScore, int32_t, 321.216f, 260.f, 350.f, 1.3f);
_DEFINE_TUNABLE_PARAMETER(MvOrToKnightPromoScore, int32_t, 85.5026f, 50.f, 200.f, 1.3f);
_DEFINE_TUNABLE_PARAMETER(MvOrToBishopPromoScore, int32_t, 94.8676f, 50.f, 300.f, 1.3f);
_DEFINE_TUNABLE_PARAMETER(MvOrToRookPromoScore, int32_t, 267.031f, 150.f, 500.f, 1.3f);
_DEFINE_TUNABLE_PARAMETER(MvOrToQueenPromoScore, int32_t, 941.178f, 700.f, 1020.f, 1.3f);
_DEFINE_TUNABLE_PARAMETER(MvOrPawnCapturedScore, int32_t, 100.f, 80.f, 120.f, 0.8f);
_DEFINE_TUNABLE_PARAMETER(MvOrRookCapturedScore, int32_t, 500.f, 450.f, 550.f, 0.8f);
_DEFINE_TUNABLE_PARAMETER(MvOrQueenCapturedScore, int32_t, 900.f, 820.f, 980.f, 0.8f);
_DEFINE_TUNABLE_PARAMETER(MvOrQuietBonusHistoryScore2Coeff, int32_t, 1024.f, 724.f, 1324.f, 1.f);
_DEFINE_TUNABLE_PARAMETER(MvOrQuietBonusHistoryScore1Coeff, int32_t, 1.f, 0.f, 300.f, 1.f);
_DEFINE_TUNABLE_PARAMETER(MvOrQuietPenaltyHistoryScore2Coeff, int32_t, 1024.f, 724.f, 1324.f, 1.f);
_DEFINE_TUNABLE_PARAMETER(MvOrQuietPenaltyHistoryScore1Coeff, int32_t, 1.f, 0.f, 300.f, 1.f);
_DEFINE_TUNABLE_PARAMETER(MvOrContBonusHistoryScore2Coeff, int32_t, 1024.f, 724.f, 1324.f, 1.f);
_DEFINE_TUNABLE_PARAMETER(MvOrContBonusHistoryScore1Coeff, int32_t, 16.f, 0.f, 300.f, 1.f);
_DEFINE_TUNABLE_PARAMETER(MvOrContPenaltyHistoryScore2Coeff, int32_t, 1024.f, 724.f, 1324.f, 1.f);
_DEFINE_TUNABLE_PARAMETER(MvOrContPenaltyHistoryScore1Coeff, int32_t, 1.f, 0.f, 300.f, 1.f);
_DEFINE_TUNABLE_PARAMETER(MvOrdContinuation1Scale, int32_t, 1024.f, 824.f, 1224.f, 1.f);
_DEFINE_TUNABLE_PARAMETER(MvOrdContinuation2Scale, int32_t, 800.f, 600.f, 1000.f, 1.f);

static inline constexpr int16_t ContinuationPlyCount = 2;

enum enumOrderPolicy : uint8_t {
    STAGED         = 1, // At nmSearch nodes
    QUIESCENT      = 2, // At qSearch nodes
    ONCE_GEN_LEGAL = 3  // At root node
};

enum class enumStage {
    STAGE_PRIVATE,
    STAGE_PRIORITY_MOVES,
    STAGE_CAPTURES,
    STAGE_QUIETS
};

class HistoryTablesCluster {
public:
    HistoryTablesCluster() = default;

    void clear();
    void onSearch();

    _NODISCARD constexpr int32_t getMaxTotalAbsValue();

    _NODISCARD mvhist::HistoryTable& getHistoryTable() noexcept;
    _NODISCARD mvhist::ContinuationTable& getContinuationTable() noexcept;
private:
    enum enumHistIndex {
        HISTORY_INDEX = 0, CONTINUATION_INDEX = 1
    };

    std::tuple<mvhist::HistoryTable, mvhist::ContinuationTable> _history_cluster;
};

/* Here we generate and sort moves.
*  It wrapps `MoveList` class with actual moves and scores.
*/
class MoveOrder {
public:
    MoveOrder() = default;

    static void setHistoryBuffer(mem::AlignedSharedPtr<HistoryTablesCluster> history_tables);

    /*
    *   nextMoveWithPolicy<STAGED>:
    *    - Generates moves by moving through generation stages (first <TACTICALS_ONLY_QUEENPROMOS>, then <QUIETS_ONLY_UNDERPROMOS>)
    *   nextMoveWithPolicy<QUIESCE>:
    *    - Generates only captures in quiescent node.
    *   nextMoveWithPolicy<ONCE_GEN_LEGAL>:
    *    - Generates all of the legal moves once.
    */

    template <enumOrderPolicy Policy, bool Root>
    _NODISCARD bool nextMoveWithPolicy(search::NodeInfo* node, 
                                       Position& pos, 
                                       Move32b& next_move,
                                       ml::MoveScore& move_score,
                                       int ply);

    void clear();
    void setHashMove(Move32b m);
    void setKillerMove(Move32b m, uint64_t parent_hash);

    Move32b getKillerMove(uint64_t& killer_move_parent_hash);

    void updateQuietsHistory(Move32b bestmove, 
                             enumColor side, 
                             int depth, 
                             int ply,
                             const search::NodeInfo* node);

    void skipQuiets();

    _NODISCARD static int32_t getQuietDepthReduction(ml::MoveScore quiet_score);
    _NODISCARD static float getCaptureDepthReduction(ml::MoveScore capture_score);

    template <enumOrderPolicy Policy, typename = std::enable_if_t<Policy == ONCE_GEN_LEGAL>>
    _NODISCARD uint getMovesLeft();

    template <enumOrderPolicy Policy, typename = std::enable_if_t<Policy == ONCE_GEN_LEGAL>>
    _NODISCARD uint getTotalMoves();

    _NODISCARD enumStage getStage() const;
private:
    template <int8_t Sign>
    void updateQuietEntry(Move32b move, 
                          enumColor side, 
                          int16_t hist_bonus, 
                          int16_t cont_hist,
                          const search::NodeInfo* node,
                          int ply);

    bool nextMoveFromList(Move32b& move, ml::MoveScore& score, std::size_t end_idx);
    bool getNextMoveInfo(Move32b& move, ml::MoveScore& score, std::size_t end_idx = maxof<std::size_t>());

    void scoreCaptures(std::size_t first_ind, const Position& pos);
    void scoreQuiets(std::size_t first_ind, 
                     enumColor side, 
                     const search::NodeInfo* node, 
                     int ply);

    bool nextMoveFromOnceGen(Position& pos, 
                             Move32b& next_move,
                             ml::MoveScore& move_score,
                             const search::NodeInfo* node,
                             int ply);

    _NODISCARD static ml::MoveScore outputMoveScore(Move32b move, ml::MoveScore s);

    enum class enumPrivateStage : uint8_t {
        NONE,
        FIRST_STAGE,
        ONCEGEN_HASH_MOVE,
        ONCEGEN_ALL,
        ONCEGEN_PICK_CAPTURES,
        ONCEGEN_PICK_QUIETS,
        STAGED_HASH_MOVE,
        STAGED_CAPTURES,
        STAGED_PICK_CAPTURES, 
        STAGED_KILLER,
        STAGED_QUIETS,
        STAGED_PICK_QUIETS,
    };

    static mem::AlignedSharedPtr<HistoryTablesCluster> 
                     _history_cluster;
    enumPrivateStage _stage = enumPrivateStage::NONE;
    std::size_t      _iterator     = 0;
    std::size_t      _quiets_ind   = 0;
    Move32b          _hash_move    = NullMove;
    Move32b          _killer_move  = NullMove;
    uint64_t         _killer_move_parent_hash = 0;
    ml::MoveList     _move_list;
};

_INLINE void MoveOrder::setHistoryBuffer(mem::AlignedSharedPtr<HistoryTablesCluster> history_tables) {
    _history_cluster = history_tables;
}

_INLINE void MoveOrder::setHashMove(Move32b m) {
    _hash_move = m;
}

_INLINE void MoveOrder::setKillerMove(Move32b m, uint64_t parent_hash) {
    _killer_move = m;
    _killer_move_parent_hash = parent_hash;
}

_INLINE Move32b MoveOrder::getKillerMove(uint64_t& killer_move_parent_hash) {
    killer_move_parent_hash = _killer_move_parent_hash;
    return _killer_move;
}

_INLINE void MoveOrder::clear() {
    _stage = enumPrivateStage::FIRST_STAGE;
    _iterator = 0;
    _quiets_ind = 0;
    _hash_move = NullMove;
    _killer_move = NullMove;
    _move_list.clear();
}

_FORCEINLINE void MoveOrder::skipQuiets() {
    _iterator = _move_list.count();
}

/* Search utilities - move reductions
*  =================================
*/

_NODISCARD _FORCEINLINE int32_t MoveOrder::getQuietDepthReduction(ml::MoveScore quiet_score) {
    const int32_t centered_score = quiet_score.value() - MvOrQuietDepthShiftMult * ml::MoveScore::MaxQuietValue / 512;
    const float rt = std::sqrt(static_cast<float>(std::abs(centered_score)));
    const int32_t val = MvOrQuietMoveScoreReductionRate * rt / 128;
    return centered_score < 0 ? val : -val;
}

_NODISCARD _FORCEINLINE float MoveOrder::getCaptureDepthReduction(ml::MoveScore capture_score) {
    // TODO: better fixed-point formula
    return static_cast<float>(MvOrCaptureMoveScoreReductionRate * capture_score.value() / 128);
}

// =================================

template <enumOrderPolicy Policy, typename /* = std::enable_if_t<Type == ONCE_GEN_LEGAL> */>
uint MoveOrder::getMovesLeft() {
    return _move_list.count() - _iterator;
}

template <enumOrderPolicy Type, typename /* = std::enable_if_t<Type == ONCE_GEN_LEGAL> */>
uint MoveOrder::getTotalMoves() {
    return static_cast<uint>(_move_list.count());
}

} // namespace mvorder
