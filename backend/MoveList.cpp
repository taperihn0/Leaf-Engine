#include "MoveList.hpp"

void MoveList::selectSort(size_t first) {
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
