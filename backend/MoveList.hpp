#pragma once

#include "Move.hpp"

#include <algorithm>
#include <numeric>

class MoveList {
public:
	INLINE void sort(size_t first, size_t end) {
		std::sort(_moves.data() + first, _moves.data() + end, 
			[](Entry e1, Entry e2) { return e1.score > e2.score; });
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
		return std::find(_moves.data(), _moves.data() + _idx, Entry{ m, 0 }) != _moves.data() + _idx;
	}

	INLINE void clear() { _idx = 0; }

	void print() const {
		for (Entry m : _moves) m.print();
	}

	void scoreCaptures(size_t first, const Position& pos);

private:
	static constexpr size_t _size = max_node_moves;

	struct Entry {
		void print() { move.print(); std::cout << score; }

		INLINE constexpr bool operator==(Entry b) const noexcept {
			return move == b.move;
		}

		Move move;
		// TODO: move score datatype 
		int16_t score;
	};

	size_t _idx = 0;
	std::array<Entry, _size> _moves;
};