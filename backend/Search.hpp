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

	unsigned  depth		  = 0,
			  wtime		  = 0,
			  btime		  = 0;
	time_ms_t winc		  = 0, 
			  binc		  = 0,
			  search_time = 0;
	Timer     timer;
};

class Search;

struct SearchResults {
	void registerBestMove(Move32b move);

	void printBestMove();
	void print(const Search* search, const Position& pos, TranspositionTable& tt);
	void printShort();

	unsigned  depth      = 0,
			  seldepth   = 0;
	Score	  score_cp   = 0;
	uint64_t  nodes_cnt  = 0;
	size_t	  tt_entries = 0;
	Move32b   best_move  = Move32b::Null;
	time_ms_t duration;
};

struct NodeInfo {
	MoveOrder					move_picker;
	Position::IrreversibleState state;
	Move32b						move;
	Move32b						best_move;
	Score						score;
	bool						can_move;
	Score						best_score;
	bool						check;
	unsigned					ply;
	uint8_t						moves_searched;
	Score					    static_eval;
	TTEntry::Bound				bound;
};

class TreeStack {
public:
	TreeStack();
	~TreeStack();

	NodeInfo* getRootNode();
	const NodeInfo* getNode(unsigned ply) const;
private:
	static constexpr size_t _Count = MaxSelDepth;
	NodeInfo* _stack;
};

class Eval;
class TranspositionTable;

class Search {
public:
	friend struct SearchResults;

	enum enumNode : int8_t {
		PV_NODE = 1,
		NON_PV_NODE = 2,
		SEARCH_NODE = PV_NODE | NON_PV_NODE,
		QUIESCE_NODE = ~SEARCH_NODE,
	};

	Search() = default;

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
				  Score alpha, Score beta, int depth, int ply);

	Score quiesce(Position& pos, SearchLimits& limits, SearchResults& results, NodeInfo* node, 
				  Score alpha, Score beta, int depth, int ply);

	int calculateExtension(Position& pos, NodeInfo* node);

	bool isRepetitionCycle(const Position& pos, const Game& game, NodeInfo* node, int ply);

	TreeStack _tree_stack;
	Eval _eval;
	TranspositionTable _tt;

	static constexpr uint64_t _CheckNodeCount = 4096;
};
