#pragma once

#include "MoveList.hpp"
#include "MoveGen.hpp"

/*
	MoveOrder<PLAIN>: 
	 - Plain move ordering. Generates all moves, both captures and quiets once.
	MoveOrder<STAGED>:
	 - Generates moves by moving through generation stages (first <CAPTURES>, then <QUIETS>)
	MoveOrder<QUIESCE>
	 - Generates only captures in quiescent node.
*/

enum OrderType {
	PLAIN,
	STAGED,
	QUIESCENT
};

template <OrderType Type>
class MoveOrder {
public:
	void generateMoves(const Position& pos);
	bool nextMove(const Position& pos, Move& next_move);

	void setHashMove(Move m);

	void clear();
private:
	bool getFromList(Move& move);

	enum class enumStage : uint8_t {
		HASH_MOVE,
		CAPTURES,
		PICK_CAPTURES, 
		QUIETS,
		PICK_QUIETS
	};

	static constexpr 
	enumStage _first_stage = enumStage::HASH_MOVE;
	enumStage _stage       = _first_stage;
	size_t _iterator       = 0;
	Move _hash_move		   = Move::null;

	MoveList _move_list;
};

template <OrderType Type>
INLINE void MoveOrder<Type>::clear() {
	_iterator = 0;
	_stage = _first_stage;
	_hash_move = Move::null;
	_move_list.clear();
}

template <OrderType Type>
INLINE void MoveOrder<Type>::setHashMove(Move m) {
	_hash_move = m;
}