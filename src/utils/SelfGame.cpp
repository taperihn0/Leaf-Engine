#include "SelfGame.hpp"
#include "backend/Time.hpp"
#include "StaticEval.hpp"

namespace Utils {

template <bool EnableLog>
SelfGame::PlayerPerspectiveResult SelfGame::mixedMatch(SelfGame::GameSpecPacket& packet) {
    std::array<EnginePlayer, 2> player;

    // true zero_player_white means (is0, os0) engine is white player
    const bool zero_player_white = random<int>(0, 1);

    // mixing sides to move 
    if (zero_player_white) {
        player[WHITE] = packet.engine0;
        player[BLACK] = packet.engine1;
    } 
    else {
        player[WHITE] = packet.engine1;
        player[BLACK] = packet.engine0;
    }

    log(*player[WHITE].is, "ucinewgame");
    log(*player[BLACK].is, "ucinewgame");

    for (enumColor side : { WHITE, BLACK }) {
        log(*player[side].is, "isready");

        std::string line;
        while ((readline(*player[side].os, line), line != "readyok"));
    }

    Timer timer;
    SearchLimits limits = packet.limits;

    const bool time_constraint = (limits.wtime != 0 and limits.btime != 0);

    const Position& opening = packet.openings->getRandomPosition();
    const std::string start_fen = opening.createFEN();

    Game game(opening, time_constraint, limits.wtime, limits.btime);
    Game::Result game_result;

    uint draw_full_moves = 0;

    const enumLogLabel thread_label = threadLabel(packet.thread_id);
    enumLogLabel debug_labels[2];
    enumLogLabel info_labels[2];
    
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

    while (!game.isWin(game_result) and !game.isDraw(game_result)) {
        Position& pos = game.getPosition();
        const bool side2move = pos.getTurn();
        EnginePlayer& curr_player = player[side2move];
        FullInfoRecord& record = game.getHistoryRecord();

        Score eval = Score::Undef;
        sentPosition<EnableLog>(start_fen, record, 
                                curr_player,
                                info_labels[side2move]);

        timer.go();
        Move32b move = getPlayerMove<EnableLog>(limits, pos, 
                                                curr_player,
                                                eval,
                                                debug_labels[side2move]);
        time_ms_t think_time = timer.duration();

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

        ASSERTNOLOG(eval != Score::Undef);

        if (std::abs(static_cast<int>(eval)) < 90) 
            draw_full_moves += side2move;
        else
            draw_full_moves = 0;

        // Adjucate game as draw
        if (draw_full_moves > 35) {
            game_result = Game::DRAW_BY_ADJUCATION;
            break;
        }
    }

    *packet.info = toStr(game_result);
    return resultToPerspectiveResult(game_result, zero_player_white);
}

template <bool EnableLog>
void SelfGame::sentPosition(const std::string& start_fen, 
                            const FullInfoRecord& record,
                            EnginePlayer player,
                            enumLogLabel ret_msg_label) 
{
    // Is, os are relative to the engines.
    // We're writing to os, reading from is.

    const int curr_halfmove_clock = static_cast<int>(record.currentHalfCount());

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
    log(*player.is, msg);

    if constexpr (EnableLog)
        labelLog(std::cout, ret_msg_label, msg);
}

template <bool EnableLog>
Move32b SelfGame::getPlayerMove(SearchLimits limits,
                                const Position& pos,
                                EnginePlayer player, 
                                Score& score,
                                enumLogLabel ret_msg_label) 
{
    // is, os streams are relative to the engines.
    // We're writing to is, reading from os.

    std::stringstream cmd;

    cmd << "go"
        << " depth " << limits.depth
        << " wtime " << limits.wtime
        << " btime " << limits.btime 
        << " winc "  << limits.winc 
        << " binc "  << limits.binc;
    
    log(*player.is, cmd.str());

    if constexpr (EnableLog)
        labelLog(std::cout, ret_msg_label, cmd.str());

    std::string line;
    std::string bestMoveStr;

    while (readline(*player.os, line)) {

        if constexpr (EnableLog)
            labelLog(std::cout, ret_msg_label, line);

        if (line.empty()) 
            continue;

        std::stringstream ss(line);
        std::string header;
        ss >> header;

        if (header == "info") {
            std::string word;

            while (ss >> word) {
                if (word == "score") {
                    std::string type;
                    int value;
                    ss >> type >> value;

                    if (type == "cp") {
                        score = static_cast<Score>(value);
                    } 
                    else if (type == "mate") {
                        score = (value >= 0) ? (Score::Mate - value) 
                                             : (-Score::Mate - value);
                    }
                }
            }
        } 
        else if (header == "bestmove") {
            ss >> bestMoveStr;
            break;
        }
    }

    return Move32b::fromStr<Move32b::Notation::REGULAR>(pos, bestMoveStr);
}

_FORCEINLINE SelfGame::PlayerPerspectiveResult SelfGame::resultToPerspectiveResult(Game::Result result, bool zero_player_white)
{
    if (isWhiteWin(result)) {
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

    ASSERTNOLOG(false);
    return GAME_INVALID;
}

bool isZeroPlayerWin(SelfGame::PlayerPerspectiveResult result)
{
    return result == SelfGame::PLAYER_ZERO_WIN_BY_MATE ||
           result == SelfGame::PLAYER_ZERO_WIN_BY_ADJUCATION ||
           result == SelfGame::PLAYER_ZERO_WIN_BY_TIMEOUT;
}

bool isOnePlayerWin(SelfGame::PlayerPerspectiveResult result)
{
    return result == SelfGame::PLAYER_ONE_WIN_BY_MATE ||
           result == SelfGame::PLAYER_ONE_WIN_BY_ADJUCATION ||
           result == SelfGame::PLAYER_ONE_WIN_BY_TIMEOUT;
}

bool isDraw(SelfGame::PlayerPerspectiveResult result)
{
    return result == SelfGame::DRAW_BY_HALF_MOVES_LIMIT ||
           result == SelfGame::DRAW_BY_STALMATE ||
           result == SelfGame::DRAW_BY_REPETITIONS ||
           result == SelfGame::DRAW_BY_ADJUCATION;
}

template SelfGame::PlayerPerspectiveResult SelfGame::mixedMatch<false>(GameSpecPacket&);
template SelfGame::PlayerPerspectiveResult SelfGame::mixedMatch<true>(GameSpecPacket&);

} // namespace Utils
