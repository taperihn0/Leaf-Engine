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

class TreeStack;
struct NodeInfo;

template <OrderType Type>
class MoveOrder {
public:
	void generateMoves(const Position& pos);
	bool nextMove(const TreeStack& tree, const Position& pos, Move32b& next_move);

	void setHashMove(Move32b m);
	void setKillerMove(Move32b m);

	void updateHistory(Move32b curr, bool side, int depth);

	void clear();
	
	static void agingHistory();
	static void clearHistory();
private:
	bool nextFromList(Move32b& move);

	void scoreCaptures(size_t first, const Position& pos);
	void scoreQuiets(size_t first, const Position& pos);

	enum class enumStage : uint8_t {
		HASH_MOVE,
		CAPTURES,
		PICK_CAPTURES, 
		KILLER,
		QUIETS,
		PICK_QUIETS,
	};

	static constexpr 
	enumStage _first_stage = Type == QUIESCENT ? enumStage::CAPTURES : enumStage::HASH_MOVE;
	enumStage _stage       = _first_stage;
	size_t _iterator       = 0;

	Move32b _hash_move		   = Move32b::null;
	Move32b _killer_move	   = Move32b::null;

	inline static alignas(64) uint16_t _history[2][6][64] = {};

	MoveList _move_list;
};

template <OrderType Type>
INLINE void MoveOrder<Type>::clear() {
	_iterator = 0;
	_stage = _first_stage;
	_hash_move = Move32b::null;
	_move_list.clear();
}

template <OrderType Type>
INLINE void MoveOrder<Type>::setHashMove(Move32b m) {
	_hash_move = m;
}

template <OrderType Type>
INLINE void MoveOrder<Type>::setKillerMove(Move32b m) {
	_killer_move = m;
}

template <OrderType Type>
INLINE void MoveOrder<Type>::updateHistory(Move32b curr, bool side, int depth) {
	_history[side][curr.getPiece()][curr.getTarget()] += depth * depth;

	if (_history[side][curr.getPiece()][curr.getTarget()] > 16000) {
		agingHistory();
	}
}

template <OrderType Type>
INLINE void MoveOrder<Type>::agingHistory() {
	for (size_t i = 0; i < 2; i++) 
		for (size_t j = 0; j < 6; j++)
			for (size_t k = 0; k < 64; k++)
				_history[i][j][k] /= 2;
}

template <OrderType Type>
INLINE void MoveOrder<Type>::clearHistory() {
	alignedMemset(_history, 0, sizeof(_history));
}
