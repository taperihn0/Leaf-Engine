#include "Collector.hpp"

namespace Utils
{

void DataCollector::startTournament(size_t games_count, SearchLimits limits) {
    std::ofstream output_white_win(std::string(_FileWhiteWin), std::ios::binary | std::ios::app);
    std::ofstream output_black_win(std::string(_FileBlackWin), std::ios::binary | std::ios::app);
    std::ofstream output_draw(std::string(_FileDraw), std::ios::binary | std::ios::app);

    if (!output_white_win) {
        ASSERT(false, "Failed to open file " + std::string(_FileWhiteWin));
        return;
    }
    else if (!output_black_win) {
        ASSERT(false, "Failed to open file " + std::string(_FileBlackWin));
        return;
    }
    else if (!output_draw) {
        ASSERT(false, "Failed to open file " + std::string(_FileDraw));
        return;
    }

    std::vector<PackedPosition> positions;
    positions.reserve(MaxGameMoves);

    size_t total_positions = 0;

    for (size_t i = 1; i <= games_count; i++) {
        std::cout << "Starting game " << i << "..." << std::endl;
        Game::Result game_result = _match.start(positions, limits);

        size_t game_positions_cnt = positions.size();
        total_positions += game_positions_cnt;
        
        std::cout << toStr(game_result) << " - collected " << game_positions_cnt << " positions\n";

        for (size_t j = 0; j < game_positions_cnt; j++) {
            const PackedPosition& packed_position = positions[j];
            
            if (game_result == Game::WHITE_WIN_BY_ADJUCATION 
             or game_result == Game::WHITE_WIN_BY_MATE
             or game_result == Game::WHITE_WIN_BY_TIMEOUT) {
                packed_position.write(output_white_win);
            }
            else if (game_result == Game::BLACK_WIN_BY_ADJUCATION
                  or game_result == Game::BLACK_WIN_BY_MATE
                  or game_result == Game::BLACK_WIN_BY_TIMEOUT) {
                packed_position.write(output_black_win);
            }
            else if (game_result == Game::DRAW_BY_HALF_MOVES_LIMIT
                  or game_result == Game::DRAW_BY_REPETITIONS
                  or game_result == Game::DRAW_BY_STEALMATE) {
                packed_position.write(output_draw);
            } 
            else ASSERT(false, "No other game results");
        }

        positions.clear();
    }

    std::cout << games_count << " games played - total of " << total_positions << " positions collected." << std::endl;
}

} // namespace Utils
