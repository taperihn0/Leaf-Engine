#include "TranspositionTable.hpp"
#include "Search.hpp"

inline constexpr size_t operator""_MB(size_t mb_count) {
	return mb_count * 1024 * 1024;
}

TranspositionTable::TranspositionTable() {
	static_assert(sizeof(TTEntry) == 16);
	_size = 128_MB / sizeof(TTEntry);
	_mem = memAlloc(_size);
}

TranspositionTable::TranspositionTable(TranspositionTable&& rtt) noexcept {
	memFree();
	_mem = rtt._mem;
	_size = rtt._size;
}

TranspositionTable::~TranspositionTable() { 
	clear();
}

void TranspositionTable::resize(size_t size_mb) {
	memFree();
	_size = size_mb * 1024 * 1024 / sizeof(TTEntry);
	_mem = memAlloc(_size);
}

void TranspositionTable::clear() {
	memFree();
	_mem = nullptr;
	_size = 0;
}

void TranspositionTable::write(uint64_t node_key, uint8_t node_depth, uint8_t node_ply, TTEntry::Bound node_bound, Score node_score) {
	if (node_score > Score::infinity - (int16_t)max_depth)
		node_score += node_ply;
	else if (node_score < -Score::infinity + (int16_t)max_depth)
		node_score -= node_ply;

	_mem[node_key & (_size - 1)] = TTEntry{ node_key, node_depth, node_bound, node_score };
}

INLINE auto validScore(Score score) { 
	return std::pair{ true, score };
}

INLINE auto invalidScore() {
	return std::pair{ false, Score::undef };
}

std::pair<bool, Score> TranspositionTable::probe(uint64_t key, Score alpha, Score beta, uint8_t node_depth, uint8_t node_ply) {
	const TTEntry* const entry = _mem + (key & (_size - 1));

	if (entry->depth < node_depth or entry->key != key)
		return invalidScore();

	switch (entry->bound) {
	case TTEntry::EXACT:
		const Score mate_score = entry->score > Score::infinity  - (int16_t)max_depth ? entry->score - node_ply :
								 entry->score < -Score::infinity + (int16_t)max_depth ? entry->score + node_ply :
																				        entry->score;
		return validScore(mate_score);
	case TTEntry::LOWERBOUND:
		return alpha >= entry->score ? validScore(alpha)
									 : invalidScore();
	case TTEntry::UPPERBOUND:
		return beta <=  entry->score ? validScore(beta)
								     : invalidScore();
	}

	return invalidScore();
}

#if defined(_DEBUG)
void TranspositionTable::printDebug() {
	std::cout << "Hash size: " << _size * sizeof(TTEntry) / 1024 / 1024 << "MB\n";
}
#endif

inline void TranspositionTable::memFree() noexcept {
	delete[] _mem;
}

inline TTEntry* TranspositionTable::memAlloc(size_t size) {
	return new TTEntry[size];
}