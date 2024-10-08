#pragma once

#include "Move.hpp"

#include <algorithm>
#include <numeric>

class MoveList {
public:
	INLINE void sort(size_t first, size_t end) {
		std::sort(_moves.data() + first, _moves.data() + end, 
			_greater_score);
	}

	INLINE void partialSort(size_t first, size_t mid, size_t end) {
		std::partial_sort(_moves.data() + first, _moves.data() + mid, _moves.data() + end,
			_greater_score);
	}

	INLINE void push(Move&& new_move) {
		assert(_idx < _size);
		_moves[_idx++].move = new_move;
	}

	INLINE uint32_t getScore(size_t idx) const {
		ASSERT(idx < _size, "Index overflow while geting an item from move list");
		return _moves[idx].score;
	}

	INLINE Move getMove(size_t idx) const {
		ASSERT(idx < _size, "Index overflow while geting an item from move list");
		return _moves[idx].move;
	}

	INLINE size_t count() const {
		return _idx;
	}

	INLINE int size() const {
		return _size;
	}

	INLINE bool contains(Move m) const {
		return std::find_if(_moves.data(), _moves.data() + _idx, 
			[m](Entry e) { return e.move == m; }) != _moves.data() + _idx;
	}

	INLINE void clear() { _idx = 0; }

	void print() const {
		for (int i = 0; i < _idx; i++)
			_moves[i].move.print(), std::cout << '\n';
	}

	void scoreCaptures(size_t first, const Position& pos);
	// TODO: scoreQuiets function

private:
	static constexpr size_t _size = max_node_moves;

	struct Entry {
		INLINE constexpr bool operator==(Entry b) const noexcept {
			return move == b.move;
		}

		Move move;
		// TODO: move score datatype 
		int16_t score;
	};

	inline static const auto _greater_score = [](Entry a, Entry b) _LAMBDA_FORCEINLINE {
		return a.score > b.score;
	};

	size_t _idx = 0;
	std::array<Entry, _size> _moves;
};