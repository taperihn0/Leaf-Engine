#pragma once

#include "Move.hpp"

#include <algorithm>

class MoveList {
public:
	struct Entry {
		using movescore_t = int16_t;

		INLINE constexpr bool operator==(Entry b) const noexcept {
			return move == b.move;
		}

		Move32b move;
		movescore_t score;
	};

	using entryscore_t = Entry::movescore_t;

	static_assert(_IS_SAME_TYPE(entryscore_t, int16_t));

	INLINE void sort(size_t first, size_t end) {
		std::sort(_moves.data() + first, _moves.data() + end, _greater_score);
	}

	INLINE void partialSort(size_t first, size_t mid, size_t end) {
		std::partial_sort(_moves.data() + first, _moves.data() + mid, _moves.data() + end, 
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

	INLINE size_t count() const {
		return _idx;
	}

	INLINE bool contains(Move32b m) const {
		return std::find_if(_moves.data(), _moves.data() + _idx, 
							[m](Entry e) { return e.move == m; }) != _moves.data() + _idx;
	}

	INLINE void clear() { _idx = 0; }

	void print() const {
		for (size_t i = 0; i < _idx; i++)
			_moves[i].move.print(), std::cout << '\n';
	}

	void selectSort(size_t first_ind);

private:
	static constexpr size_t _MaxSize = MaxNodeMoves;

	inline static const auto _greater_score = [](Entry a, Entry b) _LAMBDA_FORCEINLINE {
		return a.score > b.score;
	};

	size_t _idx = 0;
	std::array<Entry, _MaxSize> _moves;
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
