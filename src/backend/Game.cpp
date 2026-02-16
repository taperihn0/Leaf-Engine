#include "Game.hpp"
#include "MoveGen.hpp"

Game::Game(const Position& from,  bool time_constraint, time_ms_t time_white, time_ms_t time_black)
    : _current_pos(from)
    , _time_left_sided{ time_white, time_black }
    , _time_constraint(time_constraint)
    , _cached{ isAnyResponse(), 
               _current_pos.isInCheck(_current_pos.getTurn()) }
{}

void Game::applyMove(Move32b move, time_ms_t think_time) {
    bool side2move = _current_pos.getTurn();

    if (_time_constraint) {
        _time_left_sided[side2move] -= think_time;
    }

    _current_pos.make(move);

    uint64_t key = _current_pos.getZobristKey();
    _pos_record.recordInfo(key, move);

    _cached.any_response_cached = isAnyResponse();
    _cached.check = _current_pos.isInCheck(_current_pos.getTurn());
}

#define WIN_BY_MATE(color)       static_cast<Game::Result>(Game::WHITE_WIN_BY_MATE + (color))
#define WIN_BY_ADJUCATION(color) static_cast<Game::Result>(Game::WHITE_WIN_BY_ADJUCATION + (color))
#define WIN_BY_TIMEOUT(color)    static_cast<Game::Result>(Game::WHITE_WIN_BY_TIMEOUT + (color))

bool Game::isWin(Game::Result& full) const {
    bool side2move = _current_pos.getTurn();

    if (_time_constraint and _time_left_sided[side2move] + TimeMargin < 0) {
        full = WIN_BY_TIMEOUT(!side2move);
        return true;
    }

    full = WIN_BY_MATE(!side2move);
    return _cached.check and !_cached.any_response_cached;
}

bool Game::isDraw(Game::Result& full) const {
    if (_current_pos.getHalfmoveClock() >= 50) {
        full = DRAW_BY_HALF_MOVES_LIMIT;
        return true;
    }
    else if (isGameCycle()) {
        full = DRAW_BY_REPETITIONS;
        return true;
    }
    else if (isStaleMate()) {
        full = DRAW_BY_STALMATE;
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

bool Game::isGameCycle() const {
    uint64_t hash_key = _current_pos.getZobristKey();

    int halfmove_cnt = static_cast<int>(_pos_record.currentHalfCount());
    int repetition_cnt = 0;

    for (int i = 1; i <= halfmove_cnt; i++) {
		const int cnt = halfmove_cnt - i;

		const Move32b move = _pos_record.getPrevMove(cnt);

        if (move.isIrreversible())
			return false;
		else if (hash_key == _pos_record.getPrevKey(cnt)) {
			if (++repetition_cnt >= 3)
				return true;
		}
	}

    return false;
}

INLINE bool Game::isStaleMate() const {
    return !_cached.check and !_cached.any_response_cached;
}

INLINE bool Game::isAnyResponse() {
    MoveList ml;
    MoveGen::generatePseudoLegalMoves<MoveGen::ALL>(_current_pos, ml);

    return ml.any([&](MoveList::Entry en) { 
        return en.move.isLegal(_current_pos); 
    });
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
    case Game::DRAW_BY_STALMATE:
        return "draw by stealmate";
    case Game::DRAW_BY_ADJUCATION:
        return "draw by adjucation";
    default:     
        ASSERT(false, "No other game results");
    }

    return "";
}

bool isWhiteWin(Game::Result game_result) {
    return game_result == Game::WHITE_WIN_BY_ADJUCATION
        or game_result == Game::WHITE_WIN_BY_MATE
        or game_result == Game::WHITE_WIN_BY_TIMEOUT;
}

bool isBlackWin(Game::Result game_result) {
    return game_result == Game::BLACK_WIN_BY_ADJUCATION
        or game_result == Game::BLACK_WIN_BY_MATE
        or game_result == Game::BLACK_WIN_BY_TIMEOUT;
}

bool isDraw(Game::Result game_result) {
    return game_result == Game::DRAW_BY_HALF_MOVES_LIMIT
        or game_result == Game::DRAW_BY_REPETITIONS
        or game_result == Game::DRAW_BY_STALMATE
        or game_result == Game::DRAW_BY_ADJUCATION;
}
