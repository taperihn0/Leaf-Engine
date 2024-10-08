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

class TreeInfo;
struct NodeInfo;

template <OrderType Type>
class MoveOrder {
public:
	void generateMoves(const Position& pos);
	bool nextMove(const TreeInfo& tree, const NodeInfo& node, const Position& pos, Move& next_move);

	void setHashMove(Move m);
	void setKillerMove(Move m);
	void setCounterMove(Move prev, Move curr, bool side);

	void clear();

	static void clearCounterMoveHistory();
private:
	bool getFromList(Move& move);

	enum class enumStage : uint8_t {
		HASH_MOVE,
		CAPTURES,
		PICK_CAPTURES, 
		KILLER,
		COUNTERMOVE,
		QUIETS,
		PICK_QUIETS,
	};

	static constexpr 
	enumStage _first_stage = Type == QUIESCENT ? enumStage::CAPTURES : enumStage::HASH_MOVE;
	enumStage _stage       = _first_stage;
	size_t _iterator       = 0;

	Move _hash_move		   = Move::null;
	Move _killer_move	   = Move::null;
	Move _counter		   = Move::null;

	inline static Move _countermove[2][6][64] = {};

	MoveList _move_list;
};

template <OrderType Type>
INLINE void MoveOrder<Type>::clear() {
	_iterator = 0;
	_stage = _first_stage;
	_hash_move = Move::null;
	_counter = Move::null;
	_move_list.clear();
}

template <OrderType Type>
INLINE void MoveOrder<Type>::setHashMove(Move m) {
	_hash_move = m;
}

template <OrderType Type>
INLINE void MoveOrder<Type>::setKillerMove(Move m) {
	_killer_move = m;
}

template <OrderType Type>
INLINE void MoveOrder<Type>::setCounterMove(Move prev, Move curr, bool side) {
	_countermove[side][prev.getPerformerT()][prev.getTarget()] = curr;
}

template <OrderType Type>
INLINE void MoveOrder<Type>::clearCounterMoveHistory() {
	std::memset(_countermove, Move::null, sizeof(_countermove) / sizeof(Move));
}