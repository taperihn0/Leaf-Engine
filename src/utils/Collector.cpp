#include "Collector.hpp"
#include "PackedPosition.hpp"
#include "Process.hpp"
#include "SelfGame.hpp"
#include "Entry.hpp"
#include "PackedNetwork.hpp"
#include "NetworkEval.hpp"

#include <thread>
#include <iomanip>

namespace Utils
{

void TournamentCollector::perThreadGameLoop(TournamentCollector::PerThreadData& thr_data) {
    ASSERT(thr_data.output_white_win.is_open(), "Output not opened.");
    ASSERT(thr_data.output_black_win.is_open(), "Output not opened.");
    ASSERT(thr_data.output_draw.is_open(), "Output not opened.");
    ASSERT(thr_data.commons != nullptr, "Thread commons not initialized");

    auto engine1 = spawnProcess();
    auto engine0 = spawnProcess();

    auto& is0 = *engine0.in;
    auto& os0 = *engine0.out;
    auto& is1 = *engine1.in;
    auto& os1 = *engine1.out;

    const enumLogLabel thread_label = threadLabel(thr_data.id);

    {
        std::string line;

        readline(os0, line);
        labelLog(std::cout, LOG_DEBUG | LOG_ENGINE_0 | thread_label, line);

        readline(os1, line);
        labelLog(std::cout, LOG_DEBUG | LOG_ENGINE_1 | thread_label, line);
        
        static const size_t mb_tt_size = 8;

        std::ostringstream tt_log;
        tt_log << "setoption name Hash value " << mb_tt_size;

        log(is0, tt_log.str());
        labelLog(std::cout, LOG_INFO | LOG_ENGINE_0 | thread_label, tt_log.str());

        log(is1, tt_log.str());
        labelLog(std::cout, LOG_INFO | LOG_ENGINE_1 | thread_label, tt_log.str());
    }

    auto positions = std::make_shared<std::vector<PackedPosition>>();
    positions->reserve(MaxGameMoves);

    auto white_scores = std::make_shared<std::vector<Score>>();
    white_scores->reserve(MaxGameMoves);

    thr_data.games_ended = 0;
    thr_data.total_positions = 0;
    thr_data.white_win_count = 0;
    thr_data.black_win_count = 0;
    thr_data.draw_count = 0;

    const ll nodes_per_search = thr_data.limits.nodes;
    const ll nodes_randomization_range = static_cast<ll>(nodes_per_search * _NodesRandomFactor);
    const ll nodes_min = nodes_per_search - nodes_randomization_range;
    const ll nodes_max = nodes_per_search + nodes_randomization_range;

    if (GlobOpeningGenerator.isEmpty())
        GlobOpeningGenerator.load();

    for (size_t i = 0; 
         thr_data.commons->games_ended < thr_data.commons->games2play; 
         i++) 
    {
        {
            std::ostringstream ss;
            ss << "White wins, Black wins, Draws: "
               << thr_data.white_win_count << ' '
               << thr_data.black_win_count << ' '
               << thr_data.draw_count;

            const std::lock_guard<std::mutex> lock(thr_data.commons->stdout_lock);
            labelLog(std::cout, LOG_INFO | thread_label, ss.str());
        }

        {
            std::ostringstream ss;
            ss << "Starting game " << i << "...";

            const std::lock_guard<std::mutex> lock(thr_data.commons->stdout_lock);
            labelLog(std::cout, LOG_INFO | thread_label, ss.str());
        }

        // add noise to search node number (if nodes threshold is used)
        if (nodes_per_search > 0) {
            thr_data.limits.nodes = random<ll>(nodes_min, nodes_max);
        }

        auto game_result = std::make_shared<Game::Result>(Game::GAME_INVALID);

        SelfGame::GameSpecPacket game_packet = {
            thr_data.limits,
            SelfGame::EnginePlayer{ &os0, &is0 },
            SelfGame::EnginePlayer{ &os1, &is1 },
            thr_data.id,
            game_result,
            &GlobOpeningGenerator,
            positions,
            white_scores,
        };

        SelfGame().mixedMatch<_EnableSelfPlayLog>(game_packet);

        const size_t game_positions_cnt = positions->size();
        thr_data.total_positions += game_positions_cnt;
        thr_data.commons->total_positions.fetch_add(game_positions_cnt);

        {
            std::ostringstream ss;
            ss << toStr(*game_result) << " - collected " << game_positions_cnt << " positions";

            const std::lock_guard<std::mutex> lock(thr_data.commons->stdout_lock);
            labelLog(std::cout, LOG_INFO | thread_label, ss.str());
        }

        if (isWhiteWin(*game_result)) {
            thr_data.white_win_count++;
            thr_data.commons->total_white_win_count.fetch_add(1);
        }

        else if (isBlackWin(*game_result)) {
            thr_data.black_win_count++;
            thr_data.commons->total_black_win_count.fetch_add(1);
        }

        else if (isDraw(*game_result)) {
            thr_data.draw_count++;
            thr_data.commons->total_draw_count.fetch_add(1);
        }

        else if (*game_result == Game::GAME_INVALID)
            break;

        thr_data.games_ended++;
        thr_data.commons->games_ended.fetch_add(1);

        ASSERTNOLOG(positions->size() == white_scores->size());

        for (size_t i = 0; i < positions->size(); i++) {
            PackedPosition& packed_pos = positions->at(i);
            const Score white_score = white_scores->at(i);

            const TrainingDataEntry::Result8b result8b = isWhiteWin(*game_result) ? TrainingDataEntry::WHITE_WIN :
                                                         isBlackWin(*game_result) ? TrainingDataEntry::BLACK_WIN :
                                                                                    TrainingDataEntry::DRAW;
            const TrainingDataEntry entry(packed_pos, white_score, result8b);

            if (isWhiteWin(*game_result) and 
                !TrainingDataEntry::write(thr_data.output_white_win, entry)) {
                labelLog(std::cout, LOG_INFO | thread_label, "Failed to write entry");
            }
            
            else if (isBlackWin(*game_result) and
                !TrainingDataEntry::write(thr_data.output_black_win, entry)) {
                labelLog(std::cout, LOG_INFO | thread_label, "Failed to write entry");
            }
            
            else if (isDraw(*game_result) and 
                    !TrainingDataEntry::write(thr_data.output_draw, entry)) {
                labelLog(std::cout, LOG_INFO | thread_label, "Failed to write entry");
            }
        }

        positions->clear();
        white_scores->clear();
    }

    {
        std::ostringstream ss;
        ss << thr_data.games_ended << " games played - total of " 
           << thr_data.total_positions 
           << " positions collected.";

        const std::lock_guard<std::mutex> lock(thr_data.commons->stdout_lock);
        labelLog(std::cout, LOG_INFO | thread_label, ss.str());
    }
}

void TournamentCollector::startTournament(size_t games_count, 
                                          uint thread_count, 
                                          const std::string& log_dir,
                                          SearchLimits limits) 
{
    if (thread_count > PlatformThreadLimit) {
        std::cout << "Too many threads requested" << std::endl;
        return;
    }

    auto thread_common = std::make_shared<CommonThreadData>();

    thread_common->games_ended = 0;
    thread_common->games2play = games_count;
    thread_common->total_positions = 0;
    thread_common->total_white_win_count = 0;
    thread_common->total_black_win_count = 0;
    thread_common->total_draw_count = 0;

    std::vector<PerThreadData> thread_private;
    std::vector<std::thread> threads;

    thread_private.reserve(thread_count);
    threads.reserve(thread_count);

    for (uint id = 0; id < thread_count; id++) {
        PerThreadData per_thread_data;

        {
            std::ostringstream ss;
            ss << log_dir << '/' << getWhiteWinOutputFile(id);
            per_thread_data.output_white_win.open(ss.str(), std::ios::binary | std::ios::app);
        }
        {
            std::ostringstream ss;
            ss << log_dir << '/' << getBlackWinOutputFile(id);
            per_thread_data.output_black_win.open(ss.str(), std::ios::binary | std::ios::app);
        }
        {
            std::ostringstream ss;
            ss << log_dir << '/' << getDrawOutputFile(id);
            per_thread_data.output_draw.open(ss.str(), std::ios::binary | std::ios::app);
        }

        per_thread_data.limits = limits;
        per_thread_data.commons = thread_common;
        per_thread_data.id = id;

        thread_private.push_back(std::move(per_thread_data));
    }

    std::ostringstream ss;
    ss << "Starting " << thread_count << " threads";

    labelLog(std::cout, LOG_INFO, ss.str());

    for (size_t i = 0; i < thread_count; i++) {
        threads.emplace_back([&](PerThreadData& thread_data) {
            this->perThreadGameLoop(thread_data);
        }, 
        std::ref(thread_private[i]));
    }

    for (auto& thread : threads) {
        thread.join();
    }
}

} // namespace Utils
