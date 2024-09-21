#pragma once

#include "Common.hpp"
#include "Search.hpp"

struct alignas(16) TTEntry {
	enum Bound : uint8_t {
		NONE = 0,
		EXACT = 1,
		LOWERBOUND = 2,
		UPPERBOUND = 3,
	};

	uint64_t key;
	uint8_t depth;
	Bound bound;
	Score score;
	Move move;
};

class Score;
struct SearchResults;

class TranspositionTable {
public:
	TranspositionTable();
	TranspositionTable(TranspositionTable&& rtt) noexcept;
	~TranspositionTable();

	void resize(size_t size_mb);
	void clear();

	void write(uint64_t node_key, uint8_t node_depth, uint8_t node_ply, 
		TTEntry::Bound node_bound, Score node_score, Move node_move, SearchResults& results);

	bool probe(TTEntry& out_entry, uint64_t key, Score alpha, Score beta, uint8_t node_depth, uint8_t node_ply);

#if defined(_DEBUG)
	void printDebug();
#endif

	size_t getEntriesCount() const;
private:

	TranspositionTable(const TranspositionTable&) = delete;
	TranspositionTable operator=(const TranspositionTable&) = delete;

	void memFree() noexcept;
	TTEntry* memAlloc(size_t size);

	TTEntry* _mem;
	size_t _size;
};

inline size_t TranspositionTable::getEntriesCount() const {
	return _size;
}