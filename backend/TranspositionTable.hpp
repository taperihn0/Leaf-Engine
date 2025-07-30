#pragma once

#include "Common.hpp"
#include "Score.hpp"
#include "Move.hpp"

class Score;
struct SearchResults;

static constexpr size_t EntryTargetSize  = 10;
static constexpr size_t BucketTargetSize = 32;

struct TTEntry {
	enum Bound : uint8_t {
		NONE = 0,
		EXACT = 1,
		LOWERBOUND = 2,
		UPPERBOUND = 3,
	};

	INLINE bool isEmpty() const { 
		return depth == 0 and bound == NONE; 
	}

	INLINE void writeHash(uint32_t key) {
		*reinterpret_cast<uint32_t*>(this) = key;
	}

	INLINE uint32_t getHash() const {
		return *reinterpret_cast<const uint32_t*>(this);
	}

	uint16_t keyhi;
	uint16_t keylo;
	uint8_t  depth;
	Bound	 bound : 2;
	uint8_t  generation : 6;
	Move16b  move;
	Score	 score;
};

struct alignas(BucketTargetSize) TTBucket {
	static constexpr size_t InternalEntriesCnt = 3;
	TTEntry entries[InternalEntriesCnt];
	_UNUSED uint16_t __alignment;
};

class TranspositionTable {
public:
	TranspositionTable();
	~TranspositionTable();

	void resize(size_t size_mb);
	void clear();

	void write(uint64_t node_key, uint8_t node_depth, uint8_t node_ply, 
			   TTEntry::Bound node_bound, Score node_score, Move16b node_move, SearchResults& results);

	bool probe(TTEntry& out_entry, uint64_t key, Score alpha, Score beta, uint8_t node_depth) const;

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
