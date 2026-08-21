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

    static constexpr int32_t MinQuietValue = 0;
    static constexpr int32_t MaxQuietValue = 16384;

    MoveScore() = default;
    _INLINE constexpr MoveScore(const MoveScore&) = default;
    _INLINE constexpr MoveScore(const sc::Score& s) noexcept 
        : ScoreBase(s.value()) {}
    _INLINE constexpr MoveScore(int32_t val) noexcept
        : ScoreBase(val) {}

    _NODISCARD _FORCEINLINE int32_t value() const {
        return _v; 
    }

    _NODISCARD _FORCEINLINE ml::MoveScore centered() const {
        assert(_v >= MinQuietValue and _v <= MaxQuietValue);
        return value() - ml::MoveScore::MaxQuietValue / 2;
    }

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
    _INLINE constexpr bool operator<(ScoredMove sm) const noexcept { return _score < sm._score; }

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

    void sort(std::size_t first, std::size_t end);
    void partialSort(std::size_t first, std::size_t mid, std::size_t end);

    _INLINE void push(Move32b new_move);
    _INLINE ScoredMove& getEntry(std::size_t idx);
    _INLINE std::size_t count() const { return _tail_idx; }

    bool contains(Move32b m) const;
    _FORCEINLINE void clear() { _tail_idx = 0; }

    _INLINE void print() const;
    _INLINE void selectBest(std::size_t idx, std::size_t end_idx = maxof<std::size_t>());

    Move32b getRandomMove() const;

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
    _INLINE ScoredMove* end() { return _moves.data() + _tail_idx; }
    _INLINE const ScoredMove* begin() const { return _moves.data(); }
    _INLINE const ScoredMove* end() const { return _moves.data() + _tail_idx; }

private:
    static constexpr std::size_t     _MaxSize = MaxNodeMoves;
    std::size_t                      _tail_idx = 0;
    std::array<ScoredMove, _MaxSize> _moves = {};
};

_INTERNAL void MoveList::sort(std::size_t first, std::size_t end) {
    std::sort(_moves.data() + first, _moves.data() + end);
}

_INTERNAL void MoveList::partialSort(std::size_t first, std::size_t mid, std::size_t end) {
    std::partial_sort(_moves.data() + first, 
                      _moves.data() + mid, 
                      _moves.data() + end);
}

_INLINE void MoveList::push(Move32b new_move) { 
    assert(_tail_idx < _MaxSize); 
    _moves[_tail_idx++].setMove(new_move); 
}

_INLINE ScoredMove& MoveList::getEntry(std::size_t idx) { 
    assert(idx < _tail_idx); 
    return _moves[idx]; 
}

_INTERNAL bool MoveList::contains(Move32b m) const { 
    return std::find_if(begin(), end(), 
                        [m](ScoredMove e) { 
                            return e.move() == m; 
                        }) != end(); 
}

_INLINE void MoveList::print() const {
    for (const auto& m : *this)
        m.move().print(), std::cout << '\n';
}

_INLINE void MoveList::selectBest(std::size_t idx, std::size_t end_idx) {
    assert(idx < end_idx);

    ScoredMove& best = *std::max_element(_moves.data() + idx,
                                         _moves.data() + std::min(end_idx, _tail_idx));

    std::swap(best, _moves[idx]);
}

_INTERNAL Move32b MoveList::getRandomMove() const {
    if (!_tail_idx) 
        return NullMove;

    const std::size_t random_idx = rnd::random<std::size_t>(0, _tail_idx - 1);
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
    _tail_idx = std::distance(begin(), last);
    return *this;
}

} // namespace ml
