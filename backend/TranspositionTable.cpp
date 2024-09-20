#include "TranspositionTable.hpp"

static constexpr TTEntry undef_entry = TTEntry{};

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

void TranspositionTable::write(uint64_t node_key, uint8_t node_depth, uint8_t node_ply, 
	TTEntry::Bound node_bound, Score node_score, Move node_move) {
	if (node_score > Score::infinity - static_cast<int16_t>(max_depth))
		node_score += node_ply;
	else if (node_score < -Score::infinity + static_cast<int16_t>(max_depth))
		node_score -= node_ply;

	_mem[node_key & (_size - 1)] = TTEntry{ node_key, node_depth, node_bound, node_score, node_move };
}

INLINE auto makeValidResult(const TTEntry* entry, Score s) {
	return std::pair{ true, TTEntry{ 0, 0, TTEntry::NONE, s, entry->move} };
}

INLINE auto makeResult(bool valid, const TTEntry* entry, Score s) {
	return std::pair{ valid, TTEntry{ 0, 0, TTEntry::NONE, s, entry->move } };
}

std::pair<bool, TTEntry> TranspositionTable::probe(uint64_t key, Score alpha, Score beta, uint8_t node_depth, uint8_t node_ply) {
	static constexpr auto invalid_entry = std::make_pair(false, undef_entry);
	const TTEntry* const  entry = _mem + (key & (_size - 1));

	if (entry->depth < node_depth or entry->key != key)
		return invalid_entry;

	switch (entry->bound) {
	case TTEntry::EXACT:
		const Score mate_score = entry->score > Score::infinity  - static_cast<int16_t>(max_depth) ? 
								  entry->score - node_ply :
								 entry->score < -Score::infinity + static_cast<int16_t>(max_depth) ? 
							      entry->score + node_ply : entry->score;

		return makeValidResult(entry, mate_score);
	case TTEntry::LOWERBOUND:
		return makeResult(alpha >= entry->score, entry, alpha);
	case TTEntry::UPPERBOUND:
		return makeResult(beta <= entry->score, entry, beta);
	}

	return invalid_entry;
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