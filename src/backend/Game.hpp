#pragma once

#include "Common.hpp"
#include "Move.hpp"
#include "Hash.hpp"
#include "Time.hpp"

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

	INLINE Move32b getPrevMove(size_t halfmove_cnt) const {
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

/* Contains full game info - move record, time left and current position of the game.
*  Can detect game termination, either draw or win (lose).
*/
class Game {
public:
	enum GameResult {
		DRAW, WIN_OR_LOSE
	};

	Game(Position&& from, bool time_constraint, time_ms_t time_white = 0, time_ms_t time_black = 0);

	void applyMove(Move32b move, time_ms_t think_time);

	bool isWin();
	bool isDraw();

	Position& getPosition();

	FullInfoRecord& getHistoryRecord();
private:
	bool isGameCycle();
	bool isStaleMate();

	bool isAnyResponse();

	Position _current_pos;
	FullInfoRecord _pos_record;
	time_ms_t _time_left_sided[2];
	bool _time_constraint;
	
	// Cache game state needed when asking for game result
	bool _is_cached_any_response;
	bool _any_response_avaible;
};

