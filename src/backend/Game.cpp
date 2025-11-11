#include "Game.hpp"
#include "MoveGen.hpp"

Game::Game(Position&& from,  bool time_constraint, time_ms_t time_white, time_ms_t time_black)
: _current_pos(from)
, _time_left_sided{ time_white, time_black }
, _time_constraint(time_constraint)
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

bool Game::isWin() {
    bool side2move = _current_pos.getTurn();

    if (_time_constraint and _time_left_sided[side2move] < 0)
        return true;
    else if (_is_cached_any_response)
        return _any_response_avaible;    

    _any_response_avaible = isAnyResponse();
    _is_cached_any_response = true;

    return !_any_response_avaible;
}

bool Game::isDraw() {
    return _current_pos.halfmoveClock() >= 50
           or isGameCycle()
           or isStaleMate();
}

Position& Game::getPosition() {
    return _current_pos;
}

FullInfoRecord& Game::getHistoryRecord() {
    return _pos_record;
}

bool Game::isGameCycle()  {
    uint64_t hash_key = _current_pos.getZobristKey();

    int halfmove_cnt = _pos_record.currentHalfCount();
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
