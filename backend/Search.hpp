#pragma once

#include "Common.hpp"
#include "Position.hpp"
#include "Move.hpp"
#include "MoveOrder.hpp"
#include "Eval.hpp"
#include "Game.hpp"
#include "Time.hpp"

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

	INLINE Score operator+=(Score b) {
		return _raw += b._raw;
	}

	INLINE Score operator-=(Score b) {
		return _raw -= b._raw;
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

	INLINE bool operator<=(Score b) const {
		return _raw <= b._raw;
	}

	INLINE bool operator==(Score b) const {
		return _raw == b._raw;
	}

	INLINE bool operator!=(Score b) const {
		return _raw != b._raw;
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

	INLINE bool isValid() const {
		return _raw != undef;
	}

	std::string toStr() const;

	static constexpr int16_t draw = 0,
							 infinity = std::numeric_limits<int16_t>::max(),
							 undef = infinity;
private:
	int16_t _raw;
};

struct SearchLimits {
	bool isTimeLeft();

	unsigned depth = 0,
			 wtime = 0, 
			 btime = 0, 
			 winc  = 0, 
			 binc  = 0,
			 search_time = 0;

	Timer timer;
};

struct SearchResults {
	void registerBestMove();

	void clearPV();

	void printBestMove();
	void print();

	unsigned depth     = 0;
	Score score_cp     = 0;
	uint64_t nodes_cnt = 0;
	Move bestmove      = Move::null;

	std::array<std::array<Move, max_depth>, max_depth> pv_line = {};
	Timer timer;
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
class TranspositionTable;

class Search {
public:
	Search(TranspositionTable&& tt);

	void bestMove(Position& pos, const Game& game, SearchLimits limits);

	INLINE TranspositionTable& getTranspositionTable() { return _tt; }
private:
	void iterativeDeepening(Position& pos, const Game& game, SearchLimits& limits);
	bool search(Position& pos, const Game& game, SearchLimits& limits, SearchResults& results);

	template <bool Root>
	Score negaMax(Position& pos, SearchLimits& limits, SearchResults& results, const Game& game,
		Score alpha, Score beta, unsigned depth, unsigned ply);

	Score quiesce(Position& pos, SearchLimits& limits, SearchResults& results, Score alpha, Score beta);

	bool isRepetitionCycle(const Position& pos, const Game& game, int ply);

	TreeInfo _tree;
	Eval _eval;
	TranspositionTable& _tt;

	static constexpr uint64_t _check_node_count = 4096;
};