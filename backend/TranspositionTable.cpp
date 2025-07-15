#include "TranspositionTable.hpp"
#include "Search.hpp"

inline constexpr size_t operator""_MB(ull mb_count) {
	return mb_count * 1024 * 1024;
}

TranspositionTable::TranspositionTable() {
	static_assert(sizeof(TTEntry) == 16);
	_mem = reinterpret_cast<TTEntry*>(alignedMalloc(128_MB, sizeof(TTEntry)));
	_entry_cnt = 128_MB / sizeof(TTEntry);
	clear();
}

TranspositionTable::TranspositionTable(TranspositionTable&& rtt) noexcept {
	alignedFree(_mem);
	_mem = rtt._mem;
	_entry_cnt = rtt._entry_cnt;
}

TranspositionTable::~TranspositionTable() { 
	alignedFree(_mem);
}

void TranspositionTable::resize(size_t size_mb) {
	alignedFree(_mem);
	_mem = reinterpret_cast<TTEntry*>(alignedMalloc(size_mb * 1024 * 1024, sizeof(TTEntry)));
	_entry_cnt = size_mb * 1024 * 1024 / sizeof(TTEntry);
}

void TranspositionTable::clear() {
	alignedMemset(_mem, 0, _entry_cnt * sizeof(TTEntry));
}

void TranspositionTable::write(uint64_t node_key, uint8_t node_depth, uint8_t node_ply, 
	TTEntry::Bound node_bound, Score node_score, Move32b node_move, SearchResults& results) {
	TTEntry* const entry = _mem + (node_key & (_entry_cnt - 1));

	if (!entry->depth)
		results.tt_hits++;

	entry->key   = node_key;
	entry->depth = node_depth;
	entry->bound = node_bound;
	entry->score = node_score;
	entry->move  = node_move;
}

bool TranspositionTable::probe(TTEntry& out_entry, uint64_t key, Score alpha, Score beta, uint8_t node_depth, uint8_t node_ply) const {
	const TTEntry* const entry = _mem + (key & (_entry_cnt - 1));
	
	if (entry->key != key) {
		out_entry.move = Move32b::null;
		return false;
	} 
	else if (entry->depth < node_depth) {
		out_entry.move = entry->move;
		return false;
	}

	switch (entry->bound) {
	case TTEntry::EXACT: {
		const Score mate_score = entry->score > Score::mate_bound ?
								 entry->score - node_ply :
								 entry->score < -Score::mate_bound ?
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
