#pragma once

#include "Common.hpp"
#include "Move.hpp"
#include "Hash.hpp"
#include "Time.hpp"

class MoveRecord {
public:
	MoveRecord() = default;

	_INLINE void recordMove(Move32b move) {
		assert(_idx < MaxGameMoves);
		_move_history[_idx++] = move;
	}

	_INLINE Move32b getPrevMove(size_t halfmove_cnt) const {
		assert(halfmove_cnt < _idx);
		return _move_history[halfmove_cnt];
	}

	_INLINE Move32b getCurrentMove() const {
		ASSERT(_idx > 0, "No moves performed during a game");
		return getPrevMove(_idx - 1);
	}

	_INLINE size_t getMoveCount() const { return _idx; }
	_INLINE void clear() 				   { _idx = 0; }
private:
	std::array<Move32b, MaxGameMoves> _move_history;
	size_t _idx = 0;
};

class FullInfoRecord : public MoveRecord {
public:
	FullInfoRecord() = default;

	_INLINE void recordInfo(uint64_t key, Move32b move) {
		size_t curr_idx = getMoveCount();
		assert(curr_idx < MaxGameMoves);
		_key_history[curr_idx] = key;
		// MoveRecord takes care of shifting _idx by one
		MoveRecord::recordMove(move);
	}

	_INLINE uint64_t getPrevKey(size_t halfmove_cnt) const {
		assert(halfmove_cnt < getMoveCount());
		return _key_history[halfmove_cnt];
	}
private:
	std::array<uint64_t, MaxGameMoves> _key_history;
};

/* Contains full game info - move record, time left and current position of the game.
*  Can detect game termination, either draw or win (lose).
*/
class Game {
public:
    enum Result {
		GAME_INVALID,
        WHITE_WIN_BY_MATE,
        BLACK_WIN_BY_MATE,
        WHITE_WIN_BY_ADJUCATION,
        BLACK_WIN_BY_ADJUCATION,
        WHITE_WIN_BY_TIMEOUT,
        BLACK_WIN_BY_TIMEOUT,
        DRAW_BY_HALF_MOVES_LIMIT,
        DRAW_BY_STALMATE,
		DRAW_BY_REPETITIONS,
        DRAW_BY_ADJUCATION
    };

	Game(const Position& from, bool time_constraint, time_ms_t time_white, time_ms_t time_black);

	void applyMove(Move32b move);
	void applyMove(Move32b move, time_ms_t think_time);

	// Returns true whether there is a win on the board.
	// 'full' parameter contains detailed info.
	// If there is no win, then full is undefined.
	bool isWin(Result& full) const;

	// Returns true whether there is a draw on the board.
	// 'full' parameter contains detailed info.
	bool isDraw(Result& full) const;

	Position& getPosition();

	FullInfoRecord& getHistoryRecord();
	const FullInfoRecord& getHistoryRecord() const;

	size_t getMoveCount() const;

	static constexpr time_ms_t MoveOverhead = 15_ms;
    static constexpr time_ms_t TimeMargin   = 40_ms;
private:
	bool isGameCycle() const;
	bool isStaleMate() const;
	bool isAnyResponse();

    struct CachedState {
        bool       any_response_cached;
        bool       check;
    };

	Position       _current_pos;
	FullInfoRecord _pos_record;
	time_ms_t      _time_left_sided[2];
	bool           _time_constraint;
    CachedState    _cached;
};

std::string toStr(Game::Result game_result);

bool isWhiteWin(Game::Result game_result);
bool isBlackWin(Game::Result game_result);
bool isDraw    (Game::Result game_result);
