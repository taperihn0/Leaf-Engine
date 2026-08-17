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

namespace search { class NodeInfo; }
class TreeStack;
class MoveOrder;

namespace mvorder {

/*
*  Tunable parameters in move ordering.
*/

_DEFINE_TUNABLE_PARAMETER(QuietMoveScoreReductionRate, int32_t, 2420.91f, 2300.f, 2500.f, 0.2f);
_DEFINE_TUNABLE_PARAMETER(CaptureMoveScoreReductionRate, int32_t, 11.4842f, 7.f, 15.f, 0.6f);
_DEFINE_TUNABLE_PARAMETER(QuietDepthShiftMult, int32_t, 257.727f, 200.f, 300.f, 0.2f);
_DEFINE_TUNABLE_PARAMETER(KnightCapturedScore, int32_t, 277.59f, 260.f, 350.f, 1.3f);
_DEFINE_TUNABLE_PARAMETER(BishopCapturedScore, int32_t, 321.216f, 260.f, 350.f, 1.3f);
_DEFINE_TUNABLE_PARAMETER(ToKnightPromoScore, int32_t, 85.5026f, 50.f, 200.f, 1.3f);
_DEFINE_TUNABLE_PARAMETER(ToBishopPromoScore, int32_t, 94.8676f, 50.f, 300.f, 1.3f);
_DEFINE_TUNABLE_PARAMETER(ToRookPromoScore, int32_t, 267.031f, 150.f, 500.f, 1.3f);
_DEFINE_TUNABLE_PARAMETER(ToQueenPromoScore, int32_t, 941.178f, 700.f, 1020.f, 1.3f);

/*  Static parameters in move ordering -
*   These are not tuned.
*/

inline constexpr int32_t PawnCapturedScore    = 100;
inline constexpr int32_t RookCapturedScore    = 500;
inline constexpr int32_t QueenCapturedScore   = 900;

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

    /* It is basically a part of `MoveOrder` interface.
    *  It contains tables used in move ordering with history data, for instance 
    *  piece-square or from-to tables.
    *  It implements differentiation of history data between each Search object,
    *  as it is part of Search class.
    */
    class HistoryTables {
    public:
        friend class MoveOrder;
        
        HistoryTables();
        void clearQuietsHistory();
    private:
        _NODISCARD _INLINE int16_t getNormalizedQuietScore(Move32b move, enumColor side);

        static inline constexpr int16_t _MaxQuietsHistoryExp2 = 13;
        static inline constexpr int16_t _MaxAbsQuietsHistory  = 1 << _MaxQuietsHistoryExp2;
        array3d<int16_t, 2, 6, 64>      _quiets_history;
    };

    MoveOrder() = default;

    static void setHistoryBuffer(mem::AlignedSharedPtr<HistoryTables> history_tables);

    /*
    *   nextMoveWithPolicy<STAGED>:
    *    - Generates moves by moving through generation stages (first <CAPTURES>, then <QUIETS>)
    *   nextMoveWithPolicy<QUIESCE>:
    *    - Generates only captures in quiescent node.
    *   nextMoveWithPolicy<ONCE_GEN_LEGAL>:
    *    - Generates all of the legal moves once.
    */

    template <enumOrderPolicy Policy, bool Root>
    _NODISCARD bool nextMoveWithPolicy(const search::NodeInfo* node, 
                                       Position& pos, 
                                       Move32b& next_move,
                                       ml::MoveScore& move_score);

    void setHashMove(Move32b m);
    void setKillerMove(Move32b m, uint64_t parent_hash);

    Move32b getKillerMove(uint64_t& killer_move_parent_hash);

    template <int8_t Sign>
    void updateQuietEntry(Move32b move, enumColor side, int depth);
    void updateQuietsHistory(Move32b bestmove, enumColor side, int depth);
    
    void clear();
    void skipQuiets();

    // Returns score in range [0, +MaxQuietMoveScore]
    int16_t getQuietMoveScore(size_t move_idx, enumColor side) const;

    static int32_t getQuietDepthReduction(ml::MoveScore quiet_score);
    static float getCaptureDepthReduction(ml::MoveScore capture_score);

    template <enumOrderPolicy Policy, typename = std::enable_if_t<Policy == ONCE_GEN_LEGAL>>
    _NODISCARD uint getMovesLeft();

    template <enumOrderPolicy Policy, typename = std::enable_if_t<Policy == ONCE_GEN_LEGAL>>
    _NODISCARD uint getTotalMoves();

    _NODISCARD enumStage getStage() const;

    _NODISCARD static ml::MoveScore centeredQuietScore(ml::MoveScore s) noexcept;

    static constexpr int16_t MaxQuietMoveScore = 2 * HistoryTables::_MaxAbsQuietsHistory;
private:
    bool nextFromList(Move32b& move, ml::MoveScore& score, size_t end_idx = maxof<size_t>());

    void scoreCaptures(size_t first_ind, const Position& pos);
    void scoreQuiets(size_t first_ind, enumColor side);

    bool nextMoveFromOnceGen(Position& pos, 
                             Move32b& next_move,
                             ml::MoveScore& move_score);

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

    static_assert(is_same<ml::MoveScore::int_t, int16_t>);

    static mem::AlignedSharedPtr<HistoryTables> _hist_tables;

    enumPrivateStage _stage = enumPrivateStage::NONE;
    size_t    _iterator     = 0;
    size_t    _quiets_ind   = 0;

    Move32b  _hash_move     = Move32b::Null;
    Move32b  _killer_move   = Move32b::Null;
    uint64_t _killer_move_parent_hash = 0;

    ml::MoveList _move_list;
};

_INLINE void MoveOrder::setHistoryBuffer(mem::AlignedSharedPtr<HistoryTables> history_tables) {
    _hist_tables = history_tables;
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
    _hash_move = Move32b::Null;
    _killer_move = Move32b::Null;
    _move_list.clear();
}

_FORCEINLINE void MoveOrder::skipQuiets() {
    _iterator = _move_list.count();
}

_NODISCARD _FORCEINLINE int16_t MoveOrder::HistoryTables::getNormalizedQuietScore(Move32b move, enumColor side) {
    const Piece::uint_t piece_ind = index(move.getPiece());
    const Square dst = move.getTarget();
    return _hist_tables->_quiets_history[side][piece_ind][dst] + _MaxAbsQuietsHistory;
}

_FORCEINLINE int16_t MoveOrder::getQuietMoveScore(size_t move_idx, enumColor side) const {
    const Move32b move = _move_list.getMove(move_idx);
    return _hist_tables->getNormalizedQuietScore(move, side);
}

_FORCEINLINE int32_t MoveOrder::getQuietDepthReduction(ml::MoveScore quiet_score) {
    const int32_t centered_score = quiet_score.value() - QuietDepthShiftMult * HistoryTables::_MaxAbsQuietsHistory / 256;
    const float rt = std::sqrt(static_cast<float>(std::abs(centered_score)));
    const int32_t val = QuietMoveScoreReductionRate * rt / 128;
    return centered_score < 0 ? val : -val;
}

_FORCEINLINE float MoveOrder::getCaptureDepthReduction(ml::MoveScore capture_score) {
    // TODO: better fixed-point formula
    return static_cast<float>(CaptureMoveScoreReductionRate * capture_score.value() / 128);
}

template <enumOrderPolicy Policy, typename /* = std::enable_if_t<Type == ONCE_GEN_LEGAL> */>
uint MoveOrder::getMovesLeft() {
    return _move_list.count() - _iterator;
}

template <enumOrderPolicy Type, typename /* = std::enable_if_t<Type == ONCE_GEN_LEGAL> */>
uint MoveOrder::getTotalMoves() {
    return static_cast<uint>(_move_list.count());
}

_NODISCARD _FORCEINLINE enumStage MoveOrder::getStage() const {
    switch (_stage) {
    case enumPrivateStage::NONE:
    case enumPrivateStage::FIRST_STAGE:
    case enumPrivateStage::STAGED_CAPTURES:
    case enumPrivateStage::STAGED_QUIETS:
    case enumPrivateStage::ONCEGEN_ALL:
        return enumStage::STAGE_PRIVATE;
    
    case enumPrivateStage::ONCEGEN_HASH_MOVE:
    case enumPrivateStage::STAGED_HASH_MOVE:
    case enumPrivateStage::STAGED_KILLER:
        return enumStage::STAGE_PRIORITY_MOVES;    
        
    case enumPrivateStage::ONCEGEN_PICK_CAPTURES:
    case enumPrivateStage::STAGED_PICK_CAPTURES:
        return enumStage::STAGE_CAPTURES;

    case enumPrivateStage::ONCEGEN_PICK_QUIETS:
    case enumPrivateStage::STAGED_PICK_QUIETS:
        return enumStage::STAGE_QUIETS;

    default:
        return enumStage::STAGE_PRIVATE;
    }
}

_NODISCARD _FORCEINLINE ml::MoveScore MoveOrder::centeredQuietScore(ml::MoveScore s) noexcept {
    return ml::MoveScore(s.value() - HistoryTables::_MaxAbsQuietsHistory);
}

} // namespace mvorder
