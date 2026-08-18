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

#include <algorithm>

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

class MoveList {
public:
    struct Entry {
        constexpr bool operator==(Entry b) const noexcept {
            return move == b.move;
        }

        Move32b   move;
        MoveScore score;
    };

    static_assert(is_same<MoveScore::int_t, int32_t>);

    MoveList() = default;

    _INLINE void sort(size_t first, size_t end) {
        std::sort(_moves.data() + first, _moves.data() + end, _GreaterScore);
    }

    _INLINE void partialSort(size_t first, size_t mid, size_t end) {
        std::partial_sort(_moves.data() + first, 
                          _moves.data() + mid, 
                          _moves.data() + end, 
                          _GreaterScore);
    }

    _INLINE void push(Move32b new_move) {
        assert(_idx < _MaxSize);
        _moves[_idx++].move = new_move;
    }

    _INLINE Entry& getEntry(size_t idx) {
        assert(idx < _idx);
        return _moves[idx];
    }

    _INLINE Move32b getMove(size_t idx) const {
        assert(idx < _idx);
        return _moves[idx].move;
    }

    _INLINE MoveScore getScore(size_t idx) const {
        assert(idx < _idx);
        return _moves[idx].score;
    }

    _INLINE size_t count() const {
        return _idx;
    }

    _INLINE bool contains(Move32b m) const {
        return std::find_if(_moves.data(), _moves.data() + _idx, 
            [m](Entry e) { 
                return e.move == m; 
            }) != _moves.data() + _idx;
    }

    _INLINE void clear() { _idx = 0; }

    void print() const {
        for (size_t i = 0; i < _idx; i++)
            _moves[i].move.print(), std::cout << '\n';
    }

    void selectSort(size_t first_ind);

    _INLINE Move32b getRandomMove() const {
        if (!_idx) 
            return NullMove;

        size_t random_idx = rnd::random<size_t>(0, _idx - 1);
        return _moves[random_idx].move;
    }

    template <typename Pred, 
              typename = std::enable_if_t<std::is_invocable_v<
                                            std::remove_reference_t<Pred>, Entry>
                                         >
    >
    _INLINE bool any(Pred&& pred) const {
        for (size_t i = 0; i < _idx; i++) {
            if (pred(_moves[i]))
                return true;
        }

        return false;
    }

    template <typename Pred, 
              typename = std::enable_if_t<std::is_invocable_v<
                                            std::remove_reference_t<Pred>, Entry>
                                         >
    >
    _INLINE MoveList& remove(Pred&& pred) {
        auto last = std::remove_if(_moves.begin(), 
                                   std::next(_moves.begin(), _idx), 
                                   pred);

        _idx = std::distance(_moves.begin(), last);
        return *this;
    }

private:
    static constexpr size_t _MaxSize = MaxNodeMoves;

    inline static const constexpr auto _GreaterScore = [](Entry a, Entry b) _LAMBDA_FORCEINLINE {
        return a.score > b.score;
    };

    size_t                   _idx = 0;
    Array1d<Entry, _MaxSize> _moves = {};
};

_INLINE void MoveList::selectSort(size_t first_ind) {
    assert(first_ind < _idx);

    MoveScore best = _moves[first_ind].score;
    size_t ind = first_ind;

    for (size_t i = first_ind + 1; i < _idx; i++) {
        if (_moves[i].score > best) {
            best = _moves[i].score;
            ind = i;
        }
    }

    std::swap(_moves[ind], _moves[first_ind]);
}

} // namespace ml

