#pragma once

#include "MoveOrder.hpp"

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