#pragma once

#include "MoveList.hpp"
#include "MoveGen.hpp"

enum OrderType {
	PLAIN, 
	STAGED,
	QUIESCENT
};

/*
	MoveOrder<PLAIN>: 
	 - Plain move ordering. Generates all moves, both captures and quiets once.
	MoveOrder<STAGED>:
	 - Generates moves by moving through generation stages (first <CAPTURES>, then <QUIETS>)
	MoveOrder<QUIESCE>
	 - Generates only captures in quiescent node.
*/

template <OrderType Type>
class MoveOrder {
public:
	void generateMoves(const Position& pos);
	bool nextMove(const Position& pos, Move& next_move);

	INLINE void clear() { 
		_iterator = 0, _stage = _first_stage, _move_list.clear();
	}
private:
	bool loadMove(Move& move);

	static constexpr MoveGen::enumMode _first_stage = MoveGen::CAPTURES;

	MoveGen::enumMode _stage = _first_stage;
	MoveList _move_list;
	int _iterator = 0;
};