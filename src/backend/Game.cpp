/*
 * Leaf, a UCI Chess Engine
 * Copyright (C) 2026 taperihn0
 *
 * Leaf is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Leaf is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#include "Game.hpp"
#include "MoveGen.hpp"

Game::Game(const Position& from,  bool time_constraint, clk::milliseconds time_white, clk::milliseconds time_black)
    : _current_pos(from)
    , _time_left_sided{ time_white, time_black }
    , _time_constraint(time_constraint)
    , _cached{ isAnyResponse(), 
               _current_pos.isInCheck(_current_pos.getTurn()) }
{}

void Game::applyMove(Move32b move) {
    ASSERT(!_time_constraint, "Ignoring think time info");
    applyMove(move, 0);
}

void Game::applyMove(Move32b move, clk::milliseconds think_time) {
    bool side2move = _current_pos.getTurn();

    if (_time_constraint) {
        _time_left_sided[side2move] -= think_time;
    }

    ASSERT_NOLOG(_current_pos.make(move));

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
    if (_current_pos.getHalfmoveClock() >= 100) {
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

const FullInfoRecord& Game::getHistoryRecord() const {
    return _pos_record;
}

std::size_t Game::getMoveCount() const {
    return getHistoryRecord().getMoveCount();
}

bool Game::isGameCycle() const {
    uint64_t hash_key = _current_pos.getZobristKey();

    int halfmove_cnt = static_cast<int>(_pos_record.getMoveCount());
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

_INLINE bool Game::isStaleMate() const {
    return !_cached.check and !_cached.any_response_cached;
}

_INLINE bool Game::isAnyResponse() {
    ml::MoveList ml;
    MoveGen::generatePseudoLegalMoves<MoveGen::ALL>(_current_pos, ml);

    return ml.any([&](ml::MoveList::Entry en) { 
        return en.move().isLegal(_current_pos); 
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
    case Game::GAME_INVALID:
        return "invalid game";
    default:     
        FAILED("No other game results");
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
