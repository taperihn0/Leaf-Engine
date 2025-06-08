#include "MoveOrder.hpp"
#include "Position.hpp"
#include "Search.hpp"

template <>
void MoveOrder<PLAIN>::generateMoves(const Position& pos) {
	_iterator = 0;
	_move_list.clear();
	MoveGen::generatePseudoLegalMoves<MoveGen::CAPTURES>(pos, _move_list);
	MoveGen::generatePseudoLegalMoves<MoveGen::QUIETS>(pos, _move_list);
}

template <>
bool MoveOrder<PLAIN>::nextMove(const TreeInfo&, const NodeInfo&, const Position&, Move& next_move) {
	return getFromList(next_move);
}

/* 
	MoveOrder<STAGED> and MoveOrder<QUIESCENT> template classes do not specify generateMoves function. 
    Both generates appropiate moves on fly, during move picking as stage is 
	moving from really promising moves to less interesting ones.
*/

template <OrderType Type>
bool MoveOrder<Type>::nextMove(const TreeInfo& tree, const NodeInfo& node, const Position& pos, Move& next_move) {
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

		_move_list.scoreCaptures(0, pos);

		_stage = enumStage::PICK_CAPTURES;

		[[fallthrough]];
	case enumStage::PICK_CAPTURES:
		if (getFromListCapture(next_move))
			return true;
		
		if constexpr (Type == QUIESCENT)
			return false;

		_stage = enumStage::KILLER;

		[[fallthrough]];
	case enumStage::KILLER:
		_stage = enumStage::COUNTERMOVE;

		if (!_killer_move.isNull() and _killer_move != _hash_move and _killer_move.isPseudoLegal(pos)) {
			next_move = _killer_move;
			return true;
		}

		[[fallthrough]];
	case enumStage::COUNTERMOVE:
		_stage = enumStage::QUIETS;

		if (node.ply > 0) {
			const Move prev = tree.getNode(node.ply - 1).move;
			_counter = _countermove[pos.getOppositeTurn()][prev.getPerformerT()][prev.getTarget()];

			if (!_counter.isNull() and _counter != _hash_move
				and _counter != _killer_move and _counter.isPseudoLegal(pos)) {
				next_move = _counter;
				return true;
			}
		}

		[[fallthrough]];
	case enumStage::QUIETS:
		MoveGen::generatePseudoLegalMoves<MoveGen::QUIETS>(pos, _move_list);
		_stage = enumStage::PICK_QUIETS;

		[[fallthrough]];
	case enumStage::PICK_QUIETS:
		return getFromListQuiet(next_move);
	}

	return false;
}

template bool MoveOrder<STAGED>::nextMove(const TreeInfo&, const NodeInfo&, const Position&, Move&);
template bool MoveOrder<QUIESCENT>::nextMove(const TreeInfo&, const NodeInfo&, const Position&, Move&);

template <OrderType Type>
INLINE bool MoveOrder<Type>::getFromList(Move& move) {
	return getFromListQuiet(move);
}

template <OrderType Type>
INLINE bool MoveOrder<Type>::getFromListQuiet(Move& move) {
	if (_iterator >= _move_list.count()) 
		return false;

	move = _move_list.getMove(_iterator++);
	return move == _hash_move or move == _killer_move or move == _counter ? getFromList(move) : true;
}

template <OrderType Type>
template <bool ExcludeHashMove>
INLINE bool MoveOrder<Type>::getFromListCapture(Move& move) {
	if (_iterator >= _move_list.count())
		return false;

	_move_list.partialSort(_iterator, _iterator + 1, _move_list.count());
	move = _move_list.getMove(_iterator++);

	if constexpr (ExcludeHashMove)
		return move == _hash_move ? getFromListCapture<false>(move) : true;
	if constexpr (!ExcludeHashMove)
		return getFromListCapture<false>(move);
}