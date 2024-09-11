#include "MoveOrder.hpp"
#include "Position.hpp"

void MoveOrder<PLAIN>::generateMoves(const Position& pos) {
	_iterator = 0;
	MoveGen::generatePseudoLegalMoves<MoveGen::CAPTURES>(pos, _move_list);
	MoveGen::generatePseudoLegalMoves<MoveGen::QUIETS>(pos, _move_list);
}

/* 
	MoveOrder<STAGED> template class does not specify generateMoves function. 
    It generates appropiate moves on fly, during move picking as stage as is 
	moving from really promising captures to less promising quiets moves in terms of 
    possibility of beta cuttoff 
*/

void MoveOrder<QUIESCENT>::generateMoves(const Position& pos) {
	_iterator = 0;
	MoveGen::generatePseudoLegalMoves<MoveGen::CAPTURES>(pos, _move_list);
}

template <OrderType Type>
bool MoveOrder<Type>::nextMove(const Position& _, Move& next_move) {
	return loadMove(next_move);
}

template bool MoveOrder<PLAIN>::nextMove(const Position& _, Move& next_move);
template bool MoveOrder<QUIESCENT>::nextMove(const Position& _, Move& next_move);

/* TODO: STAGED move picker */
bool MoveOrder<STAGED>::nextMove(const Position& pos, Move& next_move) {
	return false;
}

template <OrderType Type>
INLINE bool MoveOrder<Type>::loadMove(Move& move) {
	if (_iterator >= _move_list.count()) return false;
	move = _move_list.getMove(_iterator++);
	return true;
}