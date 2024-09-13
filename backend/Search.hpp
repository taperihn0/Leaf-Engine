#pragma once

#include "Common.hpp"
#include "Position.hpp"
#include "Move.hpp"
#include "MoveOrder.hpp"
#include "Time.hpp"
#include "Eval.hpp"

#include <numeric>

// Wrapper around raw int16_t score. Raw score stands for evaluation score expressed in 
// centipawns (for simplicity, 1 cp is around 1/100 of pawn value)
class Score {
public:
	INLINE Score() = default;
	INLINE Score(uint16_t val)
		: _raw(val) {}

	INLINE Score operator+(Score b) const {
		return _raw + b._raw;
	}

	INLINE Score operator-(Score b) const {
		return _raw - b._raw;
	}

	INLINE bool operator>(Score b) const {
		return _raw > b._raw;
	}

	INLINE bool operator>=(Score b) const {
		return _raw >= b._raw;
	}

	INLINE bool operator<(Score b) const {
		return _raw < b._raw;
	}

	INLINE Score operator-() const {
		return -_raw;
	}

	INLINE int16_t toInt() const {
		return _raw;
	}

	std::string toStr() const;

	static constexpr int16_t draw = 0,
							 infinity = std::numeric_limits<int16_t>::max();
private:
	int16_t _raw;
};

struct SearchResults {
	INLINE SearchResults()
		: score_cp(0), nodes_cnt(0), pv_line{} {}

	void clearPV();

	void printBestMove();
	void print();

	unsigned depth;
	Timer timer;
	Score score_cp;
	uint64_t nodes_cnt;
	std::array<std::array<Move, max_depth>, max_depth> pv_line;
};

struct TreeNodeInfo {
	MoveOrder<STAGED> moves;
	Position::IrreversibleState state;
	Move move;
	Score score;
	int legals_cnt;
};

class TreeInfo {
public:
	INLINE TreeNodeInfo& getNode(unsigned ply) {
		assert(ply < max_depth);
		return _node[ply];
	}
private:
	std::array<TreeNodeInfo, max_depth> _node;
};

class Eval;

class Search {
public:
	void bestMove(Position& pos, unsigned depth);
private:
	void iterativeDeepening(Position& pos, unsigned depth);
	void search(Position& pos, unsigned depth, SearchResults& results);

	template <bool Root>
	Score negaMax(Position& pos, SearchResults& results, Score alpha, Score beta, unsigned depth, unsigned ply);

	Score quiesce(Position& pos, SearchResults& results, Score alpha, Score beta);

	TreeInfo _tree;
	Eval _eval;
};