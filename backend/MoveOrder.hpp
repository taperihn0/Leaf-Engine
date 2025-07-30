#pragma once

#include "MoveList.hpp"
#include "MoveGen.hpp"

/*
*	MoveOrder<STAGED>:
*	 - Generates moves by moving through generation stages (first <CAPTURES>, then <QUIETS>)
*	MoveOrder<QUIESCE>
*	 - Generates only captures in quiescent node.
*/

enum OrderType {
	STAGED,
	QUIESCENT
};

class TreeStack;

class MoveOrder {
public:
	MoveOrder() = default;

	template <OrderType Order, bool Root>
	bool nextMove(const TreeStack& tree, const Position& pos, Move32b& next_move);

	void setHashMove(Move32b m);

	template <OrderType Type = STAGED>
	void setKillerMove(Move32b m);

	template <OrderType Type = STAGED>
	Move32b getKillerMove();

	template <int8_t Sign, OrderType Order = STAGED>
	void updateQuietEntry(Move32b move, enumColor side, int depth);

	template <OrderType Order = STAGED>
	void updateQuietsHistory(Move32b bestmove, enumColor side, int depth);

	static void clearQuietsHistory();
	
	template <OrderType Order>
	void clear();

	void skipQuiets();

	int16_t getQuietScore(Move32b move, enumColor side);
private:
	bool nextFromList(Move32b& move);

	void scoreCaptures(size_t first_ind, const Position& pos);
	void scoreQuiets(size_t first_ind, enumColor side);

	enum class enumStage : uint8_t {
		NONE,
		HASH_MOVE,
		CAPTURES,
		PICK_CAPTURES, 
		KILLER,
		QUIETS,
		PICK_QUIETS,
	};

	inline static alignas(CACHELINE_SIZE) int16_t _quiets_history[2][6][64] = {};

	static constexpr enumStage _FirstStage = enumStage::HASH_MOVE;

	enumStage _stage       = enumStage::NONE;
	size_t _iterator       = 0;
	size_t _quiets_ind	   = 0;

	Move32b _hash_move	   = Move32b::Null;
	Move32b _killer_move   = Move32b::Null;
	
	static constexpr int16_t _MaxQuietsPower = 12;
	static constexpr int16_t _MaxQuietsHistory = 4096;

	static_assert(_IS_SAME_TYPE(MoveList::entryscore_t, int16_t));

	MoveList _move_list;
};

INLINE void MoveOrder::setHashMove(Move32b m) {
	_hash_move = m;
}

template <OrderType Type>
INLINE void MoveOrder::setKillerMove(Move32b m) {
	static_assert(Type == STAGED);
	_killer_move = m;
}

template <OrderType Type>
INLINE Move32b MoveOrder::getKillerMove() {
	static_assert(Type == STAGED);
	return _killer_move;
}

INLINE void MoveOrder::clearQuietsHistory() {
	alignedMemset(_quiets_history, 0, sizeof(_quiets_history));
}

template <OrderType Type>
INLINE void MoveOrder::clear() {
	_stage = _FirstStage;
	_iterator = 0;
	_quiets_ind = 0;
	_hash_move = Move32b::Null;

	if constexpr (Type == QUIESCENT)
		_killer_move = Move32b::Null;

	_move_list.clear();
}

INLINE void MoveOrder::skipQuiets() {
	_iterator = _move_list.count();
}

INLINE int16_t MoveOrder::getQuietScore(Move32b move, enumColor side) {
	return _quiets_history[side][move.getPiece()][move.getTarget()];
}
