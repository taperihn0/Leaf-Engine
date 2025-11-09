#include "SelfGame.hpp"
#include "backend/Time.hpp"

namespace Utils {

SelfGame::SelfGame(size_t tt_size)
: _openings(_OpeningFile)
, _search_by_side{ Search(TranspositionTable(tt_size)),
                   Search(TranspositionTable(tt_size)) } 
{}

Game SelfGame::start(SearchLimits limits) {
    for (Search& search : _search_by_side)
        search.registerNewGame();

    Timer timer;
    Game game(_openings.get(), limits.wtime, limits.btime);

    while (true) {
        Position& pos = game.getPosition();
        bool side2move = pos.getTurn();
        Search& curr_search = _search_by_side[side2move];
        FullInfoRecord& record = game.getHistoryRecord();

        timer.go();
        Move32b move = curr_search.bestMove(pos, record, limits);
        time_ms_t think_time = timer.duration();

        game.applyMove(move, think_time);

        if (game.isWin() or game.isDraw())
            break;

        if (side2move == WHITE) {
            limits.wtime -= think_time - limits.winc;
        } else {
            limits.btime -= think_time - limits.binc;
        }
    }

    return game;
}

} // namespace Utils
