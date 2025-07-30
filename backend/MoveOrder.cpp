#include "MoveOrder.hpp"
#include "Position.hpp"
#include "Search.hpp"

/* 
*	MoveOrder<STAGED> and MoveOrder<QUIESCENT> template classes do not specify generateMoves function. 
*   Both generates appropiate moves on fly, during move picking as stage is 
*	moving from really promising moves to less interesting ones.
*/

template <OrderType Type, bool Root>
bool MoveOrder::nextMove(const TreeStack& tree, const Position& pos, Move32b& next_move) {
	static_assert(!Root or Type == STAGED);
	assert(_stage != enumStage::NONE);
	
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
		assert(Type != QUIESCENT);

		_stage = enumStage::QUIETS;
		_quiets_ind = _iterator;

		if (!_killer_move.isNull() and
			_killer_move != _hash_move and
			!Root and
			_killer_move.isPseudoLegal(pos))
		{
			next_move = _killer_move;
			return true;
		}

		[[fallthrough]];
	case enumStage::QUIETS:
		assert(Type != QUIESCENT);

		MoveGen::generatePseudoLegalMoves<MoveGen::QUIETS>(pos, _move_list);

		_stage = enumStage::PICK_QUIETS;

		[[fallthrough]];
	case enumStage::PICK_QUIETS:
		assert(Type != QUIESCENT);

		const enumColor side = pos.getTurn();

		scoreQuiets(_quiets_ind, side);

		return nextFromList(next_move);
	}

	return false;
}

template <int8_t Sign, OrderType Type>
void MoveOrder::updateQuietEntry(Move32b move, enumColor side, int depth) {
	static_assert(Type == STAGED);
	static_assert(Sign == -1 or Sign == 1);

	const Piece::uint_t piece = value(move.getPiece());
	const Square dst = move.getTarget();

	const int16_t bonus = std::min(sq(static_cast<int16_t>(depth)), _MaxQuietsHistory);

	_quiets_history[side][piece][dst] += Sign * bonus - (((ll)_quiets_history[side][piece][dst] * bonus) >> _MaxQuietsPower);

	assert(abs(_quiets_history[side][piece][dst]) <= _MaxQuietsHistory);
}

template <OrderType Type>
void MoveOrder::updateQuietsHistory(Move32b bestmove, enumColor side, int depth) {
	static_assert(Type == STAGED);

	assert(bestmove.isQuiet() and !bestmove.isQueenPromotion());

	updateQuietEntry<1>(bestmove, side, depth);

	for (size_t i = _quiets_ind; i < _move_list.count(); i++) {
		MoveList::Entry* entry = _move_list.getEntry(i);
		Move32b* move = &entry->move;

		assert(move->isQuiet() and !move->isQueenPromotion());

		if (*move == bestmove)
			break;

		updateQuietEntry<-1>(*move, side, depth);
	}
}

INLINE bool MoveOrder::nextFromList(Move32b& move) {
	if (_iterator >= _move_list.count()) 
		return false;

	_move_list.selectSort(_iterator);
	move = _move_list.getMove(_iterator++);

	return move == _hash_move or move == _killer_move ? nextFromList(move) : true;
}

static constexpr std::array<int16_t, 6> CaptureScore = {
	100, 300, 300, 500, 900, 10000
};

static constexpr std::array<int16_t, 5> PromotionScore = {
	0, 150, 100, 100, 900
};

void MoveOrder::scoreCaptures(size_t first_ind, const Position& pos) {
	const enumColor oppside = static_cast<enumColor>(pos.getOppositeTurn());

	for (size_t i = first_ind; i < _move_list.count(); i++) {
		MoveList::Entry* entry = _move_list.getEntry(i);
		const Move32b* move = &entry->move;
		MoveList::entryscore_t* score = &entry->score;

		assert(move->isCapture() or 
			  (move->isPromotion() and 
			   move->isQueenPromotion() and 
			  !move->isLegalMoved())); // legality not checked yet

		*score = 0;

		if (move->isEnPassant()) {
			*score = CaptureScore[Piece::PAWN] - value(Piece::PAWN);
		}
		else if (move->isCapture()) {
			const Piece::uint_t piece_ind = value(move->getPiece());
			const Piece::uint_t vic = value(pos.pieceOn(move->getTarget(), oppside));
			*score = CaptureScore[vic] - piece_ind;
		}

		if (move->isPromotion()) {
			const Piece::uint_t promo = value(move->getPromoPiece());
			*score += PromotionScore[promo];
		}
	}
}

void MoveOrder::scoreQuiets(size_t first_ind, enumColor side) {
	for (size_t i = first_ind; i < _move_list.count(); i++) {
		MoveList::Entry* entry = _move_list.getEntry(i);
		const Move32b* move = &entry->move;
		MoveList::entryscore_t* score = &entry->score;

		assert(move->isQuiet());

		const Piece::uint_t piece = value(move->getPiece());
		const Square dst = move->getTarget();

		*score = _quiets_history[side][piece][dst] + _MaxQuietsHistory;
	}
}

template bool MoveOrder::nextMove<STAGED, false>(const TreeStack&, const Position&, Move32b&);
template bool MoveOrder::nextMove<STAGED, true> (const TreeStack&, const Position&, Move32b&);
template bool MoveOrder::nextMove<QUIESCENT, false>(const TreeStack&, const Position&, Move32b&);

template void MoveOrder::updateQuietsHistory(Move32b, enumColor, int);

template void MoveOrder::updateQuietEntry<-1>(Move32b, enumColor, int);
template void MoveOrder::updateQuietEntry<1> (Move32b, enumColor, int);
