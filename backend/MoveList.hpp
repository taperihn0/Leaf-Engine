#pragma once

#include "Move.hpp"

#include <algorithm>

class MoveList {
public:
	INLINE void sort() {
		std::sort(_moves.begin(), _moves.end(), 
			[](Entry e1, Entry e2) { return e1.score < e2.score; });
	}

	INLINE void push(Move new_move) {
		assert(idx < _size);
		_moves[idx++].move = new_move;
	}

	INLINE uint32_t getScore(uint32_t idx) const {
		ASSERT(idx < _size, "Index overflow while geting an item from move list");
		return _moves[idx].score;
	}

	INLINE Move getMove(uint32_t idx) const {
		ASSERT(idx < _size, "Index overflow while geting an item from move list");
		return _moves[idx].move;
	}

	INLINE uint32_t size() const {
		return _size;
	}

	void print() const {
		for (Entry m : _moves) m.print();
	}

private:
	static constexpr uint32_t _size = max_node_moves;

	struct Entry {
		void print() { move.print(); std::cout << score; }

		Move move;
		// TODO: move score datatype 
		uint32_t score;
	};

	int idx = 0;
	std::array<Entry, _size> _moves;
};