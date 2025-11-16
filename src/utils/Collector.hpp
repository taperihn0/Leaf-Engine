#pragma once

#include "SelfGame.hpp"
#include <mutex>

namespace Utils
{
    
class DataCollector {
public:
    DataCollector() = default;
    void startTournament(size_t games_count, size_t thread_count, SearchLimits limits);

    struct CommonThreadData;

    struct PerThreadData {
        std::ofstream     output_white_win;
        std::ofstream     output_black_win;
        std::ofstream     output_draw;

        SearchLimits      limits;

        size_t            games_ended;
        size_t            total_positions;

        CommonThreadData* commons = nullptr;
    };

    struct CommonThreadData {
        size_t     games_ended;
        size_t     total_games;
        size_t     total_positions;
        std::mutex common_lock;
    };

private:
    static constexpr std::string_view _FileWhiteWin = "src/utils/selfplay/selfplay_white_win";
    static constexpr std::string_view _FileBlackWin = "src/utils/selfplay/selfplay_black_win";
    static constexpr std::string_view _FileDraw     = "src/utils/selfplay/selfplay_draw";
};

} // namespace Utils
