#pragma once

#include "Common.hpp"
#include "Move.hpp"
#include "Hash.hpp"

class MoveRecord
{
public:
	MoveRecord() = default;

	INLINE void recordMove(Move32b move)
	{
		assert(_idx < MaxGameMoves);
		_move_history[_idx++] = move;
	}

	INLINE Move32b getPrevMove(size_t halfmove_cnt) const
	{
		assert(halfmove_cnt < _idx);
		return _move_history[halfmove_cnt];
	}

	INLINE size_t currentHalfCount() const { return _idx; }
	INLINE void clear() 				   { _idx = 0; }
protected:
	std::array<Move32b, MaxGameMoves> _move_history;
	size_t _idx = 0;
};

class FullInfoRecord : MoveRecord {
public:
	FullInfoRecord() = default;

	INLINE void recordInfo(uint64_t key, Move32b move) {
		assert(_idx < MaxGameMoves);
		_key_history[_idx] = key;
		// MoveRecord takes care of shifting _idx by one
		MoveRecord::recordMove(move);
	}

	INLINE Move32b getPrevMove(size_t halfmove_cnt) const
	{
		return MoveRecord::getPrevMove(halfmove_cnt);
	}

	INLINE uint64_t getPrevKey(size_t halfmove_cnt) const {
		assert(halfmove_cnt < _idx);
		return _key_history[halfmove_cnt];
	}

	INLINE size_t currentHalfCount() const { 
		return MoveRecord::currentHalfCount(); 
	}

	INLINE void clear() { 
		MoveRecord::clear(); 
	}
private:
	std::array<uint64_t, MaxGameMoves> _key_history;
};
