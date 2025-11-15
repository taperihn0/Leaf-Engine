#include "Game.hpp"
#include "MoveGen.hpp"

Game::Game(Position&& from,  bool time_constraint, time_ms_t time_white, time_ms_t time_black)
: _current_pos(from)
, _time_left_sided{ time_white, time_black }
, _time_constraint(time_constraint)
, _is_cached_any_response(false)
{}

void Game::applyMove(Move32b move, time_ms_t think_time) {
    bool side2move = _current_pos.getTurn();

    if (_time_constraint)
        _time_left_sided[side2move] -= think_time;

    _current_pos.make(move);

    uint64_t key = _current_pos.getZobristKey();
    _pos_record.recordInfo(key, move);

    _is_cached_any_response = false;
}

#define WIN_BY_MATE(color)       static_cast<Game::Result>(Game::WHITE_WIN_BY_MATE + color)
#define WIN_BY_ADJUCATION(color) static_cast<Game::Result>(Game::WHITE_WIN_BY_ADJUCATION + color)
#define WIN_BY_TIMEOUT(color)    static_cast<Game::Result>(Game::WHITE_WIN_BY_TIMEOUT + color)

bool Game::isWin(Game::Result& full) {
    bool side2move = _current_pos.getTurn();

    if (_time_constraint and _time_left_sided[side2move] < 0) {
        full = WIN_BY_TIMEOUT(!side2move);
        return true;
    } else if (_is_cached_any_response) {
        full = WIN_BY_MATE(!side2move);
        return _any_response_avaible;
    }

    _any_response_avaible = isAnyResponse();
    _is_cached_any_response = true;

    full = WIN_BY_MATE(!side2move);
    return !_any_response_avaible;
}

bool Game::isDraw(Game::Result& full) {
    if (_current_pos.halfmoveClock() >= 50) {
        full = DRAW_BY_HALF_MOVES_LIMIT;
        return true;
    }
    else if (isGameCycle()) {
        full = DRAW_BY_REPETITIONS;
        return true;
    }
    else if (isStaleMate()) {
        full = DRAW_BY_STEALMATE;
        return true;
    }
    return false;
}

Position& Game::getPosition() {
    return _current_pos;
}

FullInfoRecord& Game::getHistoryRecord() {
    return _pos_record;
}

bool Game::isGameCycle()  {
    uint64_t hash_key = _current_pos.getZobristKey();

    int halfmove_cnt = static_cast<int>(_pos_record.currentHalfCount());
    int repetition_cnt = 0;

    for (int i = 1; i <= halfmove_cnt; i++) {
		const int cnt = halfmove_cnt - i;

		const Move32b move = _pos_record.getPrevMove(cnt);

		if ((i & 1) == 0)
			continue;
		else if (move.isIrreversible())
			return false;
		else if (hash_key == _pos_record.getPrevKey(cnt))
		{
			if (++repetition_cnt >= 2)
				return true;
		}
	}

    return false;
}

INLINE bool Game::isStaleMate() {
    _any_response_avaible = isAnyResponse();
    _is_cached_any_response = true;
    return _current_pos.isInCheck(_current_pos.getTurn()) and !_any_response_avaible;
}

INLINE bool Game::isAnyResponse() {
    MoveList ml;
    MoveGen::generatePseudoLegalMoves<MoveGen::ALL>(_current_pos, ml);
    return ml.any([&](Move32b m) { return m.isLegal(_current_pos); });
}

std::string toStr(Game::Result game_result) {
    switch (game_result) {
    case Game::WHITE_WIN_BY_ADJUCATION:
        return "white win by adjucation";
    case Game::WHITE_WIN_BY_MATE:
        return "white win by mate";
    case Game::WHITE_WIN_BY_TIMEOUT:
        return "white win by timeout";
    case Game::BLACK_WIN_BY_ADJUCATION:
        return "black win by adjucation";
    case Game::BLACK_WIN_BY_MATE:
        return "black win by mate";
    case Game::BLACK_WIN_BY_TIMEOUT:
        return "black win by timeout";
    case Game::DRAW_BY_HALF_MOVES_LIMIT:
        return "draw by 50-moves rule";
    case Game::DRAW_BY_REPETITIONS:
        return "draw by repetitions";
    case Game::DRAW_BY_STEALMATE:
        return "draw by stealmate";
    default:     
        ASSERT(false, "No other game results");
    }

    return "";
}
