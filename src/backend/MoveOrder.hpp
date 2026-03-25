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

/*
*   Tunable parameters in move ordering.
*/

inline _P_CONSTEXPR int QuietMoveScoreReductionRate = 13;
inline _P_CONSTEXPR int QuietMoveScoreReductionDiv = 5;
inline _P_CONSTEXPR int CaptureMoveScoreReductionDiv = 63;

inline constexpr    int PawnCapturedScore   = 100;
inline _P_CONSTEXPR int KnightCapturedScore = 281;
inline _P_CONSTEXPR int BishopCapturedScore = 322;
inline constexpr    int RookCapturedScore   = 500;
inline constexpr    int QueenCapturedScore  = 900;
inline _P_CONSTEXPR int ToKnightPromoScore  = 127;
inline _P_CONSTEXPR int ToBishopPromoScore  = 58;
inline _P_CONSTEXPR int ToRookPromoScore    = 155;
inline _P_CONSTEXPR int ToQueenPromoScore   = 920;

class MoveOrder {
public:
	MoveOrder(MoveOrderHistoryTables* history_tables = nullptr);

	void setHistoryBuffer(MoveOrderHistoryTables* history_tables);

	template <OrderType Order, bool Root>
	bool nextMove(const NodeInfo* node, 
				  const Position& pos, 
				  Move32b& next_move,
				  int16_t& move_score);

	void setHashMove(Move32b m);

	template <OrderType Type = STAGED>
	void setKillerMove(Move32b m, uint64_t parent_hash);

	template <OrderType Type = STAGED>
	Move32b getKillerMove(uint64_t& killer_move_parent_hash);

	template <int8_t Sign, OrderType Order = STAGED>
	void updateQuietEntry(Move32b move, enumColor side, int depth);

	template <OrderType Order = STAGED>
	void updateQuietsHistory(Move32b bestmove, enumColor side, int depth);
	
	template <OrderType Order>
	void clear();

	void skipQuiets();

	int16_t getQuietScore(Move32b move, enumColor side);

	static float getQuietDepthReduction(int16_t quiet_score);
	static float getCaptureDepthReduction(int16_t capture_score);
private:
	bool nextFromList(Move32b& move, int16_t& score);

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

	Move32b	 _hash_move	   = Move32b::Null;
	Move32b	 _killer_move  = Move32b::Null;
	uint64_t _killer_move_parent_hash = 0;

	MoveList _move_list;
};

_INLINE void MoveOrderHistoryTables::clearQuietsHistory() {
	alignedMemset(_quiets_history, 0, sizeof(_quiets_history));
}

_INLINE void MoveOrder::setHistoryBuffer(MoveOrderHistoryTables* history_tables) {
	_tables = history_tables;
}

_INLINE void MoveOrder::setHashMove(Move32b m) {
	_hash_move = m;
}

template <OrderType Type>
_INLINE void MoveOrder::setKillerMove(Move32b m, uint64_t parent_hash) {
	static_assert(Type == STAGED);
	_killer_move = m;
	_killer_move_parent_hash = parent_hash;
}

template <OrderType Type>
_INLINE Move32b MoveOrder::getKillerMove(uint64_t& killer_move_parent_hash) {
	static_assert(Type == STAGED);
	killer_move_parent_hash = _killer_move_parent_hash;
	return _killer_move;
}

template <OrderType Type>
_INLINE void MoveOrder::clear() {
	_stage = _FirstStage;
	_iterator = 0;
	_quiets_ind = 0;
	_hash_move = Move32b::Null;

	if constexpr (Type == QUIESCENT)
		_killer_move = Move32b::Null;

	_move_list.clear();
}

_INLINE void MoveOrder::skipQuiets() {
	_iterator = _move_list.count();
}

_INLINE int16_t MoveOrder::getQuietScore(Move32b move, enumColor side) {
	const Piece::uint_t piece_ind = value(move.getPiece());
	const Square dst = move.getTarget();
	return _tables->_quiets_history[side][piece_ind][dst];
}

_FORCEINLINE float MoveOrder::getQuietDepthReduction(int16_t quiet_score) {
	const int16_t centered_score = quiet_score - MaxQuietsHistory;
	const float rt = std::sqrt(static_cast<float>(std::abs(centered_score)));
	const float val = QuietMoveScoreReductionRate * rt / QuietMoveScoreReductionDiv;
	return centered_score < 0 ? val : -val;
}

_FORCEINLINE float MoveOrder::getCaptureDepthReduction(int16_t capture_score) {
	return static_cast<float>(capture_score / CaptureMoveScoreReductionDiv);
}
