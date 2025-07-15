#pragma once

#include "Common.hpp"
#include "Position.hpp"
#include "Move.hpp"
#include "MoveOrder.hpp"
#include "Eval.hpp"
#include "Game.hpp"
#include "Time.hpp"
#include "Score.hpp"
#include "TranspositionTable.hpp"

#include <numeric>

struct SearchLimits {
	bool isTimeLeft();

	unsigned  depth = 0,
			  wtime = 0,
			  btime = 0;
	time_ms_t winc  = 0, 
			  binc  = 0,
			  search_time = 0;
	Timer     timer;
};

class Search;

struct SearchResults {
	void registerBestMove(Move32b move);

	void printBestMove();
	void print(const Search* search, const Position& pos);
	void printShort();

	unsigned  depth      = 0,
			  seldepth   = 0;
	Score	  score_cp   = 0;
	uint64_t  nodes_cnt  = 0;
	size_t    tt_hits    = 0,
			  tt_entries = 0;
	Move32b      best_move  = Move32b::null;
	time_ms_t duration;
};

struct NodeInfo {
	MoveOrder<STAGED>			move_picker;
	Position::IrreversibleState state;
	Move32b						move;
	Move32b						best_move;
	Score						score;
	bool						can_move;
	Score						best_score;
	bool						check;
	unsigned					ply;
	size_t						moves_searched;
};

class TreeStack {
public:
	NodeInfo* getRootNode();
	const NodeInfo* getNode(unsigned ply) const;
	void clear();
private:
	NodeInfo _stack[max_depth];
};

class Eval;
class TranspositionTable;

class Search {
public:
	friend struct SearchResults;

	enum enumNode {
		PV_NODE,
		NON_PV_NODE,
	};

	Search();

	template <bool PrintFullInfo = true>
	Move32b bestMove(Position& pos, const Game& game, SearchLimits limits);
	static Move32b _bestMove_unittest(Search& search, Position& pos, const Game& game, SearchLimits limits);

	void registerNewGame();
private:
	template <bool PrintFullInfo>
	Move32b iterativeDeepening(Position& pos, const Game& game, SearchLimits& limits);

	template <bool PrintFullInfo>
	bool search(Position& pos, const Game& game, SearchLimits& limits, SearchResults& results);

	template <bool Root, enumNode NodeType = PV_NODE, bool NullMove = !Root>
	Score negaMax(Position& pos, SearchLimits& limits, SearchResults& results, const Game& game, NodeInfo* node,
		Score alpha, Score beta, unsigned depth, unsigned ply);

	Score quiesce(Position& pos, SearchLimits& limits, SearchResults& results, Score alpha, Score beta, unsigned ply);

	int calculateExtension(Position& pos, NodeInfo* node);

	bool isRepetitionCycle(const Position& pos, const Game& game, NodeInfo* node, int ply);

	TreeStack _tree_stack;
	Eval _eval;
	TranspositionTable _tt;

	static constexpr uint64_t _check_node_count = 4096;
};

INLINE const NodeInfo* TreeStack::getNode(unsigned ply) const {
	assert(ply < max_depth);
	return _stack + ply;
}

INLINE NodeInfo* TreeStack::getRootNode() {
	return _stack;
}

INLINE void TreeStack::clear() {
	for (size_t i = 0; i < max_depth; i++) {
		_stack[i].move_picker.setKillerMove(Move32b::null);
	}
}
