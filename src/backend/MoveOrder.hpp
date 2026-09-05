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

namespace search::utils { class NodeInfo; }

namespace mvo {

class SMoveScore final : public ml::MoveScore {
public:
    using Base = ml::MoveScore;
    using Base::operator=;

    static constexpr value_type MinQuietValue = 0;
    static constexpr value_type MaxQuietValue = 16384;
    static constexpr value_type HalfMaxQuietValue = MaxQuietValue / 2;

    SMoveScore() = default;
    _INLINE constexpr explicit SMoveScore(const Base& s) noexcept : Base(s.value()) {}
    _INLINE constexpr SMoveScore(const SMoveScore&) = default;
    _INLINE constexpr SMoveScore(const sc::Score& s) noexcept : Base(s.value()) {}
    _INLINE constexpr SMoveScore(int32_t val) noexcept : Base(val) {}

    _NODISCARD _FORCEINLINE SMoveScore quietCentered() const {
        assert(_v >= MinQuietValue and _v <= MaxQuietValue);
        return value() - HalfMaxQuietValue;
    }

    _NODISCARD _FORCEINLINE SMoveScore quietTranslToPositive() const {
        assert(_v >= -HalfMaxQuietValue and _v <= HalfMaxQuietValue);
        return value() + HalfMaxQuietValue;
    }

    _NODISCARD _FORCEINLINE constexpr SMoveScore quietOntoOutputRange(value_type curr_max_abs) const {
        return HalfMaxQuietValue + HalfMaxQuietValue * value() / curr_max_abs;
    }

    _FORCEINLINE constexpr SMoveScore& operator=(const sc::Score& s) noexcept {
        _v = static_cast<const sc::Score::Base&>(s)._v;
        return *this;
    }
private:
    using Base::_v;
};

/*
*  Tunable parameters in move ordering.
*/

_DEFINE_TUNABLE_PARAMETER(MvOrToKnightPromoScore, int32_t, 85.5026f, 50.f, 200.f, 1.3f);
_DEFINE_TUNABLE_PARAMETER(MvOrToBishopPromoScore, int32_t, 94.8676f, 50.f, 300.f, 1.3f);
_DEFINE_TUNABLE_PARAMETER(MvOrToRookPromoScore, int32_t, 267.031f, 150.f, 500.f, 1.3f);
_DEFINE_TUNABLE_PARAMETER(MvOrToQueenPromoScore, int32_t, 941.178f, 700.f, 1020.f, 1.3f);
_DEFINE_TUNABLE_PARAMETER(MvOrPawnCapturedScore, int32_t, 100.f, 80.f, 120.f, 0.8f);
_DEFINE_TUNABLE_PARAMETER(MvOrKnightCapturedScore, int32_t, 277.59f, 260.f, 350.f, 1.3f);
_DEFINE_TUNABLE_PARAMETER(MvOrBishopCapturedScore, int32_t, 321.216f, 260.f, 350.f, 1.3f);
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

class HistoryTablesCluster {
public:
    static inline constexpr int16_t ContinuationPlyCount = 2;

    HistoryTablesCluster() = default;

    void clear();
    void onSearch();

    _NODISCARD constexpr int32_t getMaxTotalAbsValue();

    _NODISCARD hist::HistoryTable& getHistoryTable() noexcept;
    _NODISCARD hist::ContinuationTable& getContinuationTable() noexcept;
    _NODISCARD hist::CaptureHistory& getCapturesHistoryTable() noexcept;
private:
    enum enumHistIndex {
        QUIETS_HISTORY_INDEX = 0, 
        CONTINUATION_INDEX = 1,
        CAPTURES_HISTORY_INDEX = 2,
    };

    std::tuple<hist::HistoryTable, hist::ContinuationTable, hist::CaptureHistory> 
        _history_cluster;
};

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

/* Here we generate and sort moves.
*  It wrapps `MoveList` class with actual moves and scores.
*/
class MoveOrder {
public:
    MoveOrder() = default;

    static void setHistoryBuffer(mem::AlignedSharedPtr<HistoryTablesCluster> history_tables);

    /*
    *   nextMoveWithPolicy<STAGED>:
    *    - Generates moves by moving through generation stages 
    *      (first <TACTICALS_ONLY_QUEENPROMOS>, then <QUIETS_ONLY_UNDERPROMOS>)
    *   nextMoveWithPolicy<QUIESCENT>:
    *    - Generates only captures in quiescent node.
    *   nextMoveWithPolicy<ONCE_GEN_LEGAL>:
    *    - Generates all of the legal moves once.
    */

    template <enumOrderPolicy Policy, bool Root>
    _NODISCARD bool nextMoveWithPolicy(search::utils::NodeInfo* node, 
                                       Position& pos, 
                                       Move32b& next_move,
                                       SMoveScore& move_score,
                                       int ply);

    void clear();
    void setHashMove(Move32b m);
    void setKillerMove(Move32b m, uint64_t parent_hash);

    Move32b getKillerMove(uint64_t& killer_move_parent_hash);

    void updateHistories(Move32b bestmove, 
                         enumColor side, 
                         int depth, 
                         int ply,
                         const Position& pos,
                         const search::utils::NodeInfo* node);

    void skipQuiets();

    template <enumOrderPolicy Policy, typename = std::enable_if_t<Policy == ONCE_GEN_LEGAL>>
    _NODISCARD uint getMovesLeft();

    template <enumOrderPolicy Policy, typename = std::enable_if_t<Policy == ONCE_GEN_LEGAL>>
    _NODISCARD uint getTotalMoves();

    _NODISCARD enumStage getStage() const;
private:
    template <enumOrderPolicy Policy, bool Root>
    _NODISCARD bool internalNextMove(search::utils::NodeInfo* node, 
                                     Position& pos, 
                                     Move32b& next_move,
                                     SMoveScore& move_score,
                                     int ply);

    void updateContinuationPointers(search::utils::NodeInfo* node, int ply);

    _NODISCARD static std::tuple<hist::HistoryTable::value_type, hist::ContinuationTable::value_type> 
    getHistoriesBonuses(int depth);
    _NODISCARD static std::tuple<hist::HistoryTable::value_type, hist::ContinuationTable::value_type> 
    getHistoriesPenalties(int depth);
    _NODISCARD static hist::CaptureHistory::value_type getCaptureBonus(int depth);
    _NODISCARD static hist::CaptureHistory::value_type getCapturePenalty(int depth);

    template <int8_t Sign>
    void updateQuietEntry(Move32b move, 
                          enumColor side, 
                          int16_t hist_bonus, 
                          int16_t cont_hist,
                          const search::utils::NodeInfo* node,
                          int ply);

    bool nextMoveFromList(Move32b& move, 
                          SMoveScore& score, 
                          size_t end_idx);

    bool getNextMoveInfo(Move32b& move, 
                         SMoveScore& score, 
                         size_t end_idx = maxof<size_t>());

    void scoreTacticals(size_t beg_idx, 
                        const Position& pos);

    void scoreQuiets(size_t beg_idx, 
                     enumColor side, 
                     const search::utils::NodeInfo* node, 
                     int ply);

    _NODISCARD static SMoveScore getOutputMoveScore(Move32b move, SMoveScore s);

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
    enumPrivateStage _stage        = enumPrivateStage::NONE;
    size_t           _idx          = 0;
    size_t           _quiets_idx   = static_cast<size_t>(-1);
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
    _idx = 0;
    _quiets_idx = static_cast<size_t>(-1);
    _hash_move = NullMove;
    _killer_move = NullMove;
    _move_list.clear();
}

_FORCEINLINE void MoveOrder::skipQuiets() {
    _idx = _move_list.count();
}

template <enumOrderPolicy Policy, typename /* = std::enable_if_t<Type == ONCE_GEN_LEGAL> */>
_INLINE uint MoveOrder::getMovesLeft() {
    return _move_list.count() - _idx;
}

template <enumOrderPolicy Type, typename /* = std::enable_if_t<Type == ONCE_GEN_LEGAL> */>
_INLINE uint MoveOrder::getTotalMoves() {
    return static_cast<uint>(_move_list.count());
}

} // namespace mvo
