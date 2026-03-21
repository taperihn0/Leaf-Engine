#pragma once

#include "Move.hpp"

#include <algorithm>

class MoveList {
public:
	struct Entry {
		using movescore_t = int32_t;

		INLINE constexpr bool operator==(Entry b) const noexcept {
			return move == b.move;
		}

		Move32b     move;
		movescore_t score;
	};

	using entryscore_t = Entry::movescore_t;

	static_assert(_IS_SAME_TYPE(entryscore_t, int32_t) or
				  _IS_SAME_TYPE(entryscore_t, int16_t));

	MoveList() = default;

	INLINE void sort(size_t first, size_t end) {
		std::sort(_moves.data() + first, _moves.data() + end, _greater_score);
	}

	INLINE void partialSort(size_t first, size_t mid, size_t end) {
		std::partial_sort(_moves.data() + first, 
                          _moves.data() + mid, 
                          _moves.data() + end, 
						  _greater_score);
	}

	INLINE void push(Move32b&& new_move) {
		assert(_idx < _MaxSize);
		_moves[_idx++].move = new_move;
	}

	INLINE Entry* getEntry(size_t idx) {
		assert(idx < _idx);
		return _moves.data() + idx;
	}

	INLINE Move32b getMove(size_t idx) const {
		assert(idx < _idx);
		return _moves[idx].move;
	}

	INLINE entryscore_t getScore(size_t idx) const {
		assert(idx < _idx);
		return _moves[idx].score;
	}

	INLINE size_t count() const {
		return _idx;
	}

	INLINE bool contains(Move32b m) const {
		return std::find_if(_moves.data(), _moves.data() + _idx, [m](Entry e) { 
            return e.move == m; 
        }) != _moves.data() + _idx;
	}

	INLINE void clear() { _idx = 0; }

	void print() const {
		for (size_t i = 0; i < _idx; i++)
			_moves[i].move.print(), std::cout << '\n';
	}

	void selectSort(size_t first_ind);

	INLINE Move32b getRandomMove() const {
        if (!_idx) 
            return Move32b::Null;

		size_t random_idx = random<size_t>(0, _idx - 1);
		return _moves[random_idx].move;
	}

	template <typename Entry_Callable_Bool>
	INLINE bool any(Entry_Callable_Bool pred) const {
		static_assert(std::is_invocable_v<Entry_Callable_Bool, Entry>);

		for (size_t i = 0; i < _idx; i++) {
            if (pred(_moves[i]))
                return true;
		}

		return false;
	}

    template <typename Entry_Callable_Bool>
    INLINE MoveList& remove(Entry_Callable_Bool pred) {
        static_assert(std::is_invocable_v<Entry_Callable_Bool, Entry>);

        auto last = std::remove_if(_moves.begin(), 
                                   std::next(_moves.begin(), _idx), 
                                   pred);

        _idx = std::distance(_moves.begin(), last);

        return *this;
    }

private:
	static constexpr size_t _MaxSize = MaxNodeMoves;

	inline static const auto _greater_score = [](Entry a, Entry b) _LAMBDA_FORCEINLINE {
		return a.score > b.score;
	};

	size_t						_idx = 0;
	std::array<Entry, _MaxSize> _moves = {};
};

inline void MoveList::selectSort(size_t first_ind) {
	assert(first_ind < _idx);
	entryscore_t best = _moves[first_ind].score;
	size_t ind = first_ind;

	for (size_t i = first_ind + 1; i < _idx; i++) {
		if (_moves[i].score > best) {
			best = _moves[i].score;
			ind = i;
		}
	}

	std::swap(_moves[ind], _moves[first_ind]);
}
