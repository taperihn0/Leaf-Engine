#pragma once

#include "MoveList.hpp"
#include "MoveGen.hpp"
#include "Memory.hpp"

class TreeStack;
class MoveOrder;

/* It is basically a part of MoveOrder interface.
*  It contains tables used in move ordering with history data, for instance 
*  piece-square or from-to tables.
*  It implements differentiation of history data between each Search object,
*  as it is part of Search class.
*/

class MoveOrderHistoryTables {
public:
	friend class MoveOrder;

	MoveOrderHistoryTables(); 

	void clearQuietsHistory();
	// ...
private:
	int16_t _quiets_history[2][6][64];
	// ...
};

enum OrderType {
	STAGED,
	QUIESCENT
};

/*
*	MoveOrder<STAGED>:
*	 - Generates moves by moving through generation stages (first <CAPTURES>, then <QUIETS>)
*	MoveOrder<QUIESCE>:
*	 - Generates only captures in quiescent node.
*/

inline _P_CONSTEXPR int MaxQuietsHistoryPow = 13;
inline _P_CONSTEXPR int MaxQuietsHistory    = 1 << MaxQuietsHistoryPow;

/* MVV-LVA captures and promotion scores 
*/

inline constexpr    int PawnCapturedScore   = 100;
inline _P_CONSTEXPR int KnightCapturedScore = 300;
inline _P_CONSTEXPR int BishopCapturedScore = 300;
inline constexpr    int RookCapturedScore   = 500;
inline constexpr    int QueenCapturedScore  = 900;

inline _P_CONSTEXPR int ToKnightPromoScore  = 150;
inline _P_CONSTEXPR int ToBishopPromoScore  = 100;
inline _P_CONSTEXPR int ToRookPromoScore    = 100;
inline _P_CONSTEXPR int ToQueenPromoScore   = 900;

class MoveOrder {
public:
	MoveOrder(MoveOrderHistoryTables* history_tables = nullptr);

	void setHistoryBuffer(MoveOrderHistoryTables* history_tables);

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

	static_assert(_IS_SAME_TYPE(MoveList::entryscore_t, int16_t) or
				  _IS_SAME_TYPE(MoveList::entryscore_t, int32_t));

	static constexpr enumStage _FirstStage = enumStage::HASH_MOVE;

	MoveOrderHistoryTables* _tables;

	enumStage _stage       = enumStage::NONE;
	size_t _iterator       = 0;
	size_t _quiets_ind	   = 0;

	Move32b _hash_move	   = Move32b::Null;
	Move32b _killer_move   = Move32b::Null;

	MoveList _move_list;
};

INLINE void MoveOrderHistoryTables::clearQuietsHistory() {
	alignedMemset(_quiets_history, 0, sizeof(_quiets_history));
}

INLINE void MoveOrder::setHistoryBuffer(MoveOrderHistoryTables* history_tables) {
	_tables = history_tables;
}

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
	const Piece::uint_t piece_ind = value(move.getPiece());
	const Square dst = move.getTarget();
	return _tables->_quiets_history[side][piece_ind][dst];
}
