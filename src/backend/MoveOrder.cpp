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

#include "MoveOrder.hpp"
#include "Position.hpp"
#include "Search.hpp"

namespace mvorder {

mem::AlignedSharedPtr<HistoryTablesCluster> MoveOrder::_history_cluster;

void HistoryTablesCluster::clear() {
    quiet_history.clear();
    continuation_history.clear();
}

void HistoryTablesCluster::onSearch() {
    quiet_history.reduce();
    continuation_history.reduce();
}

_NODISCARD ml::MoveScore HistoryTablesCluster::centeredQuietScore(ml::MoveScore s) {
    // TODO
    return s.value() - 8192;
}

/* 
*   MoveOrder<STAGED> and MoveOrder<QUIESCENT> template classes do not specify generateMoves function. 
*   Both generates appropiate moves on fly, during move picking as stage is 
*   moving from really promising moves to less interesting ones.
*/

template <enumOrderPolicy Policy, bool Root>
bool MoveOrder::nextMoveWithPolicy(search::NodeInfo* node,
                                   Position& pos, 
                                   Move32b& next_move,
                                   ml::MoveScore& move_score,
                                   int ply) 
{
    static_assert(!Root or Policy == ONCE_GEN_LEGAL);
    static_assert(Root or Policy != ONCE_GEN_LEGAL);

    assert(_history_cluster != nullptr);

    if constexpr (Root) {
        _killer_move = NullMove;
    }

    next_move = NullMove;
    move_score = sc::Undef;

    if constexpr (Policy == ONCE_GEN_LEGAL) {
        return nextMoveFromOnceGen(pos, next_move, move_score, node, ply);
    }

    const enumColor s2m = pos.getTurn();

    switch (_stage) {
    case enumPrivateStage::FIRST_STAGE:
        _stage = enumPrivateStage::STAGED_HASH_MOVE;
        [[fallthrough]];
    case enumPrivateStage::STAGED_HASH_MOVE:
        _stage = enumPrivateStage::STAGED_CAPTURES;

        if (!_hash_move.isNullMove()) {
            next_move = _hash_move;
            return true;
        }

        [[fallthrough]];
    case enumPrivateStage::STAGED_CAPTURES:
        MoveGen::generatePseudoLegalMoves<MoveGen::CAPTURES>(pos, _move_list);
        scoreCaptures(0, pos);

        _stage = enumPrivateStage::STAGED_PICK_CAPTURES;
        [[fallthrough]];
    case enumPrivateStage::STAGED_PICK_CAPTURES:
        if (getNextMoveInfo(next_move, move_score, s2m))
            return true;
        
        if constexpr (Policy == QUIESCENT)
            return false;

        _stage = enumPrivateStage::STAGED_KILLER;
        [[fallthrough]];
    case enumPrivateStage::STAGED_KILLER:
        assert(Policy != QUIESCENT);

        _stage = enumPrivateStage::STAGED_QUIETS;
        _quiets_ind = _iterator;

        {
            uint64_t parent_hash = ZHash::Undef;

            if constexpr (!Root) {
                const search::NodeInfo* const prev_node = node - 1;
                parent_hash = prev_node->state.hash_key;
            }

            if (!_killer_move.isNullMove() and
                _killer_move != _hash_move and
                !Root and
                parent_hash == _killer_move_parent_hash and
                _killer_move.isPseudoLegal(pos))
            {
                next_move = _killer_move;
                return true;
            }
        }

        [[fallthrough]];
    case enumPrivateStage::STAGED_QUIETS:
        assert(Policy != QUIESCENT);
        MoveGen::generatePseudoLegalMoves<MoveGen::QUIETS>(pos, _move_list);

        _stage = enumPrivateStage::STAGED_PICK_QUIETS;
        [[fallthrough]];
    case enumPrivateStage::STAGED_PICK_QUIETS:
        assert(Policy != QUIESCENT);
        scoreQuiets(_iterator, pos.getTurn(), node, ply);
        return getNextMoveInfo(next_move, move_score, s2m);
    default:
        assert(false);
        break;
    }

    return false;
}

template <int8_t Sign>
void MoveOrder::updateQuietEntry(Move32b move, 
                                 enumColor side, 
                                 int16_t hist_bonus, 
                                 int16_t cont_bonus,
                                 const search::NodeInfo* node,
                                 int ply) 
{
    static_assert(Sign == -1 or Sign == 1);
    assert(_history_cluster != nullptr);

    const int32_t mhist_bonus = std::min(hist_bonus, mvhist::HistoryTable::Entry::MaxAbsBound);
    _history_cluster->quiet_history.update<Sign>(side, move, mhist_bonus);

    for (int i = 0; i < ContinuationPlyCount and i < ply; i++) {
        const search::NodeInfo* prev_node = node - i - 1;

        auto& continuation_subtable = (*prev_node->continuation_subtable_ptr);

        const int32_t mcont_bonus = std::min(cont_bonus, mvhist::ContinuationSubtable::Entry::MaxAbsBound);
        continuation_subtable.update<Sign>(side, move, mcont_bonus);
    }
}

void MoveOrder::updateQuietsHistory(Move32b bestmove, 
                                    enumColor side, 
                                    int depth, 
                                    int ply,
                                    const search::NodeInfo* node) 
{
    assert(bestmove.isQuiet() and !bestmove.isQueenPromotion());

    const int16_t hist_bonus = (MvOrQuietBonusHistoryScore2Coeff * sq(depth) + 
                                 MvOrQuietBonusHistoryScore1Coeff * depth
                                ) / 1024;

    const int16_t cont_bonus = (MvOrContBonusHistoryScore2Coeff * sq(depth) + 
                                 MvOrContBonusHistoryScore1Coeff * depth
                                ) / 1024;

    updateQuietEntry<+1>(bestmove, side, hist_bonus, cont_bonus, node, ply);

    const int16_t hist_penalty = (MvOrQuietPenaltyHistoryScore2Coeff * sq(depth) + 
                                   MvOrQuietPenaltyHistoryScore1Coeff * depth
                                  ) / 1024;

    const int16_t cont_penalty = (MvOrContPenaltyHistoryScore2Coeff * sq(depth) + 
                                  MvOrContPenaltyHistoryScore1Coeff * depth
                                 ) / 1024;

    for (std::size_t i = _quiets_ind; i < _move_list.count(); i++) {
        ml::MoveList::Entry& entry = _move_list.getEntry(i);
        const Move32b move = entry.move();

        assert(move.isQuiet() and !move.isQueenPromotion());

        if (move == bestmove)
            return;

        updateQuietEntry<-1>(move, side, hist_penalty, cont_penalty, node, ply);
    }
}

/* Search for another move in a `_move_list` starting from current `_iterator`
*  up to the possible `end_idx` position.
*/
_INLINE bool MoveOrder::nextMoveFromList(Move32b& move, ml::MoveScore& score, std::size_t end_idx) {
    assert(_iterator <= end_idx);

    while (_iterator < _move_list.count() and _iterator < end_idx) {
        _move_list.selectBest(_iterator, end_idx);

        const ml::MoveList::Entry entry = _move_list.getEntry(_iterator++);

        move = entry.move();
        score = entry.score();

        if (move != _hash_move and move != _killer_move)
            return true;
    }

    return false;
}

_INLINE bool MoveOrder::getNextMoveInfo(Move32b& move, ml::MoveScore& score, enumColor side, std::size_t end_idx) {
    const bool found = nextMoveFromList(move, score, end_idx);
    score = outputMoveScore(move, side, score);
    return found;
}

void MoveOrder::scoreCaptures(std::size_t first_ind, const Position& pos) {

    _LC_PARAM_ATTRIBS MultiArray<const int16_t, 5> CaptureScore = {
        static_cast<int16_t>(MvOrPawnCapturedScore), 
        static_cast<int16_t>(MvOrKnightCapturedScore), 
        static_cast<int16_t>(MvOrBishopCapturedScore), 
        static_cast<int16_t>(MvOrRookCapturedScore), 
        static_cast<int16_t>(MvOrQueenCapturedScore), 
    };

    _LC_PARAM_ATTRIBS MultiArray<const int16_t, 5> PromotionScore = {
        0, // pawn placeholder 
        static_cast<int16_t>(MvOrToKnightPromoScore), 
        static_cast<int16_t>(MvOrToBishopPromoScore), 
        static_cast<int16_t>(MvOrToRookPromoScore),
        static_cast<int16_t>(MvOrToQueenPromoScore)
    };

    for (std::size_t i = first_ind; i < _move_list.count(); i++) {
        ml::MoveList::Entry& entry = _move_list.getEntry(i);
        const Move32b move = entry.move();
        ml::MoveScore score = entry.score();

        assert(move.isCapture() or 
               (move.isPromotion() and 
                move.isQueenPromotion() and 
               !move.isLegalAfterMove())); // legality not checked yet

        score = 0;

        if (move.isEnPassant()) {
            score = CaptureScore[Piece::PAWN] - index(Piece::PAWN);
        }
        else if (move.isCapture()) {
            const Piece::uint_t piece_ind = index(move.getPiece());
            const Piece::uint_t vic = move.getCaptured(pos);
            score = CaptureScore[vic] - piece_ind;
        }
        
        /* We treat promotions as 'captures' here, since it 
        *  is obviously a tactical move.
        */
        if (move.isPromotion()) {
            const Piece::uint_t promo = index(move.getPromoPiece());
            score += PromotionScore[promo];
        }

        entry.setScore(score);
    }
}

void MoveOrder::scoreQuiets(std::size_t first_ind, 
                            enumColor side, 
                            const search::NodeInfo* node, 
                            int ply) 
{
    assert(_history_cluster != nullptr);

    _LC_PARAM_ATTRIBS const MultiArray<int32_t, ContinuationPlyCount> MvOrdContinuationPlyScale = {
        MvOrdContinuation1Scale,
        MvOrdContinuation2Scale,
    };

    for (std::size_t i = first_ind; i < _move_list.count(); i++) {
        ml::MoveList::Entry& entry = _move_list.getEntry(i);

        const Move32b move = entry.move();
        assert(move.isQuiet());

        ml::MoveScore score = _history_cluster->quiet_history.getValue(side, move);

        /* Apply continuation score */

        for (int j = 0; j < ContinuationPlyCount and j < ply; j++) {
            const search::NodeInfo* prev_node = node - j - 1;
            
            auto& continuation_subtable = (*prev_node->continuation_subtable_ptr);
            
            const int16_t cont_value = continuation_subtable.getValue(side, move);
            const int32_t scaled_cont_value = MvOrdContinuationPlyScale[j] * cont_value / 1024;

            score += scaled_cont_value;
        }

        entry.setScore(score);  
    }
}

bool MoveOrder::nextMoveFromOnceGen(Position& pos, 
                                    Move32b& next_move,
                                    ml::MoveScore& move_score,
                                    const search::NodeInfo* node,
                                    int ply)
{
    const enumColor s2m = pos.getTurn();

    switch (_stage) {
    case enumPrivateStage::FIRST_STAGE:
        _stage = enumPrivateStage::ONCEGEN_HASH_MOVE;
        [[fallthrough]];
    case enumPrivateStage::ONCEGEN_HASH_MOVE:
        _stage = enumPrivateStage::ONCEGEN_ALL;

        if (!_hash_move.isNullMove())
            next_move = _hash_move;
        
        [[fallthrough]];
    case enumPrivateStage::ONCEGEN_ALL:
        _stage = enumPrivateStage::ONCEGEN_PICK_CAPTURES;

        MoveGen::generateLegalMoves<MoveGen::CAPTURES>(pos, _move_list);
        scoreCaptures(0, pos);

        _quiets_ind = _move_list.count();
        MoveGen::generateLegalMoves<MoveGen::QUIETS>(pos, _move_list);

        if (!next_move.isNullMove()) // got hash move assigned already
            return true;

        [[fallthrough]];
    case enumPrivateStage::ONCEGEN_PICK_CAPTURES:
        // Search for another capture only, stop at quiets
        if (getNextMoveInfo(next_move, move_score, s2m, _quiets_ind))
            return true;

        _stage = enumPrivateStage::ONCEGEN_PICK_QUIETS;
        [[fallthrough]];
    case enumPrivateStage::ONCEGEN_PICK_QUIETS:
        scoreQuiets(_iterator, s2m, node, ply);
        return getNextMoveInfo(next_move, move_score, s2m);
    default:
        assert(false);
        break;
    }

    return false;
}

_NODISCARD _FORCEINLINE ml::MoveScore MoveOrder::outputMoveScore(Move32b move, enumColor side, ml::MoveScore s) noexcept {
    //return move.isCapture() or move.isPromotion() ? s : _history_cluster->getQuietMoveScore(side, move); // TODO
    //return s;
    return move.isCapture() or move.isPromotion() ? s : 8192 + 8192 * s.value() /
            (mvhist::HistoryTable::Entry::MaxAbsBound + (MvOrdContinuation1Scale + MvOrdContinuation2Scale) * mvhist::ContinuationSubtable::Entry::MaxAbsBound / 1024);
}

_NODISCARD enumStage MoveOrder::getStage() const {
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

template bool MoveOrder::nextMoveWithPolicy<STAGED, false>(search::NodeInfo*, Position&, Move32b&, ml::MoveScore&, int);
template bool MoveOrder::nextMoveWithPolicy<QUIESCENT, false>(search::NodeInfo*, Position&, Move32b&, ml::MoveScore&, int);
template bool MoveOrder::nextMoveWithPolicy<ONCE_GEN_LEGAL, true>(search::NodeInfo*, Position&, Move32b&, ml::MoveScore&, int);

} // namespace mvorder
