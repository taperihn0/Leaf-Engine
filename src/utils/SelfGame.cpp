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

#include "SelfGame.hpp"
#include "Log.hpp"
#include "backend/Time.hpp"
#include "StaticEval.hpp"

namespace utils {

PairOfForks::PairOfForks()
    : _ordered_forks(&_forks.first, &_forks.second)
{}

std::pair<ForkedProcess&, ForkedProcess&> PairOfForks::raw() {
    return std::make_pair(std::ref(*_ordered_forks.first), std::ref(*_ordered_forks.second));
}

_NODISCARD ForkedProcess& PairOfForks::getFork(bool idx) {
    return idx ? *_ordered_forks.second : *_ordered_forks.first;
}

bool PairOfForks::shuffleOrder() {
    const bool shuffle = rnd::random<int>(0, 1);

    if (shuffle) {
        std::swap(_ordered_forks.first, _ordered_forks.second);
    }

    return shuffle;
}

PairOfForks::iterator PairOfForks::begin() {
    return &_forks.first;
}

PairOfForks::iterator PairOfForks::end() {
    return &_forks.second + 1;
}

SelfGame& SelfGame::get() {
    static SelfGame SelfG;
    return SelfG;
}

template <bool EnableLog>
SelfGame::PlayerPerspectiveResult SelfGame::mixedMatch(PairOfForks& competitors, 
                                                       GameSpecPacket& packet,
                                                       Game* game)
{
    for (const auto& engine : competitors) {
        if (!engine.isAlive()) {
            packet.result = Game::GAME_INVALID;
            return GAME_INVALID;
        }
    }

    // mixing sides to move 
    const bool zero_player_white = !competitors.shuffleOrder();

    for (auto& engine : competitors) {
        engine.output().message("ucinewgame");
    }

    std::string line;

    for (auto& engine : competitors) {
        engine.output().message("isready");

        if (!readline(engine.input(), line) or line != "readyok") {
            packet.result = Game::GAME_INVALID;
            return GAME_INVALID;
        }
    }

    clk::Timer timer;
    search::utils::SearchLimits limits = packet.limits;

    const bool time_constraint = (limits.wtime != 0 and limits.btime != 0);
    int moves_done = 0;

    const Position& opening = packet.openings->getRandomPosition(moves_done);
    const std::string start_fen = opening.createFEN();

    const bool internal_game_storage = game == nullptr;

    std::allocator<Game> al;
    using altraits = std::allocator_traits<decltype(al)>;
    
    if (internal_game_storage) {
        game = altraits::allocate(al, 1);
    }
    
    altraits::construct(al, game, opening, time_constraint, limits.wtime, limits.btime);

    if (packet.data_buffer != nullptr) {
        lg::info("Continuing self-play match without data storage buffer");
    }

    Game::Result game_result;
    uint draw_half_moves = 0;
    const lg::logLabel thread_label = lg::threadLabel(packet.thread_id);

    while (!game->isGameEnd(game_result)) {
        Position& pos = game->getPosition();
        const bool side2move = pos.getTurn();

        ForkedProcess& player2move = competitors.getFork(side2move);
        FullInfoRecord& record = game->getHistoryRecord();

        if (!player2move.isAlive()) {
            game_result = Game::GAME_INVALID;
            break;
        }

        sentPosition<EnableLog>(start_fen, record, player2move, thread_label);
        
        sc::Score score = sc::Undef;

        timer.go();
        const Move32b move = getPlayerMove<EnableLog>(limits, pos, player2move, score, thread_label);

        if (move.isNullMove() or !player2move.isAlive()) {
            game_result = Game::GAME_INVALID;
            break;
        }

        const clk::milliseconds think_time = timer.getDurationMs();

        if (packet.data_buffer != nullptr) {
            if (internal_game_storage) 
                throw std::runtime_error(R"(Data buffer is enabled, but game is not given"
" - can't point to position from game)");
            packet.data_buffer->push_back(PositionInfo{ &pos, side2move == WHITE ? score : -score, move});
        }

        if (time_constraint and side2move == WHITE) {
            limits.wtime -= think_time - limits.winc;
            limits.wtime += Game::MoveOverhead;
            game->applyMove(move, think_time - limits.winc - Game::MoveOverhead);
        }
        else if (time_constraint) {
            limits.btime -= think_time - limits.binc;
            limits.btime += Game::MoveOverhead;
            game->applyMove(move, think_time - limits.binc - Game::MoveOverhead);
        }
        else if (!time_constraint) {
            game->applyMove(move);
        }

        moves_done++;

        if (score.isValid() and std::abs(static_cast<int>(score)) < _LowScore) {
            draw_half_moves++;
        }
        else {
            draw_half_moves = 0;
        }

        // Adjucate game as draw
        if (draw_half_moves > _AdjucateHalfMoveLimit) {
            game_result = Game::DRAW_BY_ADJUCATION;
            break;
        }
        else if (game->getMoveCount() >= MaxGameMoves) {
            game_result = Game::GAME_INVALID;
            break;
        }
    }

    packet.result = game_result;

    if (internal_game_storage) {
        altraits::deallocate(al, game, 1);
    }

    return resultToPerspectiveResult(game_result, zero_player_white);
}

template <bool EnableLog>
void SelfGame::sentPosition(const std::string& start_fen, 
                            const FullInfoRecord& record,
                            ForkedProcess& player,
                            lg::logLabel thread_label) 
{
    /* Is, os are relative to the engines.
    *  We're writing to os, reading from is.
    */

    const int curr_halfmove_clock = static_cast<int>(record.getMoveCount());

    std::ostringstream cmd;
    cmd << "position fen " << start_fen;

    if (curr_halfmove_clock > 0)
        cmd << " moves";

    for (int halfmove_clock = 0;
         halfmove_clock < curr_halfmove_clock;
         halfmove_clock++) 
    {
        Move32b move = record.getPrevMove(halfmove_clock);
        cmd << " " << move;
    }

    player.output().message(cmd.str());

    if constexpr (EnableLog)
        player.output().info(thread_label, cmd.str());
}

template <bool EnableLog>
Move32b SelfGame::getPlayerMove(search::utils::SearchLimits limits,
                                Position& pos,
                                ForkedProcess& player, 
                                sc::Score& score,
                                lg::logLabel thread_label) 
{
    /* Is, os are relative to the engines.
    *  We're writing to os, reading from is.
    */

    std::ostringstream cmd;

    cmd << "go"
        << " nodes " << limits.nodes
        << " qnodes "<< limits.qnodes
        << " depth " << limits.depth
        << " wtime " << limits.wtime
        << " btime " << limits.btime 
        << " winc "  << limits.winc 
        << " binc "  << limits.binc;
    
    player.output().message(cmd.str());

    if constexpr (EnableLog)
        player.output().debug(cmd.str());

    std::string best_move_str;

    for (std::string line; readline(player.input(), line); ) {

        if constexpr (EnableLog)
            player.output().debug(line);

        if (line.empty()) 
            continue;

        std::stringstream ss(line);
        std::string header;
        ss >> header;

        if (header == "info") {
            for (std::string word; ss >> word; ) {
                if (word == "score") {
                    std::string type;
                    int value;
                    ss >> type >> value;

                    if (type == "cp") {
                        score = static_cast<sc::Score>(value);
                    } 
                    else if (type == "mate") {
                        score = (value >= 0) ? (sc::Mate - value) 
                                             : (-sc::Mate - value);
                    }
                }
            }
        } 
        else if (header == "bestmove") {
            ss >> best_move_str;
            break;
        }
    }

    if (best_move_str.empty()) {
        lg::warning("No best move on output from player");
        return NullMove;
    }

    Move32b best_move = Move32b::fromStr<Move32b::Notation::REGULAR>(pos, best_move_str);

    if (!best_move.isLegal(pos)) {
        lg::warning("Recorded invalid best move");
        return NullMove;
    }

    return best_move;
}

_FORCEINLINE SelfGame::PlayerPerspectiveResult 
SelfGame::resultToPerspectiveResult(Game::Result result, 
                                    bool zero_player_white) 
{
    if (result == Game::GAME_INVALID)
        return GAME_INVALID;

    else if (isWhiteWin(result)) {
        switch (result) {
        case Game::WHITE_WIN_BY_MATE: 
            return zero_player_white ? PLAYER_ZERO_WIN_BY_MATE 
                                     : PLAYER_ONE_WIN_BY_MATE;
        case Game::WHITE_WIN_BY_ADJUCATION: 
            return zero_player_white ? PLAYER_ZERO_WIN_BY_ADJUCATION 
                                     : PLAYER_ONE_WIN_BY_ADJUCATION;
        case Game::WHITE_WIN_BY_TIMEOUT: 
            return zero_player_white ? PLAYER_ZERO_WIN_BY_TIMEOUT 
                                     : PLAYER_ONE_WIN_BY_TIMEOUT;
        }
    }

    else if (isBlackWin(result)) {
        switch (result) {
        case Game::BLACK_WIN_BY_MATE: 
            return zero_player_white ? PLAYER_ONE_WIN_BY_MATE 
                                     : PLAYER_ZERO_WIN_BY_MATE;
        case Game::BLACK_WIN_BY_ADJUCATION: 
            return zero_player_white ? PLAYER_ONE_WIN_BY_ADJUCATION 
                                     : PLAYER_ZERO_WIN_BY_ADJUCATION;
        case Game::BLACK_WIN_BY_TIMEOUT: 
            return zero_player_white ? PLAYER_ONE_WIN_BY_TIMEOUT 
                                     : PLAYER_ZERO_WIN_BY_TIMEOUT;
        }
    }

    switch (result) {
    case Game::DRAW_BY_HALF_MOVES_LIMIT: return DRAW_BY_HALF_MOVES_LIMIT;
    case Game::DRAW_BY_STALMATE:         return DRAW_BY_STALMATE;
    case Game::DRAW_BY_REPETITIONS:      return DRAW_BY_REPETITIONS;
    case Game::DRAW_BY_ADJUCATION:       return DRAW_BY_ADJUCATION;
    }

    FAILED_NO_LOG();
    return GAME_INVALID;
}

bool SelfGame::isZeroPlayerWin(PlayerPerspectiveResult result) {
    return result == PLAYER_ZERO_WIN_BY_MATE or
           result == PLAYER_ZERO_WIN_BY_ADJUCATION or
           result == PLAYER_ZERO_WIN_BY_TIMEOUT;
}

bool SelfGame::isOnePlayerWin(PlayerPerspectiveResult result) {
    return result == PLAYER_ONE_WIN_BY_MATE or
           result == PLAYER_ONE_WIN_BY_ADJUCATION or
           result == PLAYER_ONE_WIN_BY_TIMEOUT;
}

bool SelfGame::isDraw(PlayerPerspectiveResult result) {
    return result == DRAW_BY_HALF_MOVES_LIMIT or
           result == DRAW_BY_STALMATE or
           result == DRAW_BY_REPETITIONS or
           result == DRAW_BY_ADJUCATION;
}

template SelfGame::PlayerPerspectiveResult 
SelfGame::mixedMatch<false>(PairOfForks&, GameSpecPacket&, Game*);
template SelfGame::PlayerPerspectiveResult 
SelfGame::mixedMatch<true>(PairOfForks&, GameSpecPacket&, Game*);

} // namespace utils
