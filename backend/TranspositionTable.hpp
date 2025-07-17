#pragma once

#include "Common.hpp"
#include "Score.hpp"
#include "Move.hpp"

class Score;
struct SearchResults;

#define ENTRY_TARGET_SIZE  16
#define BUCKET_TARGET_SIZE 32

struct alignas(ENTRY_TARGET_SIZE) TTEntry {
	enum Bound : uint8_t {
		NONE = 0,
		EXACT = 1,
		LOWERBOUND = 2,
		UPPERBOUND = 3,
	};

	INLINE bool isEmpty() { 
		return depth == 0 and bound == NONE; 
	}

	uint64_t key;
	Move16b  move;
	Score	 score;
	uint8_t  depth;
	Bound	 bound;
	uint8_t  generation;
};

struct alignas(BUCKET_TARGET_SIZE) TTBucket {
	static constexpr size_t internal_entries_cnt = 2;
	TTEntry entries[internal_entries_cnt];
};

class TranspositionTable {
public:
	TranspositionTable();
	~TranspositionTable();

	void resize(size_t size_mb);
	void clear();

	void write(uint64_t node_key, uint8_t node_depth, uint8_t node_ply, 
		TTEntry::Bound node_bound, Score node_score, Move16b node_move, SearchResults& results);

	bool probe(TTEntry& out_entry, uint64_t key, Score alpha, Score beta, uint8_t node_depth, uint8_t node_ply) const;

#if defined(DEBUG)
	void printDebug();
#endif

	size_t getEntriesCount() const;
	uint16_t getHashfull() const;

	void newGeneration();
	void clearHashfull();
private:

	TranspositionTable(TranspositionTable&&) = delete;
	TranspositionTable(const TranspositionTable&) = delete;
	TranspositionTable& operator=(const TranspositionTable&) = delete;
	TranspositionTable& operator=(const TranspositionTable&&) = delete;

	TTBucket* _mem;
	size_t _buckets_cnt;
	uint8_t _generation;
	ull _hits;
};
