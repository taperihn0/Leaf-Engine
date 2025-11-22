#include "Collector.hpp"

#include <thread>
#include <iomanip>

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
    ASSERT(thread.output_draw.is_open(),      "Output not opened.");
    ASSERT(thread.commons != nullptr,         "Thread commons not initialized");

    std::vector<PackedPosition> positions;
    positions.reserve(MaxGameMoves);

    using thread_id_t = std::thread::id;

    thread_id_t thread_id = std::this_thread::get_id();

    auto print_thread_info = [&](thread_id_t id) {
        std::cout << "[THREAD " << std::setw(5) << std::hex << thread_id << std::dec << "] ";
    };

    thread.games_ended = 0;
    thread.total_positions = 0;
    thread.white_win_count = 0;
    thread.black_win_count = 0;
    thread.draw_count = 0;

    const ll nodes_per_search = thread.limits.nodes;
    const ll nodes_randomization_range = static_cast<ll>(nodes_per_search * DataCollector::NodesRandomFactor);
    const ll nodes_min = nodes_per_search - nodes_randomization_range;
    const ll nodes_max = nodes_per_search + nodes_randomization_range;

    SelfGame match(1_MB);

    for (size_t i = 0; thread.commons->games_ended < thread.commons->total_games; i++) {

        thread.commons->stdout_lock.lock();

        print_thread_info(thread_id);
        std::cout << "White wins, Black wins, Draws: "
                  << thread.white_win_count << ' '
                  << thread.black_win_count << ' '
                  << thread.draw_count      << std::endl;

        thread.commons->stdout_lock.unlock();

        thread.commons->stdout_lock.lock();
        print_thread_info(thread_id);
        std::cout << "Starting game " << i << "..." << std::endl;
        thread.commons->stdout_lock.unlock();

        // add noise to search node number (if nodes threshold is used)
        if (nodes_per_search > 0) {
            thread.limits.nodes = random<ll>(nodes_min, nodes_max);
        }

        Game::Result game_result = match.start(positions, thread.limits);

        size_t game_positions_cnt = positions.size();
        thread.total_positions += game_positions_cnt;

        thread.commons->stdout_lock.lock();
        print_thread_info(thread_id);
        std::cout << toStr(game_result) << " - collected " << game_positions_cnt << " positions\n";
        thread.commons->stdout_lock.unlock();

        if (isWhiteWin(game_result))
            thread.white_win_count++;

        else if (isBlackWin(game_result))
            thread.black_win_count++;

        else if (isDraw(game_result))
            thread.draw_count++;

        for (size_t j = 0; j < game_positions_cnt; j++) {

            const PackedPosition& packed_position = positions[j];

            if (isWhiteWin(game_result))
                PackedPosition::write(thread.output_white_win, packed_position);
            
            else if (isBlackWin(game_result))
                PackedPosition::write(thread.output_black_win, packed_position);
            
            else if (isDraw(game_result)) 
                PackedPosition::write(thread.output_draw, packed_position);
        }

        positions.clear();

        thread.games_ended++;
        incGameCount(thread.commons);
    }

    thread.commons->stdout_lock.lock();
    print_thread_info(thread_id);
    std::cout << thread.games_ended << " games played on thread " << thread_id << " - total of " 
              << thread.total_positions 
              << " positions collected." << std::endl;
    thread.commons->stdout_lock.unlock();
}

void DataCollector::startTournament(size_t games_count, size_t thread_count, SearchLimits limits) {
    CommonThreadData thread_common;

    thread_common.games_ended = 0;
    thread_common.total_games = games_count;
    thread_common.total_positions = 0;

    std::vector<PerThreadData> thread_private;

    std::vector<std::thread>   threads;

    for (size_t i = 0; i < thread_count; i++) {
        PerThreadData data;

        std::string path = std::string(_FileWhiteWin) + "_thread_" + std::to_string(i) + ".pck";
        data.output_white_win.open(path, std::ios::binary | std::ios::app);

        path = std::string(_FileBlackWin) + "_thread_" + std::to_string(i) + ".pck";
        data.output_black_win.open(path, std::ios::binary | std::ios::app);

        path = std::string(_FileDraw) + "_thread_" + std::to_string(i) + ".pck";
        data.output_draw.open(path, std::ios::binary | std::ios::app);

        data.limits = limits;
        data.commons = &thread_common;

        thread_private.push_back(std::move(data));
    }

    std::cout << "Starting " << thread_count << " threads" << std::endl;

    for (size_t i = 0; i < thread_count; i++) {
        threads.push_back(std::thread(perThreadGameLoop, std::ref(thread_private[i])));
    }

    for (auto& thread : threads) {
        thread.join();
    }
}

} // namespace Utils
