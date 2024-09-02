#pragma once

#include "MoveList.hpp"
#include "MoveGen.hpp"

enum OrderType {
	PLAIN, 
	STAGED
};

/*
	MoveOrder<PLAIN>: 
	 - Plain move ordering. Generates all moves, both captures and quiets once.
	MoveOrder<STAGED>:
	 - Generates moves by moving through generation stages (first <CAPTURES>, then <QUIETS>)
*/

template <OrderType Type>
class MoveOrder {
public:
	bool nextMove(const Position& pos, Move& next_move);
private:
	bool loadMove(Move& move);

	struct Stage {
		const bool unused;
		MoveGen::enumMode mode;
	};

	static constexpr Stage _staged_first_mode = { false , MoveGen::CAPTURES },
						   _plain_mode        = { true };

	Stage _stage = Type == PLAIN ? _plain_mode : _staged_first_mode;
	MoveList _move_list;
	int _iterator = 0;
};