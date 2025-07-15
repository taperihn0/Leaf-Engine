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

/* 
	MoveOrder<STAGED> and MoveOrder<QUIESCENT> template classes do not specify generateMoves function. 
    Both generates appropiate moves on fly, during move picking as stage is 
	moving from really promising moves to less interesting ones.
*/

template <OrderType Type>
bool MoveOrder<Type>::nextMove(const TreeStack& tree, const Position& pos, Move32b& next_move) {
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

		scoreCaptures(0, pos);

		_stage = enumStage::PICK_CAPTURES;

		[[fallthrough]];
	case enumStage::PICK_CAPTURES:
		if (nextFromList(next_move))
			return true;
		
		if constexpr (Type == QUIESCENT)
			return false;

		_stage = enumStage::KILLER;

		[[fallthrough]];
	case enumStage::KILLER:
		_stage = enumStage::QUIETS;

		if (!_killer_move.isNull() and _killer_move != _hash_move and _killer_move.isPseudoLegal(pos)) {
			next_move = _killer_move;
			return true;
		}

		[[fallthrough]];
	case enumStage::QUIETS:
		MoveGen::generatePseudoLegalMoves<MoveGen::QUIETS>(pos, _move_list);

		_stage = enumStage::PICK_QUIETS;

		[[fallthrough]];
	case enumStage::PICK_QUIETS:
		scoreQuiets(_iterator, pos);
		return nextFromList(next_move);
	}

	return false;
}

template <OrderType Type>
INLINE bool MoveOrder<Type>::nextFromList(Move32b& move) {
	if (_iterator >= _move_list.count()) 
		return false;

	_move_list.selectSort(_iterator);
	move = _move_list.getMove(_iterator++);

	return move == _hash_move or move == _killer_move ? nextFromList(move) : true;
}

static constexpr std::array<int, 6> piece_value = {
	100, 300, 300, 500, 900, 10000
};

template <OrderType Type>
void MoveOrder<Type>::scoreCaptures(size_t first, const Position& pos) {
	for (size_t i = first; i < _move_list.count(); i++) {
		MoveList::Entry* entry = _move_list.getEntry(i);
		const Move32b* move = &entry->move;
		uint16_t* score = &entry->score;

		assert(move->isCapture() or (move->isPromotion()
			and move->getPromoPiece() == Piece::QUEEN
			and !move->isLegalMoved()));

		if (move->isEnPassant()) {
			*score = piece_value[Piece::PAWN] - value(Piece::PAWN);
		}
		else if (move->isCapture()) {
			const Piece::enumType att = move->getPiece();
			const Piece::enumType vic = pos.pieceOn(move->getTarget(), pos.getOppositeTurn());
			*score = piece_value[vic] - value(att);
		}

		if (move->isPromotion()) {
			const Piece::enumType promo = move->getPromoPiece();
			*score += piece_value[promo];
		}
	}
}

template <OrderType Type>
void MoveOrder<Type>::scoreQuiets(size_t first, const Position& pos) {
	for (size_t i = first; i < _move_list.count(); i++) {
		MoveList::Entry* entry = _move_list.getEntry(i);
		Move32b* move = &entry->move;
		uint16_t* score = &entry->score;

		assert(move->isQuiet() and !move->isLegalMoved());

		*score = _history[pos.getTurn()][move->getPiece()][move->getTarget()];
	}
}

template bool MoveOrder<STAGED>::nextMove(const TreeStack&, const Position&, Move32b&);
template bool MoveOrder<QUIESCENT>::nextMove(const TreeStack&, const Position&, Move32b&);
