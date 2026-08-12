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

class TreeStack;
class MoveOrder;

/* It is basically a part of MoveOrder interface.
*  It contains tables used in move ordering with history data, for instance 
*  piece-square or from-to tables.
*  It implements differentiation of history data between each Search object,
*  as it is part of Search class.
*/

class MoveOrderHistoryTables {
public:
    friend class MoveOrder;
    MoveOrderHistoryTables() { clearQuietsHistory(); }

    _INLINE void clearQuietsHistory() {
        mem::memSet(dataOfArray3d(_quiets_history), 0, sizeof(_quiets_history));
    }
private:
    array3d<int16_t, 2, 6, 64> _quiets_history;
    // ...
};

enum OrderType : uint8_t {
    STAGED         = 1, // At nmSearch nodes
    QUIESCENT      = 2, // At qSearch nodes
    ONCE_GEN_LEGAL = 3  // At root node
};

/*
*  Tunable parameters in move ordering.
*/

_PARAM_ATTRIBS int QuietMoveScoreReductionRate = roundi<float>(97.7501f);
_PARAM_ATTRIBS int CaptureMoveScoreReductionDiv = roundi<float>(54.0528f);
_PARAM_ATTRIBS int KnightCapturedScore = roundi<float>(277.09f);
_PARAM_ATTRIBS int BishopCapturedScore = roundi<float>(329.986f);
_PARAM_ATTRIBS int ToKnightPromoScore = roundi<float>(83.7102f);
_PARAM_ATTRIBS int ToBishopPromoScore = roundi<float>(112.354f);
_PARAM_ATTRIBS int ToRookPromoScore = roundi<float>(233.922f);
_PARAM_ATTRIBS int ToQueenPromoScore = roundi<float>(944.733f);

/*  Static parameters in move ordering -
*   These are not tuned.
*/

inline constexpr int MaxQuietsHistoryExp2 = 13;
inline constexpr int MaxAbsQuietsHistory  = 1 << MaxQuietsHistoryExp2;
inline constexpr int PawnCapturedScore    = 100;
inline constexpr int RookCapturedScore    = 500;
inline constexpr int QueenCapturedScore   = 900;

/*
*   MoveOrder<STAGED>:
*    - Generates moves by moving through generation stages (first <CAPTURES>, then <QUIETS>)
*   MoveOrder<QUIESCE>:
*    - Generates only captures in quiescent node.
*/

class MoveOrder {
public:
    enum class enumStage {
        STAGE_UNKNOWN,
        STAGE_PRIORITY_MOVES,
        STAGE_CAPTURES,
        STAGE_QUIETS
    };

    explicit MoveOrder(MoveOrderHistoryTables* history_tables = nullptr);

    void setHistoryBuffer(MoveOrderHistoryTables* history_tables);

    template <OrderType Type, bool Root>
    _NODISCARD bool nextMove(const NodeInfo* node, 
                             Position& pos, 
                             Move32b& next_move,
                             int16_t& move_score);

    void setHashMove(Move32b m);
    void setKillerMove(Move32b m, uint64_t parent_hash);

    Move32b getKillerMove(uint64_t& killer_move_parent_hash);

    template <int8_t Sign>
    void updateQuietEntry(Move32b move, enumColor side, int depth);
    void updateQuietsHistory(Move32b bestmove, enumColor side, int depth);
    
    template <OrderType Type>
    void clear();

    void skipQuiets();

    // Returns history score of move in range [-MaxAbsQuietsHistory, +MaxAbsQuietsHistory]
    int16_t getQuietScore(Move32b move, enumColor side) const;
    // Same as `getQuietScore`, but returns score in range [0, +2MaxQuietsHistory]
    int16_t getPositiveNormQuietScore(Move32b move, enumColor side) const;

    static int32_t getQuietDepthReduction(int16_t quiet_score);
    static float getCaptureDepthReduction(int16_t capture_score);

    template <OrderType Type, typename = std::enable_if_t<Type == ONCE_GEN_LEGAL>>
    _NODISCARD uint getMovesLeft();

    template <OrderType Type, typename = std::enable_if_t<Type == ONCE_GEN_LEGAL>>
    _NODISCARD uint getTotalMoves();

    _NODISCARD enumStage getStage() const;
private:
    bool nextFromList(Move32b& move, int16_t& score, size_t end_idx = maxof<size_t>());

    void scoreCaptures(size_t first_ind, const Position& pos);
    void scoreQuiets(size_t first_ind, enumColor side);

    bool nextMoveFromOnceGen(Position& pos, 
                             Move32b& next_move,
                             int16_t& move_score);

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

    static_assert(is_same<MoveList::entryscore_t, int16_t> or
                  is_same<MoveList::entryscore_t, int32_t>);

    MoveOrderHistoryTables* _tables;

    enumPrivateStage _stage = enumPrivateStage::NONE;
    size_t    _iterator     = 0;
    size_t    _quiets_ind   = 0;

    Move32b  _hash_move     = Move32b::Null;
    Move32b  _killer_move   = Move32b::Null;
    uint64_t _killer_move_parent_hash = 0;

    MoveList _move_list;
};

_INLINE void MoveOrder::setHistoryBuffer(MoveOrderHistoryTables* history_tables) {
    _tables = history_tables;
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

template <OrderType Type>
_INLINE void MoveOrder::clear() {
    _stage = enumPrivateStage::FIRST_STAGE;
    _iterator = 0;
    _quiets_ind = 0;
    _hash_move = Move32b::Null;

    if constexpr (Type == QUIESCENT)
        _killer_move = Move32b::Null;

    _move_list.clear();
}

_FORCEINLINE void MoveOrder::skipQuiets() {
    _iterator = _move_list.count();
}

_FORCEINLINE int16_t MoveOrder::getQuietScore(Move32b move, enumColor side) const {
    const Piece::uint_t piece_ind = index(move.getPiece());
    const Square dst = move.getTarget();
    return _tables->_quiets_history[side][piece_ind][dst];
}

_FORCEINLINE int16_t MoveOrder::getPositiveNormQuietScore(Move32b move, enumColor side) const {
    return getQuietScore(move, side) + MaxAbsQuietsHistory;
}

_FORCEINLINE int32_t MoveOrder::getQuietDepthReduction(int16_t quiet_score) {
    const int32_t centered_score = quiet_score - 256 * MaxAbsQuietsHistory / 256;
    const float rt = std::sqrt(static_cast<float>(std::abs(centered_score)));
    const int32_t val = QuietMoveScoreReductionRate * rt / 16;
    return centered_score < 0 ? val : -val;
}

_FORCEINLINE float MoveOrder::getCaptureDepthReduction(int16_t capture_score) {
    return static_cast<float>(capture_score / CaptureMoveScoreReductionDiv);
}

template <OrderType Type, typename /* = std::enable_if_t<Type == ONCE_GEN_LEGAL> */>
uint MoveOrder::getMovesLeft() {
    return _move_list.count() - _iterator;
}

template <OrderType Type, typename /* = std::enable_if_t<Type == ONCE_GEN_LEGAL> */>
uint MoveOrder::getTotalMoves() {
    return static_cast<uint>(_move_list.count());
}

_NODISCARD _FORCEINLINE MoveOrder::enumStage MoveOrder::getStage() const {
    switch (_stage) {
    case enumPrivateStage::NONE:
    case enumPrivateStage::FIRST_STAGE:
    case enumPrivateStage::STAGED_CAPTURES:
    case enumPrivateStage::STAGED_QUIETS:
    case enumPrivateStage::ONCEGEN_ALL:
    return enumStage::STAGE_UNKNOWN;
    
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
        return enumStage::STAGE_UNKNOWN;
    }
}
