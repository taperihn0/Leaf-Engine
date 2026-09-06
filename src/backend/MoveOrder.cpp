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

namespace mvo {

_NODISCARD _FORCEINLINE constexpr int16_t getCapturedScore(Piece::enumType pc) {
    switch (pc) {
    case Piece::PAWN:   return MvOrPawnCapturedScore;
    case Piece::KNIGHT: return MvOrKnightCapturedScore;
    case Piece::BISHOP: return MvOrBishopCapturedScore;
    case Piece::ROOK:   return MvOrRookCapturedScore;
    case Piece::QUEEN:  return MvOrQueenCapturedScore;
    default: unreachable();
    }
}

_NODISCARD _FORCEINLINE constexpr int16_t getPromotionScore(Piece::enumType promo) {
    switch (promo) {
    case Piece::KNIGHT: return MvOrToKnightPromoScore;
    case Piece::BISHOP: return MvOrToBishopPromoScore;
    case Piece::ROOK:   return MvOrToRookPromoScore;
    case Piece::QUEEN:  return MvOrToQueenPromoScore;
    default: unreachable();
    }
}

_NODISCARD _FORCEINLINE constexpr int32_t getContinuationPlyScale(int ago) {
    assert(ago < HistoryTablesCluster::ContinuationPlyCount);

    switch (ago) {
    case 0: return MvOrdContinuation1Scale;
    case 1: return MvOrdContinuation2Scale;
    }

    FAILED_NO_LOG();
    return 0;
}

template <typename T, typename = std::enable_if_t<std::is_base_of_v<hist::HistoryTableBase<T>, T>>>
constexpr int32_t getMaxAbsValueOfTableEntry(const T&) {
    if constexpr (std::is_same_v<T, hist::HistoryTable>)
        return T::getMaxAbsValueOfEntry();

    else if constexpr (std::is_same_v<T, hist::ContinuationTable>) {
        int32_t scale_accum = 0;

        for (int i = 0; i < HistoryTablesCluster::ContinuationPlyCount; i++) 
            scale_accum += getContinuationPlyScale(i);

        return scale_accum * T::getMaxAbsValueOfEntry() / 1024;
    }
    
    return 0;
}

mem::AlignedSharedPtr<HistoryTablesCluster> MoveOrder::_history_cluster;

_NODISCARD constexpr int32_t HistoryTablesCluster::getMaxTotalAbsValue() {
    return std::apply(
        [](const auto&... tables) {
            return (getMaxAbsValueOfTableEntry(tables) + ... + 0);
        },
        _history_cluster
    );
}

_NODISCARD hist::HistoryTable& HistoryTablesCluster::getHistoryTable() noexcept {
    return std::get<QUIETS_HISTORY_INDEX>(_history_cluster);
}

_NODISCARD hist::ContinuationTable& HistoryTablesCluster::getContinuationTable() noexcept {
    return std::get<CONTINUATION_INDEX>(_history_cluster);
}

_NODISCARD hist::CaptureHistory& HistoryTablesCluster::getCapturesHistoryTable() noexcept {
    return std::get<CAPTURES_HISTORY_INDEX>(_history_cluster);
}

void HistoryTablesCluster::clear() {
    std::apply(
        [](auto&... tables) {
            (tables.clear(), ...);
        }, 
        _history_cluster
    );
}

void HistoryTablesCluster::onSearch() {
    std::apply(
        [](auto&... tables) {
            (tables.reduce(), ...);
        }, 
        _history_cluster
    );
}

/* 
*   MoveOrder<STAGED> and MoveOrder<QUIESCENT> template classes do not specify generateMoves function. 
*   Both generates appropiate moves on fly, during move picking as stage is 
*   moving from really promising moves to less interesting ones.
*/

template <enumOrderPolicy Policy, bool Root>
bool MoveOrder::nextMoveWithPolicy(search::utils::NodeInfo* node,
                                   Position& pos, 
                                   Move32b& next_move,
                                   SMoveScore& move_score,
                                   int ply) 
{
    return internalNextMove<Policy, Root>(node, pos, next_move, move_score, ply);
}

template <enumOrderPolicy Policy, bool Root>
_NODISCARD bool MoveOrder::internalNextMove(search::utils::NodeInfo* node, 
                                            Position& pos, 
                                            Move32b& next_move,
                                            SMoveScore& move_score,
                                            int ply)
{
    static_assert(!Root);
    assert(_history_cluster != nullptr);

    next_move = NullMove;
    move_score = sc::Undef;

    switch (_stage) {
    case enumPrivateStage::FIRST_STAGE:
        if constexpr (Policy != QUIESCENT)
            updateContinuationPointers(node, ply);

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
        MoveGen::generatePseudoLegalMoves<MoveGen::TACTICALS_ONLY_QUEENPROMOS>(pos, _move_list);
        scoreTacticals(0, pos);

        _stage = enumPrivateStage::STAGED_PICK_CAPTURES;
        [[fallthrough]];
    case enumPrivateStage::STAGED_PICK_CAPTURES:
        if (getNextMoveInfo(next_move, move_score))
            return true;
        
        if constexpr (Policy == QUIESCENT)
            return false;

        _stage = enumPrivateStage::STAGED_KILLER;
        [[fallthrough]];
    case enumPrivateStage::STAGED_KILLER:
        assert(Policy != QUIESCENT);

        _stage = enumPrivateStage::STAGED_QUIETS;
        _quiets_idx = _idx;

        {
            uint64_t parent_hash = ZHash::Undef;

            if constexpr (!Root) {
                const search::utils::NodeInfo* const prev_node = node - 1;
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
        MoveGen::generatePseudoLegalMoves<MoveGen::QUIETS_ONLY_UNDERPROMOS>(pos, _move_list);

        _stage = enumPrivateStage::STAGED_PICK_QUIETS;
        [[fallthrough]];
    case enumPrivateStage::STAGED_PICK_QUIETS:
        assert(Policy != QUIESCENT);
        scoreQuiets(_idx, pos.getTurn(), node, ply);
        return getNextMoveInfo(next_move, move_score);
    default:
        unreachable();
        break;
    }

    unreachable();
    return false;
}

template <>
_NODISCARD bool MoveOrder::internalNextMove<ONCE_GEN_LEGAL, true>(
                                            search::utils::NodeInfo* node, 
                                            Position& pos, 
                                            Move32b& next_move,
                                            SMoveScore& move_score,
                                            int ply)
{
    assert(_history_cluster != nullptr);

    next_move = NullMove;
    move_score = sc::Undef;

    switch (_stage) {
    case enumPrivateStage::FIRST_STAGE:
        updateContinuationPointers(node, ply);
        _stage = enumPrivateStage::ONCEGEN_HASH_MOVE;
        [[fallthrough]];
    case enumPrivateStage::ONCEGEN_HASH_MOVE:
        _stage = enumPrivateStage::ONCEGEN_ALL;

        if (!_hash_move.isNullMove())
            next_move = _hash_move;
        
        [[fallthrough]];
    case enumPrivateStage::ONCEGEN_ALL:
        _stage = enumPrivateStage::ONCEGEN_PICK_CAPTURES;

        MoveGen::generateLegalMoves<MoveGen::TACTICALS_ONLY_QUEENPROMOS>(pos, _move_list);
        scoreTacticals(0, pos);

        _quiets_idx = _move_list.count();
        MoveGen::generateLegalMoves<MoveGen::QUIETS_ONLY_UNDERPROMOS>(pos, _move_list);

        if (!next_move.isNullMove()) // got hash move assigned already
            return true;

        [[fallthrough]];
    case enumPrivateStage::ONCEGEN_PICK_CAPTURES:
        // Search for another capture only, stop at quiets
        if (getNextMoveInfo(next_move, move_score, _quiets_idx))
            return true;

        _stage = enumPrivateStage::ONCEGEN_PICK_QUIETS;
        [[fallthrough]];
    case enumPrivateStage::ONCEGEN_PICK_QUIETS:
        scoreQuiets(_idx, pos.getTurn(), node, ply);
        return getNextMoveInfo(next_move, move_score);
    default:
        unreachable();
        break;
    }

    unreachable();
    return false;
}

template <int8_t Sign>
void MoveOrder::updateQuietEntry(Move32b move, 
                                 enumColor side, 
                                 int16_t hist_bonus, 
                                 int16_t cont_bonus,
                                 const search::utils::NodeInfo* node,
                                 int ply) 
{
    static_assert(Sign == -1 or Sign == 1);
    assert(_history_cluster != nullptr);

    auto& hist_table = _history_cluster->getHistoryTable();

    hist_table.update<Sign>(side, move, hist_bonus);

    for (int i = 0; i < HistoryTablesCluster::ContinuationPlyCount and i <= ply; i++) {
        const search::utils::NodeInfo* prev_node = node - i - 1;
        assert(prev_node->continuation_subtable_ptr != nullptr);

        auto& cont_subtable = *prev_node->continuation_subtable_ptr;
        cont_subtable.update<Sign>(side, move, cont_bonus);
    }
}

void MoveOrder::updateHistories(Move32b bestmove, 
                                enumColor side, 
                                int depth, 
                                int ply,
                                const Position& pos,
                                const search::utils::NodeInfo* node) 
{
    auto& captures_history = _history_cluster->getCapturesHistoryTable();

    if (bestmove.isQuiet() and !bestmove.isQueenPromotion()) {
        const auto [hist_bonus, cont_bonus] = getHistoriesBonuses(depth);
        updateQuietEntry<+1>(bestmove, side, hist_bonus, cont_bonus, node, ply);
    }
    else if (bestmove.isCapture()) {
        const int16_t capt_bonus = getCaptureBonus(depth);
        captures_history.update<+1>(side, bestmove, pos, capt_bonus);
    }

    const int16_t capt_penalty = getCapturePenalty(depth);
    const int16_t first_quiet_idx = _quiets_idx != static_cast<size_t>(-1) ? _quiets_idx : _move_list.count();
    const int16_t bestmove_idx = static_cast<int16_t>(_idx) - 1;

    for (int16_t i = 0; i < std::min<int16_t>(first_quiet_idx, bestmove_idx); i++) {
        const Move32b move = _move_list.getEntry(i).move();
        captures_history.update<-1>(side, move, pos, capt_penalty);
    }

    const auto [hist_penalty, cont_penalty] = getHistoriesPenalties(depth);

    for (int16_t i = first_quiet_idx; i < bestmove_idx; i++) {
        assert(bestmove.isQuiet() and !bestmove.isQueenPromotion());
        const Move32b move = _move_list.getEntry(i).move();
        updateQuietEntry<-1>(move, side, hist_penalty, cont_penalty, node, ply);
    }
}

void MoveOrder::updateContinuationPointers(search::utils::NodeInfo* node, int ply) {
    auto& cont_table = _history_cluster->getContinuationTable();

    for (int i = 0; i < mvo::HistoryTablesCluster::ContinuationPlyCount and i <= ply; i++) {
        search::utils::NodeInfo* prev_node = node - i - 1;
        prev_node->continuation_subtable_ptr = 
            &cont_table.getSubtable(prev_node->side2move, prev_node->move);
    }
}

_NODISCARD _FORCEINLINE std::tuple<hist::HistoryTable::value_type, hist::ContinuationTable::value_type> 
MoveOrder::getHistoriesBonuses(int depth) {
    const int32_t unscaled_hist_bonus = (
        MvOrQuietBonusHistoryScore2Coeff * depth * depth + 
        MvOrQuietBonusHistoryScore1Coeff * depth
    );

    const int32_t unscaled_cont_bonus = (
        MvOrContBonusHistoryScore2Coeff * depth * depth + 
        MvOrContBonusHistoryScore1Coeff * depth 
    );

    return std::make_tuple(unscaled_hist_bonus / 1024, unscaled_cont_bonus / 1024);
}

_NODISCARD _FORCEINLINE std::tuple<hist::HistoryTable::value_type, hist::ContinuationTable::value_type> 
MoveOrder::getHistoriesPenalties(int depth) {
    const int32_t unscaled_hist_penalty = (
        MvOrQuietPenaltyHistoryScore2Coeff * depth * depth + 
        MvOrQuietPenaltyHistoryScore1Coeff * depth
    );

    const int32_t unscaled_cont_penalty = (
        MvOrContPenaltyHistoryScore2Coeff * depth * depth + 
        MvOrContPenaltyHistoryScore1Coeff * depth    
    );

    return std::make_tuple(unscaled_hist_penalty / 1024, unscaled_cont_penalty / 1024);
}

_NODISCARD _FORCEINLINE hist::CaptureHistory::value_type MoveOrder::getCaptureBonus(int depth) {
    return depth * depth / 2;
}

_NODISCARD _FORCEINLINE hist::CaptureHistory::value_type MoveOrder::getCapturePenalty(int depth) {
    return depth * depth / 2;
}

_INLINE bool MoveOrder::nextMoveFromList(Move32b& move, 
                                         SMoveScore& score, 
                                         size_t end_idx) 
{
    assert(_idx <= end_idx);

    const size_t end = std::min(end_idx, _move_list.count());

    while (_idx < end) {
        _move_list.selectBest(_idx, end_idx);

        const ml::MoveList::Entry entry = _move_list.getEntry(_idx++);

        move = entry.move();
        score = static_cast<SMoveScore>(entry.score());

        if (move != _hash_move and move != _killer_move)
            return true;
    }

    return false;
}

_INLINE bool MoveOrder::getNextMoveInfo(Move32b& move, 
                                        SMoveScore& score, 
                                        size_t end_idx) 
{
    const bool found = nextMoveFromList(move, score, end_idx);
    score = getOutputMoveScore(move, score);
    return found;
}

void MoveOrder::scoreTacticals(size_t beg_idx, const Position& pos) {
    const auto& captures_history = _history_cluster->getCapturesHistoryTable();
    const enumColor side = pos.getTurn();
    
    for (size_t i = beg_idx; i < _move_list.count(); i++) {
        ml::MoveList::Entry& entry = _move_list.getEntry(i);

        const Move32b move = entry.move();
        ml::MoveScore score = 0;

        assert(move.isCapture() or move.isQueenPromotion());

        if (move.isEnPassant()) {
            const Piece::value_type attacker = pc::value(move.getPiece());
            const int16_t hist_score = captures_history.getValue(side, move, Piece::PAWN);
            score = getCapturedScore(Piece::PAWN) * 10 - attacker + hist_score;
        }
        else if (move.isCapture()) {
            const Piece::enumType vic = move.getCaptured(pos);
            const Piece::value_type attacker = pc::value(move.getPiece());
            const int16_t hist_score = captures_history.getValue(side, move, vic);
            score = getCapturedScore(vic) * 10 - attacker + hist_score;
        }
        
        if (move.isPromotion()) {
            const Piece::enumType promo = move.getPromoPiece();
            score += getPromotionScore(promo) * 10;
        }

        entry.setScore(score);
    }
}

void MoveOrder::scoreQuiets(size_t beg_idx, 
                            enumColor side, 
                            const search::utils::NodeInfo* node, 
                            int ply) 
{
    assert(_history_cluster != nullptr);

    const auto& hist_table = _history_cluster->getHistoryTable();

    for (size_t i = beg_idx; i < _move_list.count(); i++) {
        ml::MoveList::Entry& entry = _move_list.getEntry(i);

        const Move32b move = entry.move();
        assert(move.isQuiet() or move.isUnderPromotion());

        SMoveScore score = hist_table.getValue(side, move);

        /* Apply continuation score */

        for (int j = 0; j < HistoryTablesCluster::ContinuationPlyCount and j <= ply; j++) {
            const search::utils::NodeInfo* prev_node = node - j - 1;
            assert(prev_node->continuation_subtable_ptr != nullptr);

            const auto& cont_subtable = *prev_node->continuation_subtable_ptr;

            const int16_t cont_value = cont_subtable.getValue(side, move);
            const int32_t scaled_cont_value = getContinuationPlyScale(j) * cont_value / 1024;

            score += scaled_cont_value;
        }

        entry.setScore(score);  
    }
}

_NODISCARD _FORCEINLINE SMoveScore MoveOrder::getOutputMoveScore(Move32b move, SMoveScore s) {
    return move.isCapture() or move.isQueenPromotion() ? s 
            : s.quietOntoOutputRange(_history_cluster->getMaxTotalAbsValue());
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

template bool MoveOrder::nextMoveWithPolicy<STAGED, false>(search::utils::NodeInfo*, Position&, Move32b&, SMoveScore&, int);
template bool MoveOrder::nextMoveWithPolicy<QUIESCENT, false>(search::utils::NodeInfo*, Position&, Move32b&, SMoveScore&, int);
template bool MoveOrder::nextMoveWithPolicy<ONCE_GEN_LEGAL, true>(search::utils::NodeInfo*, Position&, Move32b&, SMoveScore&, int);

} // namespace mvo
