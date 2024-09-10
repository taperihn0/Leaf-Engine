#include "MoveOrder.hpp"
#include "Position.hpp"

template <OrderType Type>
bool MoveOrder<Type>::loadMove(Move& move) {
	if (_iterator >= _move_list.count()) return false;
	move = _move_list.getMove(_iterator++);
	return true;
}

bool MoveOrder<PLAIN>::nextMove(const Position& pos, Move& next_move) {
	if (!_iterator) {
		MoveGen::generatePseudoLegalMoves<MoveGen::CAPTURES>(pos, _move_list);
		MoveGen::generatePseudoLegalMoves<MoveGen::QUIETS>(pos, _move_list);
	}

	return loadMove(next_move);
}