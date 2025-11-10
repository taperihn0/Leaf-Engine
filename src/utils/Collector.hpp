#pragma once

#include "SelfGame.hpp"

namespace Utils
{
    
class DataCollector {
public:
    DataCollector() = default;
    void startTournament(size_t games_count, std::string file, SearchLimits limits);
private:
    SelfGame _match;
};

} // namespace Utils
