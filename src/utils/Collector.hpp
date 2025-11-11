#pragma once

#include "SelfGame.hpp"

namespace Utils
{
    
class DataCollector {
public:
    DataCollector() = default;
    void startTournament(size_t games_count, SearchLimits limits);
private:
    static constexpr std::string_view _FileWhiteWin = "src/utils/selfplay/selfplay_white_win.epd";
    static constexpr std::string_view _FileBlackWin = "src/utils/selfplay/selfplay_black_win.epd";
    static constexpr std::string_view _FileDraw     = "src/utils/selfplay/selfplay_draw.epd";
    SelfGame _match;
};

} // namespace Utils
