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

#pragma once

#include "SelfGame.hpp"

#include <atomic>
#include <mutex>

namespace utils
{

/* Self-play tournament class with data collection system.
*/
class TournamentCollector {
public:
    TournamentCollector() = default;

    struct TournamentPacket {
        std::size_t                games_count; 
        uint                  thread_count;
        std::filesystem::path selfplay_filename;
        search::SearchLimits  limits;
    };
    
    void startTournament(const TournamentPacket& packet);
    static bool explicitFilterPolicy(const Position& pos, 
                                     sc::Score white_score);
private:
    static bool internalFilterPolicy(Move32b internal_move,
                                     std::size_t internal_total_positions_cnt);
    bool filterTrainPosition(const Position& pos, 
                             sc::Score white_score, 
                             Move32b internal_move,
                             std::size_t internal_total_positions_cnt);

    struct CommonThreadData {
        std::atomic<std::size_t> games_ended;
        std::size_t              games2play;
        std::atomic<std::size_t> total_positions;
        std::atomic<std::size_t> total_white_win_count;
        std::atomic<std::size_t> total_black_win_count;
        std::atomic<std::size_t> total_draw_count;
        std::size_t              total_thread_cnt;
        std::filesystem::path 
                            err_fp;
        std::mutex          err_output_lock;
    };

    struct PerThreadData {
        std::ofstream output_white_win;
        std::ofstream output_black_win;
        std::ofstream output_draw;
        search::SearchLimits  limits;
        std::size_t        games_ended;
        std::size_t        white_win_count;
        std::size_t        black_win_count;
        std::size_t        draw_count;
        std::size_t        total_positions;
        uint          id;
        std::shared_ptr<CommonThreadData> 
                      commons;
    };

    bool threadTournamentWorker(PerThreadData& thread, 
                                enumLogLabel thread_label);
    void perThread(PerThreadData& thread);

    static constexpr float _NodesRandomFactor = 0.18f;
#if defined(DEBUG)
    static constexpr bool  _EnableSelfPlayLog = true;
#else
    static constexpr bool  _EnableSelfPlayLog = false;
#endif
};

} // namespace utils
