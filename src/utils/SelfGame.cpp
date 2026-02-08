#include "SelfGame.hpp"
#include "backend/Time.hpp"
#include "StaticEval.hpp"

namespace Utils {

SelfGame::SelfGame(size_t tt_size_per_search)
: _search_by_side{ Search(TranspositionTable(tt_size_per_search)),
                   Search(TranspositionTable(tt_size_per_search)) } 
{}

Game::Result SelfGame::start(std::vector<ExtPackedPosition>& packed_positions, SearchLimits limits) {
    _search_by_side[WHITE].registerNewGame();
    _search_by_side[BLACK].registerNewGame();

    Timer timer;
    const bool time_constraint = limits.wtime != 0 and limits.btime != 0;

    Game game(OpeningGenerator::getPosition(), time_constraint, limits.wtime, limits.btime);
    Game::Result game_result;

    unsigned int draw_full_moves = 0;

    while (!game.isWin(game_result) and !game.isDraw(game_result)) {
        Position& pos = game.getPosition();
        bool side2move = pos.getTurn();
        Search& curr_search = _search_by_side[side2move];
        FullInfoRecord& record = game.getHistoryRecord();
        const int eval = static_cast<int>(StaticEval::staticEval(pos));

        if (std::abs(eval) < 90) 
            draw_full_moves += side2move;
        else
            draw_full_moves = 0;

        // Adjucate game as draw
        if (draw_full_moves > 7) {
            game_result = Game::DRAW_BY_ADJUCATION;
            break;
        }

        packed_positions.emplace_back(pos);

        timer.go();
        Move32b move = curr_search.bestMove<Search::SEARCH_NO_INFO>(pos, record, limits);
        time_ms_t think_time = timer.duration();

        game.applyMove(move, think_time);

        if (time_constraint and side2move == WHITE)
            limits.wtime -= think_time - limits.winc;
        else if (time_constraint)
            limits.btime -= think_time - limits.binc;
    }

    return game_result;
}

} // namespace Utils
