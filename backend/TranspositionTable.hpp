#pragma once

#include "Common.hpp"
#include "Search.hpp"

#include <utility>

// TODO
struct alignas(16) TTEntry {
	enum Bound : uint8_t {
		EXACT = 0,
		LOWERBOUND = 1,
		UPPERBOUND = 2,
		NONE = 3
	};

	uint64_t key;
	uint8_t depth;
	Bound bound;
	Score score;
	Move move;
};

class Score;

class TranspositionTable {
public:
	TranspositionTable();
	TranspositionTable(TranspositionTable&& rtt) noexcept;
	~TranspositionTable();

	void resize(size_t size_mb);
	void clear();

	void write(uint64_t node_key, uint8_t node_depth, uint8_t node_ply, 
		TTEntry::Bound node_bound, Score node_score, Move node_move);

	std::pair<bool, TTEntry> probe(uint64_t key, Score alpha, Score beta, uint8_t node_depth, uint8_t node_ply);

#if defined(_DEBUG)
	void printDebug();
#endif
private:

	TranspositionTable(const TranspositionTable&) = delete;
	TranspositionTable operator=(const TranspositionTable&) = delete;

	void memFree() noexcept;
	TTEntry* memAlloc(size_t size);

	TTEntry* _mem;
	size_t _size;
};