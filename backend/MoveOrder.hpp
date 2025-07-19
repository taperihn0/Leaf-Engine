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
struct NodeInfo;

template <OrderType Type>
class MoveOrder {
public:
	template <bool Root>
	bool nextMove(const TreeStack& tree, const Position& pos, Move32b& next_move);

	void setHashMove(Move32b m);
	void setKillerMove(Move32b m);

	Move32b getKillerMove();

	template <int8_t Sign>
	void updateQuietsHistory(Move32b move, enumColor side, int depth);

	template <bool All>
	void applyQuietsMaluses(Move32b bestmove, enumColor side, int depth);

	static void clearQuietsHistory();

	void clear();
private:
	bool nextFromList(Move32b& move);

	void scoreCaptures(size_t first_ind, const Position& pos);
	void scoreQuiets(size_t first_ind, enumColor side);

	enum class enumStage : uint8_t {
		HASH_MOVE,
		CAPTURES,
		PICK_CAPTURES, 
		KILLER,
		QUIETS,
		PICK_QUIETS,
	};

	static constexpr enumStage _FirstStage = Type == QUIESCENT ? enumStage::CAPTURES :
																 enumStage::HASH_MOVE;

	enumStage _stage       = _FirstStage;
	size_t _iterator       = 0;
	size_t _quiets_ind	   = 0;

	Move32b _hash_move	   = Move32b::Null;
	Move32b _killer_move   = Move32b::Null;
	
	static constexpr int16_t _MaxQuietsPower = 12;
	static constexpr int16_t _MaxQuietsHistory = 4096;

	inline static alignas(64) int16_t _quiets_history[2][6][64] = {};

	static_assert(_IS_SAME_TYPE(MoveList::entryscore_t, int16_t));

	MoveList _move_list;
};

template <OrderType Type>
INLINE void MoveOrder<Type>::clear() {
	_iterator = 0;
	_stage = _FirstStage;
	_hash_move = Move32b::Null;
	_move_list.clear();
}

template <OrderType Type>
INLINE void MoveOrder<Type>::setHashMove(Move32b m) {
	static_assert(Type == STAGED);
	_hash_move = m;
}

template <OrderType Type>
INLINE void MoveOrder<Type>::setKillerMove(Move32b m) {
	static_assert(Type == STAGED);
	_killer_move = m;
}

template <OrderType Type>
INLINE Move32b MoveOrder<Type>::getKillerMove() {
	static_assert(Type == STAGED);
	return _killer_move;
}

template <OrderType Type>
INLINE void MoveOrder<Type>::clearQuietsHistory() {
	static_assert(Type == STAGED);
	alignedMemset(_quiets_history, 0, sizeof(_quiets_history));
}
