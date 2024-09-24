#pragma once

#include "Common.hpp"
#include "Position.hpp"
#include "Move.hpp"
#include "MoveOrder.hpp"
#include "Eval.hpp"
#include "Game.hpp"
#include "Time.hpp"
#include "Score.hpp"

#include <numeric>

struct SearchLimits {
	bool isTimeLeft();

	unsigned depth = 0,
			 wtime = 0, 
			 btime = 0, 
			 winc  = 0, 
			 binc  = 0,
			 search_time = 0;
	Timer    timer;
};

class Search;

struct SearchResults {
	void registerBestMove(Move move);

	void printBestMove();
	void print(const Search* search, unsigned depth, const Position& pos);

	unsigned depth      = 0,
			 seldepth   = 0;
	Score	 score_cp   = 0;
	uint64_t nodes_cnt  = 0;
	size_t   tt_hits    = 0,
			 tt_entries = 0;
	Move     best_move  = Move::null;
	Timer    timer;
};

struct NodeInfo {
	MoveOrder<STAGED> move_picker;
	Position::IrreversibleState state;
	Move move;
	Move best_move;
	Score score;
	bool can_move;
	Score best_score;
	bool check;
};

class TreeInfo {
public:
	NodeInfo& getNode(unsigned ply);
	void clear();
private:
	std::array<NodeInfo, max_depth> _node;
};

class Eval;
class TranspositionTable;

class Search {
public:
	friend struct SearchResults;

	Search(TranspositionTable&& tt);

	void bestMove(Position& pos, const Game& game, SearchLimits limits);

	INLINE TranspositionTable& getTranspositionTable() { return _tt; }
private:
	void iterativeDeepening(Position& pos, const Game& game, SearchLimits& limits);
	bool search(Position& pos, const Game& game, SearchLimits& limits, SearchResults& results);

	template <bool Root>
	Score negaMax(Position& pos, SearchLimits& limits, SearchResults& results, const Game& game,
		Score alpha, Score beta, unsigned depth, unsigned ply);

	Score quiesce(Position& pos, SearchLimits& limits, SearchResults& results, Score alpha, Score beta, unsigned ply);

	bool isRepetitionCycle(const Position& pos, const Game& game, int ply);

	TreeInfo _tree;
	Eval _eval;
	TranspositionTable& _tt;

	static constexpr uint64_t _check_node_count = 4096;
};

INLINE NodeInfo& TreeInfo::getNode(unsigned ply) {
	assert(ply < max_depth);
	return _node[ply];
}

INLINE void TreeInfo::clear() {
	for (auto& node : _node) 
		node.move_picker.setKillerMove(Move::null);
}