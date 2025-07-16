#pragma once

#include "Common.hpp"
#include "Score.hpp"
#include "Move.hpp"

class Score;
struct SearchResults;

#define ENTRY_TARGET_SIZE 16

struct TTEntry {
	enum Bound : uint8_t {
		NONE	   = 0,
		EXACT	   = 1,
		LOWERBOUND = 2,
		UPPERBOUND = 3,
	};

	uint64_t key;
	uint8_t depth;
	Bound bound;
	Score score;
	Move16b move;
};

//struct alignas(CACHELINE_SIZE) TTBucket {
//	static constexpr size_t internal_entries_cnt = 4;
//	TTEntry entries[internal_entries_cnt];
//};

class TranspositionTable {
public:
	TranspositionTable();
	TranspositionTable(TranspositionTable&& rtt) noexcept;
	~TranspositionTable();

	void resize(size_t size_mb);
	void clear();

	void write(uint64_t node_key, uint8_t node_depth, uint8_t node_ply, 
		TTEntry::Bound node_bound, Score node_score, Move16b node_move, SearchResults& results);

	bool probe(TTEntry& out_entry, uint64_t key, Score alpha, Score beta, uint8_t node_depth, uint8_t node_ply) const;

#if defined(_DEBUG)
	void printDebug();
#endif

	size_t getEntriesCount() const;
private:

	TranspositionTable(const TranspositionTable&) = delete;
	TranspositionTable operator=(const TranspositionTable&) = delete;

	TTEntry* _mem;
	size_t _entry_cnt;
};

inline size_t TranspositionTable::getEntriesCount() const {
	return _entry_cnt;
}
