#pragma once

#include "Common.hpp"
#include "Position.hpp"
#include "Move.hpp"

#include <numeric>

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

class Search {
public:
	void bestMove(Position& pos, unsigned depth);
private:
	void iterativeDeepening(Position& pos, unsigned depth);
	void search(Position& pos, unsigned depth);
	Score negaMax(Position& pos, Score alpha, Score beta, unsigned depth, unsigned ply);

	INLINE void clearPV() {
		for (auto& ply_line : _pv_line)
			ply_line.fill(Move::null);
	}

	std::array<std::array<Move, max_depth>, max_depth> _pv_line;
};