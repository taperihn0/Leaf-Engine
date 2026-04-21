#pragma once

#include "SelfGame.hpp"

#include <atomic>
#include <mutex>

namespace Utils
{

/* Self-play tournament class with data collection system.
*/
class TournamentCollector {
public:
    TournamentCollector() = default;
    void startTournament(size_t games_count, 
                         uint thread_count, 
                         const std::string& log_dir,
                         const std::string& err_log_dir,
                         SearchLimits limits);
private:
    bool filterTrainPosition(const Position& pos, 
                             Score white_score, 
                             Move32b move,
                             size_t total_positions_cnt);

    struct CommonThreadData {
        std::atomic<size_t> games_ended;
        size_t              games2play;
        std::atomic<size_t> total_positions;
        std::atomic<size_t> total_white_win_count;
        std::atomic<size_t> total_black_win_count;
        std::atomic<size_t> total_draw_count;
        size_t              total_thread_cnt;
        std::ofstream       err_output;
        std::mutex          stdout_lock;
        std::mutex          err_output_lock;
    };

    struct PerThreadData {
        std::ofstream output_white_win;
        std::ofstream output_black_win;
        std::ofstream output_draw;
        SearchLimits  limits;
        size_t        games_ended;
        size_t        white_win_count;
        size_t        black_win_count;
        size_t        draw_count;
        size_t        total_positions;
        uint          id;
        std::shared_ptr<CommonThreadData> commons;
    };

    bool threadTournament(PerThreadData& thread, 
                          enumLogLabel thread_label);
    void perThread(PerThreadData& thread);

    static constexpr float _NodesRandomFactor = 0.18f;
#if defined(DEBUG)
    static constexpr bool _EnableSelfPlayLog = false;
#else
    static constexpr bool _EnableSelfPlayLog = false;
#endif
};

} // namespace Utils
