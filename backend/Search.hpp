#pragma once

#include "Common.hpp"
#include "Position.hpp"
#include "Move.hpp"

#include <numeric>

// Wrapper around raw int16_t score. Raw score stands for evaluation score expressed in 
// centipawns (for simplicity, 1 cp is around 1/100 of pawn value)
class Score {
public:
	INLINE Score() = default;
	INLINE Score(uint16_t val)
		: _raw(val) {}

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

	static constexpr int16_t draw = 0,
							 infinity = std::numeric_limits<int16_t>::max();
private:
	int16_t _raw;
};

struct SearchResults {
	INLINE SearchResults()
		: score_cp(0), nodes_cnt(0), pv_line{} {}

	INLINE void clearPV() {
		for (auto& ply_line : pv_line)
			ply_line.fill(Move::null);
	}

	INLINE void printBestMove() {
		ASSERT(!pv_line[0][0].isNull(), "Null bestmove error");

		std::cout << "bestmove ";
		pv_line[0][0].print();
		std::cout << '\n';
	}

	INLINE void print() {
		std::cout << "info score cp " << score_cp.toInt() << " nodes " << nodes_cnt
			<< " pv ";

		int it = 0;
		while (!pv_line[0][it].isNull())
			pv_line[0][it++].print(), std::cout << ' ';

		std::cout << '\n';
	}

	Score score_cp;
	uint64_t nodes_cnt;
	std::array<std::array<Move, max_depth>, max_depth> pv_line;
};

class Search {
public:
	void bestMove(Position& pos, unsigned depth);
private:
	void iterativeDeepening(Position& pos, unsigned depth);
	void search(Position& pos, unsigned depth, SearchResults& results);

	template <bool Root>
	Score negaMax(Position& pos, SearchResults& results, Score alpha, Score beta, unsigned depth, unsigned ply);
};