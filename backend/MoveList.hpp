#pragma once

#include "Move.hpp"

#include <algorithm>

class MoveList {
public:
	struct Entry {
		INLINE constexpr bool operator==(Entry b) const noexcept {
			return move == b.move;
		}

		Move move;
		uint16_t score;
	};

	INLINE void sort(size_t first, size_t end) {
		std::sort(_moves.data() + first, _moves.data() + end, _greater_score);
	}

	INLINE void partialSort(size_t first, size_t mid, size_t end) {
		std::partial_sort(_moves.data() + first, _moves.data() + mid, _moves.data() + end, 
			_greater_score);
	}

	INLINE void push(Move&& new_move) {
		assert(_idx < _max_size);
		_moves[_idx++].move = new_move;
	}

	INLINE Entry* getEntry(size_t idx) {
		assert(idx < _idx);
		return _moves.data() + idx;
	}

	INLINE Move getMove(size_t idx) {
		assert(idx < _idx);
		return _moves[idx].move;
	}

	INLINE size_t count() const {
		return _idx;
	}

	INLINE bool contains(Move m) const {
		return std::find_if(_moves.data(), _moves.data() + _idx, 
			[m](Entry e) { return e.move == m; }) != _moves.data() + _idx;
	}

	INLINE void clear() { _idx = 0; }

	void print() const {
		for (size_t i = 0; i < _idx; i++)
			_moves[i].move.print(), std::cout << '\n';
	}

	void selectSort(size_t first);

private:
	static constexpr size_t _max_size = max_node_moves;

	inline static const auto _greater_score = [](Entry a, Entry b) _LAMBDA_FORCEINLINE {
		return a.score > b.score;
	};

	size_t _idx = 0;
	std::array<Entry, _max_size> _moves;
};

inline void MoveList::selectSort(size_t first) {
	assert(first < _idx);
	uint16_t best = _moves[first].score;
	size_t ind = first;

	for (size_t i = first + 1; i < _idx; i++) {
		if (_moves[i].score > best) {
			best = _moves[i].score;
			ind = i;
		}
	}

	std::swap(_moves[ind], _moves[first]);
}
