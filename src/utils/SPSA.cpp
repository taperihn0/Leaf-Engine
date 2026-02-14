#include "SPSA.h"
#include "Process.hpp"
#include "Opening.hpp"

#include <sys/wait.h>

namespace Utils {

void SPSA_Tuning::run() {
    auto engine0 = spawnProcess();
    auto engine1 = spawnProcess();

    auto& in0 = *engine0.in;
    auto& os0 = *engine0.out;
    auto& in1 = *engine1.in;
    auto& os1 = *engine1.out;

    // ...
}

_FORCEINLINE void log(std::ostream& is, const std::string& c_str) {
    is << c_str << std::endl;
}

_FORCEINLINE std::istream& readline(std::istream& os, std::string& line) {
    return std::getline(os, line);
}

INLINE float SPSA_Tuning::match(SearchLimits limits,
                                std::istream& engine_os0, std::ostream& engine_is0,
                                std::istream& engine_os1, std::ostream& engine_is1)
{
    // Is, os are relative to the engines.
    // We're writing to os, reading from is.
    struct EnginePlayer {
        std::istream* os;
        std::ostream* is;
    };

    EnginePlayer player[2];

    // (is0, os0) engine is white player
    const bool plus_player_white = random<int>(0, 1);

    // mixing sides to move 
    if (plus_player_white) {
        player[WHITE] = { &engine_os0, &engine_is0 };
        player[BLACK] = { &engine_os1, &engine_is1 };
    } 
    else {
        player[WHITE] = { &engine_os1, &engine_is1 };
        player[BLACK] = { &engine_os0, &engine_is0 };
    }

    log(*player[WHITE].is, "ucinewgame");
    log(*player[BLACK].is, "ucinewgame");

    for (enumColor side : { WHITE, BLACK }) {
        log(*player[side].is, "isready");

        std::string line;
        while ((readline(*player[side].os, line), line != "readyok"));
    }

    Timer timer;
    const bool time_constraint = limits.wtime != 0 and limits.btime != 0;

    // Remember about loading random position first!
    Position opening = OpeningGenerator::getPosition();
    const std::string opening_fen = opening.createFEN();

    Game game(opening, time_constraint, limits.wtime, limits.btime);
    Game::Result game_result;

    static constexpr time_ms_t MoveOverhead = 15_ms;

    uint draw_full_moves = 0;

    while (!game.isWin(game_result) and !game.isDraw(game_result)) {
        Position& pos = game.getPosition();
        const bool side2move = pos.getTurn();
        EnginePlayer& curr_player = player[side2move];
        FullInfoRecord& record = game.getHistoryRecord();

        Score eval = Score::Undef;
        sentPosition(opening_fen, record, *curr_player.os, *curr_player.is);

        timer.go();
        Move32b move = getPlayerMove(limits, pos, *curr_player.os, *curr_player.is, eval);
        time_ms_t think_time = timer.duration();

        game.applyMove(move, think_time);

        if (time_constraint and side2move == WHITE) {
            limits.wtime -= think_time - limits.winc;
            limits.wtime += MoveOverhead;
        }
        else if (time_constraint) {
            limits.btime -= think_time - limits.binc;
            limits.btime += MoveOverhead;
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

    //  1. - if player 0 wins
    // -1. - if player 1 wins

    if (isWhiteWin(game_result)) {
        return plus_player_white ? 1.f : -1.f;
    }
    else if (isBlackWin(game_result)) {
        return plus_player_white ? -1.f : 1.f;
    }
    return 0.f;
}

void SPSA_Tuning::sentPosition(const std::string& start_fen, 
                               const FullInfoRecord& record,
                               std::istream& engine_os, std::ostream& engine_is) 
{
    // Is, os are relative to the engines.
    // We're writing to os, reading from is.

    const int curr_halfmove_clock = record.currentHalfCount();

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
    log(engine_is, msg);
}

Move32b SPSA_Tuning::getPlayerMove(SearchLimits limits,
                                   const Position& pos,
                                   std::istream& engine_os, std::ostream& engine_is, 
                                   Score& score) 
{
    // is, os streams are relative to the engines.
    // We're writing to is, reading from os.

    std::stringstream cmd;
    cmd << "go";
    
    if (limits.depth > 0) {
        cmd << " depth " << limits.depth;
    } else {
        cmd << " wtime " << limits.wtime 
            << " btime " << limits.btime 
            << " winc "  << limits.winc 
            << " binc "  << limits.binc;
    }
    
    log(engine_is, cmd.str());

    std::string line;
    std::string bestMoveStr;

    while (readline(engine_os, line)) {
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

}
