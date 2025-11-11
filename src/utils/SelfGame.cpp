#include "SelfGame.hpp"
#include "backend/Time.hpp"

namespace Utils {

OpeningGenerator SelfGame::_openings(_OpeningFile);

SelfGame::SelfGame(size_t tt_size)
: _search_by_side{ Search(TranspositionTable(tt_size)),
                   Search(TranspositionTable(tt_size)) } 
{
    _openings.load();
}

void SelfGame::start(std::vector<PackedPosition>& positions, SearchLimits limits) {
    for (Search& search : _search_by_side)
        search.registerNewGame();

    Timer timer;
    const bool time_constraint = limits.wtime != 0 and limits.btime != 0;

    Game game(_openings.getPosition(), time_constraint, limits.wtime, limits.btime);
    
    while (!game.isWin() and !game.isDraw()) {
        Position& pos = game.getPosition();
        bool side2move = pos.getTurn();
        Search& curr_search = _search_by_side[side2move];
        FullInfoRecord& record = game.getHistoryRecord();

        positions.push_back(PackedPosition::packed(game.getPosition()));

        timer.go();
        Move32b move = curr_search.bestMove(pos, record, limits);
        time_ms_t think_time = timer.duration();

        game.applyMove(move, think_time);

        if (time_constraint and side2move == WHITE)
            limits.wtime -= think_time - limits.winc;
        else if (time_constraint)
            limits.btime -= think_time - limits.binc;
    }
}

} // namespace Utils
