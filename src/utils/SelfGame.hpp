#pragma once

#include "UtilsCommon.hpp"
#include "Opening.hpp"
#include "PackedPosition.hpp"

namespace Utils {

class SelfGame {
public:
    SelfGame(size_t tt_size_per_search = 1_MB);
    Game::Result start(std::vector<ExtPackedPosition>& packed_positions, SearchLimits limits);
private:
    Search _search_by_side[2];
};

} // namespace Utils
