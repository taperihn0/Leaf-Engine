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

mem::AlignedSharedPtr<MoveOrder::HistoryTables> MoveOrder::_hist_tables;

MoveOrder::HistoryTables::HistoryTables() { clearQuietsHistory(); }

void MoveOrder::HistoryTables::clearQuietsHistory() {
    mem::memSet(dataOfArray3d(_quiets_history), 0, sizeof(_quiets_history));
}

/* 
*   MoveOrder<STAGED> and MoveOrder<QUIESCENT> template classes do not specify generateMoves function. 
*   Both generates appropiate moves on fly, during move picking as stage is 
*   moving from really promising moves to less interesting ones.
*/

template <enumOrderPolicy Policy, bool Root>
bool MoveOrder::nextMoveWithPolicy(const search::NodeInfo* node,
                                   Position& pos, 
                                   Move32b& next_move,
                                   ml::MoveScore& move_score) 
{
    static_assert(!Root or Policy == ONCE_GEN_LEGAL);
    static_assert(Root or Policy != ONCE_GEN_LEGAL);

    assert(_hist_tables != nullptr);

    if constexpr (Root) {
        _killer_move = NullMove;
    }

    next_move = NullMove;
    move_score = sc::Undef;

    if constexpr (Policy == ONCE_GEN_LEGAL) {
        return nextMoveFromOnceGen(pos, next_move, move_score);
    }

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
        if (nextFromList(next_move, move_score))
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
                const search::NodeInfo* const parent_node = node - 1;
                parent_hash = parent_node->state.hash_key;
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
        scoreQuiets(0, pos.getTurn());

        _stage = enumPrivateStage::STAGED_PICK_QUIETS;
        [[fallthrough]];
    case enumPrivateStage::STAGED_PICK_QUIETS:
        assert(Policy != QUIESCENT);
        return nextFromList(next_move, move_score);
    default:
        assert(false);
        break;
    }

    return false;
}

template <int8_t Sign>
void MoveOrder::updateQuietEntry(Move32b move, enumColor side, int depth) {
    static_assert(Sign == -1 or Sign == 1);
    assert(_hist_tables != nullptr);

    const Piece::uint_t piece = index(move.getPiece());
    const Square dst = move.getTarget();

    const int32_t bonus = std::min(sq(depth), static_cast<int>(HistoryTables::_MaxAbsQuietsHistory));
    const int32_t quiet_value = static_cast<int32_t>(_hist_tables->_quiets_history[side][piece][dst]);

    _hist_tables->_quiets_history[side][piece][dst] += static_cast<int16_t>(
        Sign * bonus - quiet_value * bonus / HistoryTables::_MaxAbsQuietsHistory
    );

    assert(abs(quiet_value) <= HistoryTables::_MaxAbsQuietsHistory);
}

void MoveOrder::updateQuietsHistory(Move32b bestmove, enumColor side, int depth) {
    assert(bestmove.isQuiet() and !bestmove.isQueenPromotion());

    updateQuietEntry<1>(bestmove, side, depth);

    for (size_t i = _quiets_ind; i < _move_list.count(); i++) {
        ml::MoveList::Entry& entry = _move_list.getEntry(i);
        const Move32b& move = entry.move;

        assert(move.isQuiet() and !move.isQueenPromotion());

        if (move == bestmove)
            break;

        updateQuietEntry<-1>(move, side, depth);
    }
}

/* Search for another move in a `_move_list` starting from current `_iterator`
*  up to the possible `end_idx` position.
*/
_INLINE bool MoveOrder::nextFromList(Move32b& move, ml::MoveScore& score, size_t end_idx) {
    assert(_iterator <= end_idx);

    while (_iterator < _move_list.count() and _iterator < end_idx) {
        _move_list.selectSort(_iterator);

        move = _move_list.getMove(_iterator);
        score = _move_list.getScore(_iterator++);

        if (move != _hash_move and move != _killer_move)
            return true;
    }

    return false;
}

static Array1d<const int16_t*, 5> CaptureScore = {
    reinterpret_cast<const int16_t*>(&PawnCapturedScore), 
    reinterpret_cast<const int16_t*>(&KnightCapturedScore), 
    reinterpret_cast<const int16_t*>(&BishopCapturedScore), 
    reinterpret_cast<const int16_t*>(&RookCapturedScore), 
    reinterpret_cast<const int16_t*>(&QueenCapturedScore), 
};

static Array1d<const int16_t*, 5> PromotionScore = {
    nullptr,                        // pawn placeholder 
    reinterpret_cast<const int16_t*>(&ToKnightPromoScore), 
    reinterpret_cast<const int16_t*>(&ToBishopPromoScore), 
    reinterpret_cast<const int16_t*>(&ToRookPromoScore),
    reinterpret_cast<const int16_t*>(&ToQueenPromoScore)
};

void MoveOrder::scoreCaptures(size_t first_ind, const Position& pos) {
    for (size_t i = first_ind; i < _move_list.count(); i++) {
        ml::MoveList::Entry& entry = _move_list.getEntry(i);
        const Move32b& move = entry.move;
        ml::MoveScore& score = entry.score;

        assert(move->isCapture() or 
               (move->isPromotion() and 
                move->isQueenPromotion() and 
               !move->isLegalMoved())); // legality not checked yet

        score = 0;

        if (move.isEnPassant()) {
            score = *CaptureScore[Piece::PAWN] - index(Piece::PAWN);
        }
        else if (move.isCapture()) {
            const Piece::uint_t piece_ind = index(move.getPiece());
            const Piece::uint_t vic = move.getCaptured(pos);
            score = *CaptureScore[vic] - piece_ind;
        }
        
        /* We treat promotions as 'captures' here, since it 
        *  is obviously a tactical move.
        */
        if (move.isPromotion()) {
            const Piece::uint_t promo = index(move.getPromoPiece());
            score += *PromotionScore[promo];
        }
    }
}

void MoveOrder::scoreQuiets(size_t first_ind, enumColor side) {
    assert(_hist_tables != nullptr);

    for (size_t i = first_ind; i < _move_list.count(); i++) {
        ml::MoveList::Entry& entry = _move_list.getEntry(i);
        const Move32b& move = entry.move;
        ml::MoveScore& score = entry.score;

        assert(move.isQuiet());

        /* history value is in range [-HistoryTables::_MaxAbsQuietsHistory, +HistoryTables::_MaxAbsQuietsHistory],
        *  we shift so that we got non-negative actual score.
        */
        score = _hist_tables->getNormalizedQuietScore(move, side);
    }
}

bool MoveOrder::nextMoveFromOnceGen(Position& pos, 
                                    Move32b& next_move,
                                    ml::MoveScore& move_score)
{
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

        scoreQuiets(_quiets_ind, pos.getTurn());

        if (!next_move.isNullMove()) // got hash move assigned already
            return true;

        [[fallthrough]];
    case enumPrivateStage::ONCEGEN_PICK_CAPTURES:
        // Search for another capture only, stop at quiets
        if (nextFromList(next_move, move_score, _quiets_ind))
            return true;

        _stage = enumPrivateStage::ONCEGEN_PICK_QUIETS;
        [[fallthrough]];
    case enumPrivateStage::ONCEGEN_PICK_QUIETS:
        return nextFromList(next_move, move_score);
    default:
        assert(false);
        break;
    }

    return false;
}

template bool MoveOrder::nextMoveWithPolicy<STAGED, false>(const search::NodeInfo*, Position&, Move32b&, ml::MoveScore&);
template bool MoveOrder::nextMoveWithPolicy<QUIESCENT, false>(const search::NodeInfo*, Position&, Move32b&, ml::MoveScore&);
template bool MoveOrder::nextMoveWithPolicy<ONCE_GEN_LEGAL, true>(const search::NodeInfo*, Position&, Move32b&, ml::MoveScore&);

} // namespace mvorder
