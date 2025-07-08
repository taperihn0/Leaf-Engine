#include "TranspositionTable.hpp"
#include "Search.hpp"

inline constexpr size_t operator""_MB(ull mb_count) {
	return mb_count * 1024 * 1024;
}

TranspositionTable::TranspositionTable() {
	static_assert(sizeof(TTEntry) == 16);
	_entry_cnt = 128_MB / sizeof(TTEntry);
	_mem = memAlloc(_entry_cnt);
	clear();
}

TranspositionTable::TranspositionTable(TranspositionTable&& rtt) noexcept {
	memFree();
	_mem = rtt._mem;
	_entry_cnt = rtt._entry_cnt;
}

TranspositionTable::~TranspositionTable() { 
	memFree();
}

void TranspositionTable::resize(size_t size_mb) {
	memFree();
	_entry_cnt = size_mb * 1024 * 1024 / sizeof(TTEntry);
	_mem = memAlloc(_entry_cnt);
}

void TranspositionTable::clear() {
	std::memset(_mem, 0, _entry_cnt * sizeof(TTEntry));
}

void TranspositionTable::write(uint64_t node_key, uint8_t node_depth, uint8_t node_ply, 
	TTEntry::Bound node_bound, Score node_score, Move node_move, SearchResults& results) {
	if (node_score > Score::infinity - static_cast<int16_t>(max_depth))
		node_score += node_ply;
	else if (node_score < -Score::infinity + static_cast<int16_t>(max_depth))
		node_score -= node_ply;

	TTEntry* const entry = _mem + (node_key & (_entry_cnt - 1));

	if (!entry->depth)
		results.tt_hits++;

	*entry = TTEntry{ node_key, node_depth, node_bound, node_score, node_move };
}

bool TranspositionTable::probe(TTEntry& out_entry, uint64_t key, Score alpha, Score beta, uint8_t node_depth, uint8_t node_ply) const {
	const TTEntry* const entry = _mem + (key & (_entry_cnt - 1));

	if (entry->key != key)
		return false;
	else if (entry->depth < node_depth) {
		out_entry = *entry;
		return false;
	}

	switch (entry->bound) {
	case TTEntry::EXACT: {
		const Score mate_score = entry->score > Score::infinity  - static_cast<int16_t>(max_depth) ? 
								 entry->score - node_ply :
								 entry->score < -Score::infinity + static_cast<int16_t>(max_depth) ? 
							     entry->score + node_ply : 
								 entry->score;

		out_entry = *entry;
		out_entry.score = mate_score;
		return true;
	}
	case TTEntry::LOWERBOUND: {
		out_entry = *entry;
		out_entry.score = alpha;
		return alpha >= entry->score;
	}
	case TTEntry::UPPERBOUND: {
		out_entry = *entry;
		out_entry.score = beta;
		return beta <= entry->score;
	}
	default: break; 
	}

	return false;
}

#if defined(_DEBUG)
void TranspositionTable::printDebug() {
	std::cout << "Hash size: " << _entry_cnt * sizeof(TTEntry) / 1024 / 1024 << "MB\n";
}
#endif

inline void TranspositionTable::memFree() noexcept {
	delete[] _mem;
}

inline TTEntry* TranspositionTable::memAlloc(size_t size) {
	return new TTEntry[size];
}
