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

#include "Move.hpp"

namespace ml {

class MoveScore final : public sc::util::ScoreBase<MoveScore, int32_t> {
public:
    friend class sc::util::ScoreBase<MoveScore, int32_t>;
    using Base = sc::util::ScoreBase<MoveScore, int32_t>;

    using Base::operator=;

    MoveScore() = default;
    _INLINE constexpr MoveScore(const MoveScore&) = default;
    _INLINE constexpr MoveScore(const sc::Score& s) noexcept 
        : ScoreBase(s.value()) {}
    _INLINE constexpr MoveScore(int32_t val) noexcept
        : ScoreBase(val) {}

    _FORCEINLINE constexpr MoveScore& operator=(const sc::Score& s) noexcept {
        _v = static_cast<const sc::Score::Base&>(s)._v;
        return *this;
    }
private:
    using Base::_v;
};

class ScoredMove {
public:
    ScoredMove() = default;

    _INLINE constexpr explicit ScoredMove(Move32b move, MoveScore score = MoveScore()) noexcept
        : _move(move), _score(score) {}

    _INLINE constexpr bool operator==(ScoredMove sm) const noexcept { return _move == sm._move; }
    _INLINE constexpr bool operator>(ScoredMove sm) const noexcept { return _score > sm._score; }

    _INLINE constexpr Move32b move() const noexcept { return _move; }
    _INLINE constexpr MoveScore score() const noexcept { return _score; }

    _INLINE constexpr void setMove(Move32b move) noexcept { _move = move; }
    _INLINE constexpr void setScore(MoveScore score) noexcept { _score = score; }

private:
    Move32b   _move;
    MoveScore _score;
};

class MoveList {
public:
    using Entry = ScoredMove;

    MoveList() = default;

    _INLINE void sort(std::size_t first, std::size_t end);
    _INLINE void partialSort(std::size_t first, std::size_t mid, std::size_t end);

    _INLINE void push(Move32b new_move) { assert(_idx < _MaxSize); _moves[_idx++].setMove(new_move); }
    _INLINE ScoredMove& getEntry(std::size_t idx) { assert(idx < _idx); return _moves[idx]; }
    _INLINE std::size_t count() const { return _idx; }

    _INLINE bool contains(Move32b m) const { return std::find_if(begin(), end(), [m](ScoredMove e) { return e.move() == m; }) != end(); }
    _FORCEINLINE void clear() { _idx = 0; }

    _INLINE void print() const;
    _INLINE void selectBest(std::size_t first_idx, std::size_t end_idx = -1);

    _INLINE Move32b getRandomMove() const;

    template <typename Pred, 
              typename = std::enable_if_t<std::is_invocable_v<
                                            std::remove_reference_t<Pred>, ScoredMove>
                                         >
    >
    _INLINE bool any(Pred&& pred) const;

    template <typename Pred, 
              typename = std::enable_if_t<std::is_invocable_v<
                                            std::remove_reference_t<Pred>, ScoredMove>
                                         >
    >
    _INLINE MoveList& remove(Pred&& pred);

    _INLINE ScoredMove* begin() { return _moves.data(); }
    _INLINE ScoredMove* end() { return _moves.data() + _idx; }
    _INLINE const ScoredMove* begin() const { return _moves.data(); }
    _INLINE const ScoredMove* end() const { return _moves.data() + _idx; }

private:
    static constexpr std::size_t _MaxSize = MaxNodeMoves;

    std::size_t                      _idx = 0;
    MultiArray<ScoredMove, _MaxSize> _moves = {};
};

_INLINE void MoveList::sort(std::size_t first, std::size_t end) {
    std::sort(_moves.data() + first, _moves.data() + end, std::greater<ScoredMove>{});
}

_INLINE void MoveList::partialSort(std::size_t first, std::size_t mid, std::size_t end) {
    std::partial_sort(_moves.data() + first, 
                      _moves.data() + mid, 
                      _moves.data() + end, 
                      std::greater<ScoredMove>{});
}

_INLINE void MoveList::print() const {
    for (const auto& m : *this)
        m.move().print(), std::cout << '\n';
}

_INLINE void MoveList::selectBest(std::size_t first_idx, std::size_t end_idx) {
    assert(first_idx < end_idx);
    assert(end_idx <= _idx);

    ScoredMove best = _moves[first_idx];
    std::size_t idx = first_idx;

    for (std::size_t i = first_idx + 1; i < end_idx and i < _idx; i++) {
        if (_moves[i] > best) {
            best = _moves[i];
            idx = i;
        }
    }

    std::swap(_moves[idx], _moves[first_idx]);
}

_INLINE Move32b MoveList::getRandomMove() const {
    if (!_idx) 
        return NullMove;

    std::size_t random_idx = rnd::random<std::size_t>(0, _idx - 1);
    return _moves[random_idx].move();
}

template <typename Pred, typename>
_INLINE bool MoveList::any(Pred&& pred) const {
    for (const auto& m : *this) {
        if (pred(m))
            return true;
    }

    return false;
}

template <typename Pred, typename>
_INLINE MoveList& MoveList::remove(Pred&& pred) {
    auto last = std::remove_if(begin(), end(), pred);
    _idx = std::distance(begin(), last);
    return *this;
}

} // namespace ml
