#pragma once

#include "Common.hpp"
#include "Score.hpp"
#include "Move.hpp"

class Score;
struct SearchResults;

static constexpr size_t EntryTargetSize  = 10;
static constexpr size_t BucketTargetSize = 32;
static constexpr size_t EntryKeySize     = 18;

struct TTEntry {
	enum Bound : uint8_t {
		NONE 	   = 0,
		EXACT  	   = 1,
		LOWERBOUND = 2,
		UPPERBOUND = 3,
		MAX_BOUND  = 3
	};

	INLINE bool isEmpty() const { 
		return depth == 0 and bound == NONE; 
	}

	INLINE void writeHash(uint32_t keyhi) {
		key16 = static_cast<uint16_t>(keyhi);
		key18 = (keyhi & 0x30000) >> 16;
	}

	INLINE uint32_t getHash() const {
		uint32_t h;
		std::memcpy(&h, this, sizeof(uint32_t));
		return h & 0x3FFFF;
	}

	uint16_t key16;
	uint16_t key18 : 2;
	uint8_t  generation : 6;
	Bound	 bound : 2;
	int8_t   depth : 6;
	Move16b  move;
	Score	 score;
	Score    eval;
};

static_assert(sizeof(TTEntry) == EntryTargetSize);

struct alignas(BucketTargetSize) TTBucket {
	static constexpr size_t InternalEntriesCnt = 3;
	TTEntry entries[InternalEntriesCnt];
	_UNUSED int16_t __align;
};

static_assert(sizeof(TTBucket) == BucketTargetSize);

class TranspositionTable {
public:
	TranspositionTable(size_t mb_size = 1_MB);
	TranspositionTable(TranspositionTable&& tt);
	~TranspositionTable();

	TranspositionTable(const TranspositionTable&) = delete;
	TranspositionTable& operator=(const TranspositionTable&) = delete;
	TranspositionTable& operator=(TranspositionTable&&) = delete;

	void resize(size_t size_mb);
	void clear();

	void write(uint64_t node_key, uint8_t node_depth, 
			   uint8_t node_ply, TTEntry::Bound node_bound, 
			   Score node_score, Move16b node_move, Score node_eval,
			   SearchResults& results);

	bool probe(TTEntry& out_entry,
			   uint64_t key, 
			   Score alpha, Score beta, 
			   uint8_t node_depth) const;

	void prefetchBucket(uint64_t key64) const;

#if defined(DEBUG)
	void printDebug();
#endif

	size_t getEntriesCount() const;
	uint16_t getHashfull() const;

	void newGeneration();
	void clearHashfull();
private:
	TTBucket* _mem;
	size_t    _buckets_cnt;
	uint8_t   _buckets_pow_2;
	uint8_t   _generation;
	ull       _hits;
};
