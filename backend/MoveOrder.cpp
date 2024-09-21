#include "MoveOrder.hpp"
#include "Position.hpp"

void MoveOrder<PLAIN>::generateMoves(const Position& pos) {
	_iterator = 0;
	_move_list.clear();
	MoveGen::generatePseudoLegalMoves<MoveGen::CAPTURES>(pos, _move_list);
	MoveGen::generatePseudoLegalMoves<MoveGen::QUIETS>(pos, _move_list);
}

bool MoveOrder<PLAIN>::nextMove(const Position& _, Move& next_move) {
	return getFromList(next_move);
}

/* 
	MoveOrder<STAGED> and MoveOrder<QUIESCENT> template classes do not specify generateMoves function. 
    Both generates appropiate moves on fly, during move picking as stage is 
	moving from really promising moves to less interesting ones.
*/

template <OrderType Type>
bool MoveOrder<Type>::nextMove(const Position& pos, Move& next_move) {
	switch (_stage) {
	case enumStage::HASH_MOVE:
		_stage = enumStage::CAPTURES;

		if (!_hash_move.isNull()) {
			next_move = _hash_move;
			return true;
		}

		[[fallthrough]];
	case enumStage::CAPTURES:
		MoveGen::generatePseudoLegalMoves<MoveGen::CAPTURES>(pos, _move_list);

		// TODO: prefer partial sort over normal sort
		_move_list.scoreCaptures(0, pos);
		_move_list.sort(0, _move_list.count());

		_stage = enumStage::PICK_CAPTURES;

		[[fallthrough]];
	case enumStage::PICK_CAPTURES:
		if (getFromList(next_move))
			return true;
		
		if constexpr (Type == QUIESCENT)
			return false;

		_stage = enumStage::QUIETS;

		[[fallthrough]];
	case enumStage::QUIETS:
		MoveGen::generatePseudoLegalMoves<MoveGen::QUIETS>(pos, _move_list);
		_stage = enumStage::PICK_QUIETS;

		[[fallthrough]];
	case enumStage::PICK_QUIETS:
		if (getFromList(next_move))
			return true;
	}

	return false;
}

template bool MoveOrder<STAGED>::nextMove(const Position& _, Move& next_move);
template bool MoveOrder<QUIESCENT>::nextMove(const Position& _, Move& next_move);

template <OrderType Type>
INLINE bool MoveOrder<Type>::getFromList(Move& move) {
	if (_iterator >= _move_list.count()) 
		return false;

	move = _move_list.getMove(_iterator++);
	return move == _hash_move ? getFromList(move) : true;
}