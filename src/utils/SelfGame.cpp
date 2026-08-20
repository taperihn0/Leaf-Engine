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
#include "backend/Time.hpp"
#include "StaticEval.hpp"
#include "Process.hpp"

namespace utils {

template <bool EnableLog>
SelfGame::PlayerPerspectiveResult SelfGame::mixedMatch(EngineProcess& engine0, 
                                                       EngineProcess& engine1, 
                                                       GameSpecPacket& packet)
{
    if (!engine0.isAlive() or !engine1.isAlive()) {
        *packet.result = Game::GAME_INVALID;
        return GAME_INVALID;
    }

    std::array<EngineProcess*, 2> player;
    
    // true zero_player_white means (is0, os0) engine is white player
    const bool zero_player_white = rnd::random<int>(0, 1);

    // mixing sides to move 
    if (zero_player_white) {
        player[WHITE] = &engine0;
        player[BLACK] = &engine1;
    } 
    else {
        player[WHITE] = &engine1;
        player[BLACK] = &engine0;
    }

    log(*player[WHITE]->proc_stdin, "ucinewgame");
    log(*player[BLACK]->proc_stdin, "ucinewgame");

    for (enumColor side : { WHITE, BLACK }) {
        log(*player[side]->proc_stdin, "isready");

        std::string line;
        if (!readline(*player[side]->proc_stdout, line) or line != "readyok") {
            *packet.result = Game::GAME_INVALID;
            return GAME_INVALID;
        }
    }

    clk::Timer timer;
    search::SearchLimits limits = packet.limits;

    const bool time_constraint = (limits.wtime != 0 and limits.btime != 0);
    int moves_done = 0;

    const Position& opening = packet.openings->getRandomPosition(moves_done);
    const std::string start_fen = opening.createFEN();

    Game game(opening, time_constraint, limits.wtime, limits.btime);
    Game::Result game_result;

    uint draw_half_moves = 0;

    const enumLogLabel thread_label = threadLabel(packet.thread_id);
    enumLogLabel debug_labels[2] = { LOG_ENGINE_0, LOG_ENGINE_1 };
    enumLogLabel info_labels[2] = { LOG_ENGINE_0, LOG_ENGINE_1 };
    
    if constexpr (EnableLog) {
        debug_labels[0] = zero_player_white ? LOG_DEBUG | LOG_ENGINE_0 | thread_label 
                                            : LOG_DEBUG | LOG_ENGINE_1 | thread_label;
        debug_labels[1] = zero_player_white ? LOG_DEBUG | LOG_ENGINE_1 | thread_label 
                                            : LOG_DEBUG | LOG_ENGINE_0 | thread_label;

        info_labels[0]  = zero_player_white ? LOG_INFO | LOG_ENGINE_0 | thread_label 
                                            : LOG_INFO | LOG_ENGINE_1 | thread_label;
        info_labels[1]  = zero_player_white ? LOG_INFO | LOG_ENGINE_1 | thread_label 
                                            : LOG_INFO | LOG_ENGINE_0 | thread_label;
    }

    if (packet.train_data_spec != nullptr) {
        ASSERT_NOLOG(packet.train_data_spec->positions_buf != nullptr and
                    packet.train_data_spec->white_scores_buf != nullptr and
                    packet.train_data_spec->moves_buf != nullptr);

        if (!packet.train_data_spec->positions_buf->empty())
            packet.train_data_spec->positions_buf->clear();

        if (!packet.train_data_spec->white_scores_buf->empty())
            packet.train_data_spec->white_scores_buf->clear();

        if (!packet.train_data_spec->moves_buf->empty())
            packet.train_data_spec->moves_buf->clear();
    }

    while (!game.isWin(game_result) and !game.isDraw(game_result)) {
        Position& pos = game.getPosition();
        const bool side2move = pos.getTurn();
        EngineProcess* curr_player = player[side2move];
        FullInfoRecord& record = game.getHistoryRecord();

        sentPosition<EnableLog>(start_fen, record, 
                                *curr_player,
                                info_labels[side2move]);
        
        sc::Score score = sc::Undef;

        timer.go();
        Move32b move = getPlayerMove<EnableLog>(limits, pos, 
                                                *curr_player,
                                                score,
                                                debug_labels[side2move]);

        if (move.isNullMove() or
            !engine0.isAlive() or
            !engine1.isAlive()) {
            game_result = Game::GAME_INVALID;
            break;   
        }

        const clk::milliseconds think_time = timer.getDurationMs();

        if (packet.train_data_spec != nullptr) {
            packet.train_data_spec->positions_buf->push_back(pos);

            const sc::Score white_score = pos.getTurn() == WHITE ? score : -score;
            packet.train_data_spec->white_scores_buf->push_back(white_score);
            
            packet.train_data_spec->moves_buf->push_back(move);
        }

        if (time_constraint and side2move == WHITE) {
            limits.wtime -= think_time - limits.winc;
            limits.wtime += Game::MoveOverhead;

            game.applyMove(move, think_time - limits.winc - Game::MoveOverhead);
        }
        else if (time_constraint) {
            limits.btime -= think_time - limits.binc;
            limits.btime += Game::MoveOverhead;

            game.applyMove(move, think_time - limits.binc - Game::MoveOverhead);
        }
        else if (!time_constraint) {
            game.applyMove(move);
        }

        moves_done++;

        if (score.isValid() and std::abs(static_cast<int>(score)) < _LowScore) 
            draw_half_moves++;
        else
            draw_half_moves = 0;

        // Adjucate game as draw
        if (draw_half_moves > _AdjucateHalfMoveLimit) {
            game_result = Game::DRAW_BY_ADJUCATION;
            break;
        }
        else if (game.getMoveCount() >= MaxGameMoves) {
            game_result = Game::GAME_INVALID;
            break;
        }
    }

    *packet.result = game_result;
    return resultToPerspectiveResult(game_result, zero_player_white);
}

template <bool EnableLog>
void SelfGame::sentPosition(const std::string& start_fen, 
                            const FullInfoRecord& record,
                            EngineProcess& player,
                            enumLogLabel ret_msg_label) 
{
    /* Is, os are relative to the engines.
    *  We're writing to os, reading from is.
    */

    const int curr_halfmove_clock = static_cast<int>(record.getMoveCount());

    std::stringstream cmd;
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

    const std::string msg = cmd.str();
    log(*player.proc_stdin, msg);

    if constexpr (EnableLog)
        labelLog(std::cout, ret_msg_label, msg);
}

template <bool EnableLog>
Move32b SelfGame::getPlayerMove(search::SearchLimits limits,
                                Position& pos,
                                EngineProcess& player, 
                                sc::Score& score,
                                enumLogLabel ret_msg_label) 
{
    /* Is, os are relative to the engines.
    *  We're writing to os, reading from is.
    */

    std::stringstream cmd;

    cmd << "go"
        << " nodes " << limits.nodes
        << " qnodes "<< limits.qnodes
        << " depth " << limits.depth
        << " wtime " << limits.wtime
        << " btime " << limits.btime 
        << " winc "  << limits.winc 
        << " binc "  << limits.binc;
    
    log(*player.proc_stdin, cmd.str());

    if constexpr (EnableLog)
        labelLog(std::cout, ret_msg_label, cmd.str());

    std::string best_move_str;

    for (std::string line; readline(*player.proc_stdout, line); ) {

        if constexpr (EnableLog)
            labelLog(std::cout, ret_msg_label, line);

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
        labelLog(std::cout, LOG_INFO, "Null best move");
        return NullMove;
    }

    Move32b best_move = Move32b::fromStr<Move32b::Notation::REGULAR>(pos, best_move_str);

    if (!best_move.isLegal(pos)) {
        labelLog(std::cout, LOG_INFO, "Invalid best move");
        return NullMove;
    }

    return best_move;
}

_FORCEINLINE SelfGame::PlayerPerspectiveResult SelfGame::resultToPerspectiveResult(Game::Result result, bool zero_player_white) {
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

    ASSERT_NOLOG(false);
    return GAME_INVALID;
}

bool isZeroPlayerWin(SelfGame::PlayerPerspectiveResult result) {
    return result == SelfGame::PLAYER_ZERO_WIN_BY_MATE ||
           result == SelfGame::PLAYER_ZERO_WIN_BY_ADJUCATION ||
           result == SelfGame::PLAYER_ZERO_WIN_BY_TIMEOUT;
}

bool isOnePlayerWin(SelfGame::PlayerPerspectiveResult result) {
    return result == SelfGame::PLAYER_ONE_WIN_BY_MATE ||
           result == SelfGame::PLAYER_ONE_WIN_BY_ADJUCATION ||
           result == SelfGame::PLAYER_ONE_WIN_BY_TIMEOUT;
}

bool isDraw(SelfGame::PlayerPerspectiveResult result) {
    return result == SelfGame::DRAW_BY_HALF_MOVES_LIMIT ||
           result == SelfGame::DRAW_BY_STALMATE ||
           result == SelfGame::DRAW_BY_REPETITIONS ||
           result == SelfGame::DRAW_BY_ADJUCATION;
}

template SelfGame::PlayerPerspectiveResult SelfGame::mixedMatch<false>(EngineProcess&, 
                                                                       EngineProcess&, 
                                                                       GameSpecPacket&);
template SelfGame::PlayerPerspectiveResult SelfGame::mixedMatch<true>(EngineProcess&, 
                                                                      EngineProcess&, 
                                                                      GameSpecPacket&);

} // namespace utils
