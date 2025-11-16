#include "Collector.hpp"
#include <thread>

namespace Utils
{

void incGameCount(DataCollector::CommonThreadData* common) {
    common->common_lock.lock();
    common->games_ended++;
    common->common_lock.unlock();
}

void perThreadGameLoop(DataCollector::PerThreadData& thread) {
    ASSERT(thread.output_white_win.is_open(), "Output not opened.");
    ASSERT(thread.output_black_win.is_open(), "Output not opened.");
    ASSERT(thread.output_draw     .is_open(), "Output not opened.");
    ASSERT(thread.commons != nullptr,         "Thread commons not initialized");

    std::vector<PackedPosition> positions;
    positions.reserve(MaxGameMoves);

    using thread_id_t = std::thread::id;

    thread_id_t thread_id = std::this_thread::get_id();

    auto print_thread_info = [&](thread_id_t id) {
        std::cout << "[THREAD " << thread_id << "] ";
    };

    thread.games_ended = 0;
    thread.total_positions = 0;

    SelfGame match(1_MB);

    for (size_t i = 0; thread.commons->games_ended < thread.commons->total_games; i++) {
        print_thread_info(thread_id);
        std::cout << "Starting game " << i << "..." << std::endl;

        Game::Result game_result = match.start(positions, thread.limits);

        size_t game_positions_cnt = positions.size();
        thread.total_positions += game_positions_cnt;

        print_thread_info(thread_id);
        std::cout << toStr(game_result) << " - collected " << game_positions_cnt << " positions\n";

        for (size_t j = 0; j < game_positions_cnt; j++) {
            const PackedPosition& packed_position = positions[j];

           if (game_result == Game::WHITE_WIN_BY_ADJUCATION
                 or game_result == Game::WHITE_WIN_BY_MATE
                 or game_result == Game::WHITE_WIN_BY_TIMEOUT) {
                packed_position.write(thread.output_white_win);
            }
            else if (game_result == Game::BLACK_WIN_BY_ADJUCATION
                 or game_result == Game::BLACK_WIN_BY_MATE
                 or game_result == Game::BLACK_WIN_BY_TIMEOUT) {
                packed_position.write(thread.output_black_win);
            }
            else if (game_result == Game::DRAW_BY_HALF_MOVES_LIMIT
                 or game_result == Game::DRAW_BY_REPETITIONS
                 or game_result == Game::DRAW_BY_STEALMATE) {
                packed_position.write(thread.output_draw);
            }
            else ASSERT(false, "No other game results");
        }

        positions.clear();

        thread.games_ended++;
        incGameCount(thread.commons);
    }

    print_thread_info(thread_id);
    std::cout << thread.games_ended << " games played on single thread - total of " << thread.total_positions 
              << " positions collected." << std::endl;
}

void DataCollector::startTournament(size_t games_count, size_t thread_count, SearchLimits limits) {
    std::ofstream output_white_win(std::string(_FileWhiteWin), std::ios::binary | std::ios::app);
    std::ofstream output_black_win(std::string(_FileBlackWin), std::ios::binary | std::ios::app);
    std::ofstream output_draw     (std::string(_FileDraw), std::ios::binary | std::ios::app);

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

    CommonThreadData thread_common;

    thread_common.games_ended = 0;
    thread_common.total_games = games_count;
    thread_common.total_positions = 0;

    std::vector<PerThreadData> thread_private;

    std::vector<std::thread>   threads;

    for (size_t i = 0; i < thread_count; i++) {
        PerThreadData data;

        std::string path = std::string(_FileWhiteWin) + "_thread_" + std::to_string(i) + ".epd";
        data.output_white_win.open(path, std::ios::binary | std::ios::app);

        path = std::string(_FileBlackWin) + "_thread_" + std::to_string(i) + ".epd";
        data.output_black_win.open(path);

        path = std::string(_FileDraw) + "_thread_" + std::to_string(i) + ".epd";
        data.output_draw.open(path);

        data.limits = limits;
        data.commons = &thread_common;

        thread_private.push_back(std::move(data));
    }

    std::cout << "Starting " << thread_count << " threads" << std::endl;

    for (size_t i = 0; i < thread_count; i++) {
        threads.push_back(std::thread(perThreadGameLoop, std::ref(thread_private[i])));
    }

    for (size_t i = 0; i < thread_count; i++) {
        threads[i].join();
    }
}

} // namespace Utils
