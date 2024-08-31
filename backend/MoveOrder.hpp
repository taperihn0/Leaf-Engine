#pragma once

#include "MoveList.hpp"

class MoveOrder {
public:
	bool nextMove(const Position pos, Move& next_move);
private:
	MoveList _move_list;
};